#!/usr/bin/env python3
"""Controlled Publisher stop/restart against one DDS Monitor process."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

parser = argparse.ArgumentParser()
parser.add_argument('--master', required=True)
parser.add_argument('--probe', required=True)
parser.add_argument('--output', required=True)
parser.add_argument('--cycles', type=int, default=1)
parser.add_argument('--dwell', type=float, default=2.0)
parser.add_argument('--skip-snapshot', action='store_true')
args = parser.parse_args()
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'task3' / 'tests'))
from runtime_library_verification import verify_statistics_backend_mapped
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=False)
identities = out / 'identities'
identities.mkdir()
pid_file = out / 'inferior.pid'
env = os.environ.copy()
env.update({
    'QT_QPA_PLATFORM': 'offscreen',
    'QSG_RHI_BACKEND': 'software',
    'TASK3_KEY': 'p0-isolation-key-not-for-deployment',
    'TASK3_DATA_DIR': str(out / 'state'),
    'TASK3_SNAPSHOT_PATH': str(out / 'snapshot.json'),
    'TASK3_SCREENSHOT_PATH': str(out / 'dashboard.png'),
    'TASK3_TELEMETRY_DOMAIN': '95',
    'TASK3_TIMEOUT_MS': '5000',
    'TASK3_FLOW_RULES': str(out / 'flows.json'),
    'TASK3_EXIT_AFTER_MS': '900000',
    'P0_INFERIOR_PID_FILE': str(pid_file),
})
(out / 'flows.json').write_text('[]')
wrapper = Path(__file__).with_name('dds-monitor-gdb')
logs, children = [], []
result = {'requested_cycles': args.cycles, 'completed_cycles': 0, 'cycles': []}


def launch(name, command):
    stream = (out / (name + '.log')).open('w')
    logs.append(stream)
    process = subprocess.Popen(command, env=env, stdout=stream, stderr=subprocess.STDOUT)
    children.append(process)
    return process


def stop(process, timeout=12):
    if process.poll() is None:
        process.send_signal(signal.SIGTERM)
        try:
            process.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    return process.returncode


master = None
try:
    master = launch('master', ['/bin/sh', str(wrapper), args.master])
    deadline = time.monotonic() + 45
    while time.monotonic() < deadline and not pid_file.exists():
        if master.poll() is not None:
            raise RuntimeError(f'GDB/Master exited before main: {master.returncode}')
        time.sleep(0.1)
    if not pid_file.exists():
        raise TimeoutError('GDB did not publish inferior PID')
    inferior_pid = int(pid_file.read_text())
    result['runtime_backend_map'] = verify_statistics_backend_mapped(
        inferior_pid, env.get('TASK3_PATCHED_BACKEND_PREFIX'))
    if args.skip_snapshot:
        time.sleep(5)
    else:
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if master.poll() is not None:
                raise RuntimeError(f'Master exited during startup: {master.returncode}')
            try:
                snapshot = json.loads((out / 'snapshot.json').read_text())
                if snapshot.get('master_pid') == inferior_pid:
                    break
            except (OSError, ValueError):
                pass
            time.sleep(0.2)
        else:
            raise TimeoutError('Master snapshot did not become ready')

    probe = [args.probe, '--domain', '0', '--participants', '2',
             '--identity-dir', str(identities)]
    subscriber = launch('subscriber', probe + ['--app', 'sub', '--mode', 'reader'])
    time.sleep(2)
    if master.poll() is not None or subscriber.poll() is not None:
        raise RuntimeError('Master or Subscriber exited before Publisher loop')

    for cycle in range(1, args.cycles + 1):
        publisher = launch(f'publisher-{cycle:02d}', probe + ['--app', 'pub', '--mode', 'writer'])
        time.sleep(args.dwell)
        if master.poll() is not None or publisher.poll() is not None:
            raise RuntimeError(f'Master/Publisher exited before stop on cycle {cycle}')
        publisher_rc = stop(publisher)
        time.sleep(1)
        master_rc = master.poll()
        result['cycles'].append({
            'cycle': cycle,
            'publisher_exit': publisher_rc,
            'master_alive_after_stop': master_rc is None,
        })
        if publisher_rc != 0:
            raise RuntimeError(f'Publisher did not stop cleanly on cycle {cycle}: {publisher_rc}')
        if master_rc is not None:
            raise RuntimeError(f'Master/GDB exited after Publisher stop on cycle {cycle}: {master_rc}')
        result['completed_cycles'] = cycle

    result['status'] = 'PASS'
except Exception as error:
    result['status'] = 'FAIL'
    result['error'] = str(error)
finally:
    for process in reversed(children):
        if process is master:
            continue
        stop(process)
    if master is not None and master.poll() is None and pid_file.exists():
        try:
            os.kill(int(pid_file.read_text()), signal.SIGTERM)
            master.wait(timeout=25)
        except (OSError, subprocess.TimeoutExpired):
            if master.poll() is None:
                master.kill()
                master.wait()
    result['master_gdb_exit'] = master.returncode if master is not None else None
    for stream in logs:
        stream.close()
    (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))

raise SystemExit(0 if result.get('status') == 'PASS' else 1)
