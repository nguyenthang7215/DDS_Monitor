#!/usr/bin/env python3
"""Real Linux agents and native Qt window: navigation, selection, screenshots and lifecycle.

All processes run on one machine. Uses isolated state and only stops owned children.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import signal
import sqlite3
import struct
import subprocess
import time
import zlib

from runtime_library_verification import verify_statistics_backend_mapped

cli = argparse.ArgumentParser()
cli.add_argument('--build', required=True)
cli.add_argument('--output', required=True)
cli.add_argument('--platform', choices=['xcb', 'offscreen'], default='offscreen')
cli.add_argument('--width', type=int, default=1600)
cli.add_argument('--height', type=int, default=900)
cli.add_argument('--long-labels', action='store_true', help='Exercise real telemetry with long host/application IDs')
args = cli.parse_args()
build = Path(args.build).resolve()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=False)
identities = out / 'identities'
identities.mkdir()
env = dict(os.environ, QT_QPA_PLATFORM=args.platform, QSG_RHI_BACKEND='opengl',
           TASK3_DATA_DIR=str(out/'state'), TASK3_SNAPSHOT_PATH=str(out/'snapshot.json'),
           TASK3_SCREENSHOT_PATH=str(out/'desktop.png'), TASK3_TELEMETRY_DOMAIN='94',
           TASK3_TIMEOUT_MS='1800', TASK3_EXPECTED_HOSTS='zz-awaiting-agent',
           TASK3_UI_WIDTH=str(args.width), TASK3_UI_HEIGHT=str(args.height),
           TASK3_UI_EXERCISE='desktop', TASK3_EXIT_AFTER_MS='150000')
children, logs, results, observations = [], [], {}, []

def start(name, command):
    log = (out/(name+'.log')).open('w')
    logs.append(log)
    process = subprocess.Popen(command, env=env, stdout=log, stderr=subprocess.STDOUT)
    children.append(process)
    return process

def stop(process):
    if process.poll() is None:
        process.send_signal(signal.SIGTERM)
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

def snapshot():
    try:
        return json.loads((out/'snapshot.json').read_text())
    except (OSError, ValueError):
        return {}

def verify_png(path):
    data = path.read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', 'Invalid PNG: '+str(path)
    pos = 8
    while pos + 12 <= len(data):
        length = struct.unpack('>I', data[pos:pos+4])[0]
        kind = data[pos+4:pos+8]
        end = pos+8+length
        assert end+4 <= len(data), 'Truncated PNG: '+str(path)
        assert zlib.crc32(data[pos+4:end]) == struct.unpack('>I', data[end:end+4])[0], 'PNG CRC error: '+str(path)
        if kind == b'IHDR':
            assert struct.unpack('>II', data[pos+8:pos+16]) == (args.width, args.height), 'Wrong screenshot dimensions'
        pos = end+4
        if kind == b'IEND':
            assert pos == len(data), 'Trailing PNG data'
            return
    raise AssertionError('Missing PNG end: '+str(path))

def wait(name, predicate, timeout=35):
    end = time.monotonic()+timeout
    while time.monotonic() < end:
        assert master.poll() is None, 'Master exited: '+str(master.returncode)
        data = snapshot()
        if predicate(data):
            results[name] = 'PASS'
            (out/(name+'.json')).write_text(json.dumps(data, indent=2))
            return data
        if master.poll() is not None:
            raise AssertionError('Master exited: '+str(master.returncode))
        time.sleep(.1)
    raise AssertionError('Timeout: '+name)

host_a = 'linux-agent-a' + ('-long-host-identifier-for-desktop-layout-validation-0123456789' if args.long_labels else '')
publisher = 'publisher' + ('-long-application-identifier-for-desktop-layout-validation-0123456789' if args.long_labels else '')
config = dict(host_id=host_a, telemetry_domain=94, interval_ms=250,
              identity_directory=str(identities), disk_paths=['/', '/tmp'], applications=[
                  dict(application_id=publisher, process_name='task3_probe', argument_token=publisher, identity_required=True),
                  dict(application_id='subscriber', process_name='task3_probe', argument_token='subscriber', identity_required=True),
                  dict(application_id='missing-process', process_name='dds-ui-test-no-such-process')])
config_a, config_b = out/'agent-a.json', out/'agent-b.json'
config_a.write_text(json.dumps(config))
config_b.write_text(json.dumps(dict(config, host_id='linux-agent-b', applications=[])))
try:
    master = start('master', [str(build/'dds_monitor')])
    mapping = verify_statistics_backend_mapped(master.pid, env.get('TASK3_PATCHED_BACKEND_PREFIX'))
    (out/'runtime-library-maps.json').write_text(json.dumps(mapping, indent=2))
    agent_a = start('agent-a', [str(build/'task3/task3_agent'), '--config', str(config_a)])
    agent_b = start('agent-b', [str(build/'task3/task3_agent'), '--config', str(config_b)])
    sub = start('subscriber', [str(build/'task3/task3_probe'), '--app', 'subscriber', '--mode', 'reader', '--identity-dir', str(identities)])
    pub = start('publisher', [str(build/'task3/task3_probe'), '--app', publisher, '--mode', 'writer', '--identity-dir', str(identities)])
    initial = wait('default_fleet_overview', lambda d: d.get('ui', {}).get('dashboard_loaded') and
                   d['ui']['workspace'] == 0 and d['ui']['host_id'] == '')
    wait('real_agents_and_unknown_host', lambda d: len(d.get('hosts', [])) == 3 and
         sum(h['online'] for h in d['hosts']) == 2 and any(h['metrics_state'] == 'unknown' for h in d['hosts']))
    time.sleep(.2)
    shutil.copyfile(out/'desktop.png.system.png', out/'overview.png')
    wait('mapped_application_and_flow', lambda d: any(e.get('kind') == 'Participant' and e.get('application_id') == publisher for e in d.get('entities', [])) and
         any(f.get('source_application') == publisher and f.get('target_application') == 'subscriber' for f in d.get('flows', [])))
    seen = set()
    end = time.monotonic()+40
    while time.monotonic() < end:
        assert master.poll() is None, 'Master exited: '+str(master.returncode)
        data = snapshot()
        ui = data.get('ui', {})
        if ui.get('dashboard_loaded'):
            assert ui['dds_workspace_count'] == 1, 'DDS workspace recreated'
            assert ui['title'] == 'DDS Monitor'
            assert ui['selection_retained'], 'Selection lost across navigation'
            assert (ui['width'], ui['height']) == (args.width, args.height), 'Unexpected viewport size'
            if ui['workspace'] == 0 and ui['host_id'] == host_a and ui['detail_tab'] == 0 and ui['resource_tab'] == 1:
                network = ui.get('network_view', {})
                assert network.get('width', 0) > 300 and network.get('height', 0) > 80, 'Network list collapsed'
                assert network.get('count', 0) > 0 and network.get('content_height', 0) > 0, 'Network rows missing'
                results['network_list_layout'] = 'PASS'
            seen.add(ui['workspace'])
            if not observations or observations[-1] != ui:
                observations.append(ui)
        if data.get('ui_steps', 0) >= 20 and seen == {0, 1, 2} and ui.get('application_id'):
            break
        time.sleep(.1)
    assert seen == {0, 1, 2}, 'Not all three workspaces visited'
    assert data['ui']['host_id'] == host_a and data['ui']['application_id'] == publisher
    assert data['ui']['dds_started'], 'Original DDS monitor did not initialize'
    results['three_tabs_selection_and_one_dds_workspace'] = 'PASS'
    assert results.get('network_list_layout') == 'PASS', 'Network page layout not exercised'
    for page in ['resources', 'network', 'system-1', 'system-2', 'system-3', 'events', 'dds']:
        shutil.copyfile(out/('desktop.png.'+page+'.png'), out/(page+'-online.png'))
    stop(agent_a)
    wait('offline_stale', lambda d: any(h['host_id'] == host_a and not h['online'] and h['metrics_state'] == 'stale' for h in d.get('hosts', [])))
    wait('offline_unverified_alert', lambda d: any(e.get('verification') == 'Not verified — Agent Offline' for e in d.get('events', [])))
    wait('offline_ui_selected', lambda d: d['ui']['workspace'] == 0 and d['ui']['detail_tab'] == 0 and d['ui']['host_id'] == host_a)
    time.sleep(.3)
    shutil.copyfile(out/'desktop.png.system.png', out/'host-offline.png')
    wait('offline_event_page', lambda d: d['ui']['workspace'] == 2)
    time.sleep(.3)
    shutil.copyfile(out/'desktop.png.events.png', out/'events-unverified.png')
    agent_a = start('agent-a-restart', [str(build/'task3/task3_agent'), '--config', str(config_a)])
    wait('agent_recovery', lambda d: any(h['host_id'] == host_a and h['online'] for h in d.get('hosts', [])))
    wait('recovered_event_page', lambda d: d['ui']['workspace'] == 2 and
         any(e.get('kind') == 'agent_timeout' and e.get('state') == 'Recovered' for e in d.get('events', [])))
    time.sleep(.3)
    shutil.copyfile(out/'desktop.png.events.png', out/'events-recovery.png')
    for page in ['system', 'dds', 'events']:
        image = out/('desktop.png.'+page+'.png')
        assert image.exists() and image.stat().st_size > 1000, 'Missing screenshot: '+page
    results['three_desktop_screenshots'] = 'PASS'
    assert all((out/('desktop.png.'+name+'.png')).exists() for name in ['resources', 'network'])
    results['resources_and_network_screenshots'] = 'PASS'
    stop(master)
    assert master.returncode == 0, 'Unclean Master shutdown'
    results['master_shutdown'] = 'PASS'
    for capture in out.glob('*.png'):
        verify_png(capture)
    results['screenshot_integrity_and_dimensions'] = 'PASS'
    with sqlite3.connect(out/'state/events.sqlite') as db:
        assert db.execute('pragma integrity_check').fetchone()[0] == 'ok'
    results['sqlite_integrity'] = 'PASS'
    errors = [line for line in (out/'master.log').read_text().splitlines()
              if any(token in line for token in ['TypeError', 'ReferenceError', 'Binding loop',
                 'binding loop', 'Cannot assign', 'is not a type', 'Expected token', 'Unable to assign', 'QThread: Destroyed'])]
    (out/'qml-errors.json').write_text(json.dumps(errors, indent=2))
    assert not errors, 'QML errors; see qml-errors.json'
    results['qml_runtime_errors'] = 'PASS'
    results['windows_and_physical_multi_host'] = 'PENDING_EXTERNAL_TEST'
except Exception as error:
    results['error'] = str(error)
finally:
    for child in reversed(children):
        stop(child)
    for log in logs:
        log.close()
    results['process_exit_codes'] = [{'pid': p.pid, 'executable': Path(p.args[0]).name, 'exit_code': p.returncode} for p in children]
    (out/'ui-observations.json').write_text(json.dumps(observations, indent=2))
    (out/'results.json').write_text(json.dumps(results, indent=2))
    print(json.dumps(results, indent=2))
raise SystemExit(1 if 'error' in results else 0)
