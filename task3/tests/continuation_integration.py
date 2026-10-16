#!/usr/bin/env python3
"""Owned local DDS processes: runtime GUID lifecycle, matching, silent writer, UI and shutdown.
All processes run on ONE Linux machine. No system services or clock settings are modified.
"""
import argparse
import json
import os
from pathlib import Path
import signal
import sqlite3
import subprocess
import time
from runtime_library_verification import verify_statistics_backend_mapped

cli = argparse.ArgumentParser()
cli.add_argument('--build', required=True)
cli.add_argument('--output', required=True)
cli.add_argument('--duration', type=int, default=60)
args = cli.parse_args()
build = Path(args.build).resolve()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)
children, logs, results, usage = [], [], {}, []
runtime_library_maps = []
env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QSG_RHI_BACKEND='software',
           TASK3_KEY='runtime-integration-key-not-for-deployment', TASK3_DATA_DIR=str(out/'state'),
           TASK3_SNAPSHOT_PATH=str(out/'snapshot.json'), TASK3_SCREENSHOT_PATH=str(out/'dashboard.png'),
           TASK3_TELEMETRY_DOMAIN='95', TASK3_TIMEOUT_MS='2000', TASK3_UI_EXERCISE='1',
           TASK3_FLOW_RULES=str(out/'flows.json'), TASK3_EXIT_AFTER_MS='240000')
(out/'flows.json').write_text(json.dumps([{'topic':'Task3/RuntimeProbe', 'mode':'periodic', 'deadline_ms':1500}]))
identities = out/'identities'
identities.mkdir(exist_ok=True)
config = {'host_id':'runtime-linux', 'telemetry_domain':95, 'interval_ms':200,
          'identity_directory':str(identities), 'applications':[
              {'application_id':name, 'process_name':'task3_probe', 'argument_token':name, 'identity_required':True}
              for name in ['pub', 'sub']]}
(out/'agent.json').write_text(json.dumps(config))

def start(name, argv):
    log = (out/(name+'.log')).open('w')
    logs.append(log)
    p = subprocess.Popen(argv, stdout=log, stderr=subprocess.STDOUT, env=env)
    children.append(p)
    if name in ('master', 'master_restart'):
        runtime_library_maps.append(verify_statistics_backend_mapped(
            p.pid, env.get('TASK3_PATCHED_BACKEND_PREFIX')))
    return p

def stop(p, require_clean=False):
    if p.poll() is None:
        p.send_signal(signal.SIGTERM)
        try:
            p.wait(timeout=10)
        except subprocess.TimeoutExpired:
            p.kill(); p.wait()
    if require_clean and p.returncode != 0:
        raise AssertionError('Expected clean shutdown: '+str(p.returncode))

def snapshot():
    try:
        return json.loads((out/'snapshot.json').read_text())
    except (OSError, ValueError):
        return {}

def wait(name, predicate, timeout=30):
    deadline = time.monotonic()+timeout
    while time.monotonic() < deadline:
        data = snapshot()
        if data.get('master_pid') == master.pid and predicate(data):
            results[name] = 'PASS'
            (out/(name+'.json')).write_text(json.dumps(data, indent=2))
            return data
        if master.poll() is not None:
            raise AssertionError('Master exited: '+str(master.returncode))
        time.sleep(.2)
    results[name] = 'FAIL'
    raise AssertionError('Timeout: '+name)

def mapped(data, app):
    return [e for e in data.get('entities',[]) if e.get('kind')=='Participant' and
            e.get('alive') and e.get('application_id')==app]

def probe(name, mode, silent=0):
    return start(name, [str(build/'task3/task3_probe'), '--app', mode, '--mode', 'writer' if mode=='pub' else 'reader',
        '--identity-dir', str(identities), '--participants', '2', '--silent-after-ms', str(silent)])

