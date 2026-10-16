#!/usr/bin/env python3
"""Verify Task3 Master survives a mapped DDS subscriber stop and restart."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

parser = argparse.ArgumentParser()
parser.add_argument('--build', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'task3' / 'tests'))
from runtime_library_verification import verify_statistics_backend_mapped
build, out = Path(args.build).resolve(), Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)
identities = out / 'identities'
identities.mkdir(exist_ok=True)
config = {
    'host_id': 'subscriber-restart-test',
    'telemetry_domain': 94,
    'interval_ms': 300,
    'identity_directory': str(identities),
    'applications': [
        {'application_id': app, 'process_name': 'task3_probe',
         'argument_token': app, 'identity_required': True}
        for app in ('pub', 'sub')
    ],
}
config_path = out / 'agent.json'
config_path.write_text(json.dumps(config))
env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QSG_RHI_BACKEND='software',
           TASK3_DATA_DIR=str(out / 'state'),
           TASK3_SNAPSHOT_PATH=str(out / 'snapshot.json'),
           TASK3_TELEMETRY_DOMAIN='94', TASK3_TIMEOUT_MS='2000',
           TASK3_EXIT_AFTER_MS='90000')
children, logs = [], []
results = {}
runtime_library_maps = []


def start(name, argv):
    log = (out / (name + '.log')).open('w')
    logs.append(log)
    process = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT, env=env)
    children.append(process)
    if name == 'master':
        runtime_library_maps.append(verify_statistics_backend_mapped(
            process.pid, env.get('TASK3_PATCHED_BACKEND_PREFIX')))
    return process


def stop(process, require_clean=False):
    if process.poll() is None:
        process.send_signal(signal.SIGTERM)
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
    if require_clean and process.returncode != 0:
        raise AssertionError(f'Expected clean shutdown, got {process.returncode}')


def snapshot():
    try:
        return json.loads((out / 'snapshot.json').read_text())
    except (OSError, ValueError):
        return {}


def mapped(data, app):
    return [entity for entity in data.get('entities', [])
            if entity.get('kind') == 'Participant' and entity.get('alive')
            and entity.get('application_id') == app]


def wait(name, predicate, timeout=25):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        data = snapshot()
        if data.get('master_pid') == master.pid and predicate(data):
            results[name] = 'PASS'
            (out / (name + '.json')).write_text(json.dumps(data, indent=2))
            return data
        if master.poll() is not None:
            raise AssertionError(f'Master exited: {master.returncode}')
        time.sleep(0.2)
    results[name] = 'FAIL'
    raise AssertionError(f'Timeout: {name}')


def probe(app, label):
    mode = 'writer' if app == 'pub' else 'reader'
    return start(label, [str(build / 'task3/task3_probe'), '--app', app,
                         '--mode', mode, '--identity-dir', str(identities),
                         '--participants', '2'])


try:
    master = start('master', [str(build / 'dds_monitor')])
    agent = start('agent', [str(build / 'task3/task3_agent'), '--config', str(config_path)])
    subscriber = probe('sub', 'subscriber')
    publisher = probe('pub', 'publisher')
    initial = wait('subscriber_and_publisher_discovered',
                   lambda data: len(mapped(data, 'pub')) == 2 and len(mapped(data, 'sub')) == 2)
    previous_sub_guids = {entity['guid'] for entity in mapped(initial, 'sub')}

    stop(subscriber, require_clean=True)
    wait('subscriber_disposal_observed',
         lambda data: not mapped(data, 'sub') and len(mapped(data, 'pub')) == 2)

    subscriber = probe('sub', 'subscriber_restart')
    restarted = wait('subscriber_restart_remapped',
                     lambda data: len(mapped(data, 'sub')) == 2 and
                     not previous_sub_guids.intersection(entity['guid'] for entity in mapped(data, 'sub')))
    results['master_alive_after_subscriber_restart'] = 'PASS' if master.poll() is None else 'FAIL'
    results['publisher_alive_during_subscriber_restart'] = (
        'PASS' if publisher.poll() is None and len(mapped(restarted, 'pub')) == 2 else 'FAIL')
    stop(master, require_clean=True)
    results['master_graceful_shutdown'] = 'PASS'
except Exception as error:
    results['error'] = str(error)
finally:
    for process in reversed(children):
        stop(process)
    for log in logs:
        log.close()
    (out / 'results.json').write_text(json.dumps(results, indent=2))
    (out / 'runtime-library-maps.json').write_text(json.dumps(runtime_library_maps, indent=2))
    print(json.dumps(results, indent=2))

raise SystemExit(1 if 'error' in results or 'FAIL' in results.values() else 0)
