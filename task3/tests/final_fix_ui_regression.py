#!/usr/bin/env python3
"""Exercise the real QML status label with an isolated corrupt event database.

No user's database or OS configuration is modified. Only owned processes are stopped.
"""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import time

from runtime_library_verification import verify_statistics_backend_mapped

cli = argparse.ArgumentParser()
cli.add_argument('--build', required=True)
cli.add_argument('--output', required=True)
args = cli.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=False)
state = out / 'state'
state.mkdir()
(state / 'events.sqlite').write_bytes(b'Intentional corrupt database for P1-01 UI regression')
env = dict(os.environ, QT_QPA_PLATFORM='offscreen', QSG_RHI_BACKEND='software',
           TASK3_DATA_DIR=str(state), TASK3_AUTO_DOMAIN='off', TASK3_TELEMETRY_DOMAIN='93',
           TASK3_EXPECTED_HOSTS='status-regression-host', TASK3_SNAPSHOT_PATH=str(out/'snapshot.json'),
           TASK3_SCREENSHOT_PATH=str(out/'database-error.png'), TASK3_EXIT_AFTER_MS='20000')
results = {}
master = None
try:
    with (out/'master.log').open('w') as log:
        master = subprocess.Popen([str(Path(args.build).resolve()/'dds_monitor')],
                                  env=env, stdout=log, stderr=subprocess.STDOUT)
        mapping = verify_statistics_backend_mapped(master.pid, env.get('TASK3_PATCHED_BACKEND_PREFIX'))
        (out/'runtime-library-maps.json').write_text(json.dumps(mapping, indent=2))
        deadline = time.monotonic() + 10
        snapshot = {}
        while time.monotonic() < deadline:
            assert master.poll() is None, 'Master exited before UI verification'
            try:
                snapshot = json.loads((out/'snapshot.json').read_text())
            except (OSError, ValueError):
                time.sleep(.1)
                continue
            if snapshot.get('ui', {}).get('dashboard_loaded') and (out/'database-error.png').exists():
                break
            time.sleep(.1)
        assert 'Event database:' in snapshot.get('status', ''), 'Database error was not reproduced in core'
        results['corrupt_database_reproduced'] = 'PASS'
        ui = snapshot.get('ui', {})
        assert ui.get('status_visible'), 'Actual QML status label is hidden'
        assert 'Event database:' in ui.get('status_text', ''), 'Actual QML label hides the database error'
        assert 'DDS telemetry domain' not in ui['status_text'], 'Technical topic prefix should stay hidden'
        assert 'Task3/HostTelemetry/v1' not in ui['status_text'], 'Technical topic should stay hidden'
        results['database_error_visible_on_real_qml_label'] = 'PASS'
        assert (out/'database-error.png').exists(), 'Missing actual UI screenshot'
        master.send_signal(signal.SIGTERM)
        master.wait(timeout=10)
        assert master.returncode == 0, 'Unclean Master shutdown'
        results['master_shutdown'] = 'PASS'
except Exception as error:
    results['error'] = str(error)
finally:
    if master is not None and master.poll() is None:
        master.terminate()
        try:
            master.wait(timeout=10)
        except subprocess.TimeoutExpired:
            master.kill()
            master.wait()
    (out/'results.json').write_text(json.dumps(results, indent=2))
    print(json.dumps(results, indent=2))
raise SystemExit(1 if 'error' in results else 0)