try:
    master = start('master', [str(build/'dds_monitor')])
    agent = start('agent', [str(build/'task3/task3_agent'), '--config', str(out/'agent.json')])
    sub = probe('subscriber', 'sub')
    pub = probe('publisher', 'pub', 12000)
    data = wait('runtime_mapping_multiple_participants', lambda d: len(mapped(d,'pub'))==2 and len(mapped(d,'sub'))==2)
    original_guids = {e['guid'] for e in mapped(data,'pub')}
    wait('topology_application_routes', lambda d: any(f.get('source_application')=='pub' and f.get('target_application')=='sub' for f in d.get('flows',[])))
    wait('fresh_connection_list_matching', lambda d: any(f.get('confirmed') and f.get('source_application')=='pub' for f in d.get('flows',[])))
    wait('alive_writer_no_data', lambda d: any(e.get('kind')=='Writer' and e.get('application_id')=='pub' and e.get('alive') and
          e.get('data_state','').startswith('No Data') for e in d.get('entities',[])), 35)
    stop(pub, True)
    wait('stopped_run_unmapped', lambda d: not mapped(d,'pub'))
    pub = probe('publisher_restart', 'pub')
    data = wait('restart_guid_remapping', lambda d: len(mapped(d,'pub'))==2 and not original_guids.intersection(e['guid'] for e in mapped(d,'pub')))
    wait('runtime_counter_recovery', lambda d: any(e.get('kind')=='dds_no_data' and e.get('state')=='Recovered' for e in d.get('events',[])))
    stop(agent, True)
    wait('agent_offline', lambda d: d.get('hosts') and not d['hosts'][0]['online'])
    agent = start('agent_restart', [str(build/'task3/task3_agent'), '--config', str(out/'agent.json')])
    wait('agent_restart_identity_recovery', lambda d: d.get('hosts') and d['hosts'][0]['online'] and len(mapped(d,'pub'))==2)
    stop(master, True)
    master = start('master_restart', [str(build/'dds_monitor')])
    wait('master_restart_live_agent', lambda d: d.get('hosts') and d['hosts'][0]['online'])
    wait('single_window_navigation_resize', lambda d: d.get('window_count')==1 and d.get('ui_steps',0)>=8)
    end = time.monotonic()+max(5,args.duration)
    while time.monotonic() < end:
        if any(p.poll() is not None for p in [master,agent,pub,sub]):
            raise AssertionError('A long-run child exited')
        stat = Path('/proc/'+str(master.pid)+'/stat').read_text().rsplit(')',1)[1].split()
        status = Path('/proc/'+str(master.pid)+'/status').read_text().splitlines()
        rss = int(next(line for line in status if line.startswith('VmRSS:')).split()[1])
        usage.append({'monotonic':time.monotonic(), 'rss_kib':rss, 'cpu_ticks':int(stat[11])+int(stat[12]),
                      'ui_steps':snapshot().get('ui_steps',0)})
        time.sleep(1)
    results['local_long_run'] = 'PASS'
    stop(master, True); results['master_graceful_shutdown'] = 'PASS'
    with sqlite3.connect(out/'state/events.sqlite') as db:
        results['sqlite_integrity'] = 'PASS' if db.execute('pragma integrity_check').fetchone()[0]=='ok' else 'FAIL'
        results['persisted_event_rows'] = db.execute('select count(*) from events').fetchone()[0]
    errors = []
    for log in [out/'master.log', out/'master_restart.log']:
        errors += [line for line in log.read_text().splitlines() if any(t in line for t in ['TypeError','ReferenceError','Binding loop','binding loop','undefined','QThread: Destroyed','persistence incomplete'])]
    (out/'qml-errors.json').write_text(json.dumps(errors, indent=2))
    results['qml_runtime_errors'] = 'PASS' if not errors else 'FAIL'
    if errors:
        raise AssertionError('QML/runtime errors recorded')
    results['physical_windows_ntp'] = 'PENDING_EXTERNAL_TEST'
except Exception as error:
    results['error'] = str(error)
finally:
    for p in reversed(children):
        stop(p)
    for log in logs:
        log.close()
    (out/'resource-usage.json').write_text(json.dumps(usage, indent=2))
    (out/'results.json').write_text(json.dumps(results, indent=2))
    (out/'runtime-library-maps.json').write_text(json.dumps(runtime_library_maps, indent=2))
    print(json.dumps(results, indent=2))
raise SystemExit(1 if 'error' in results or 'FAIL' in results.values() else 0)
