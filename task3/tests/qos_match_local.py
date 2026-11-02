#!/usr/bin/env python3
"""E2E-12 local pre-check: run Fast DDS HelloWorld Publisher/Subscriber pairs with explicit XML profiles
and record whether the endpoints match and samples arrive.

The both-BEST_EFFORT control proves the XML is really loaded: if profiles were ignored every pair would
match with the example's default RELIABLE QoS, and the BEST_EFFORT writer case could not differ."""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import time

parser = argparse.ArgumentParser()
parser.add_argument('--hello', required=True, help='hello_world executable')
parser.add_argument('--case', action='append', required=True,
                    help='name=profile.xml=expect where expect is match or nomatch')
parser.add_argument('--output', required=True)
parser.add_argument('--window', type=float, default=8.0)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=False)
results = []


def run(role, env, log):
    return subprocess.Popen([args.hello, role], env=env, stdout=log, stderr=subprocess.STDOUT,
                            cwd=str(out))


for spec in args.case:
    name, profile, expect = spec.split('=')
    env = os.environ.copy()
    env['FASTDDS_DEFAULT_PROFILES_FILE'] = str(Path(profile).resolve(strict=True))
    env.pop('FASTDDS_STATISTICS', None)
    sub_log = (out / f'{name}-subscriber.log').open('w')
    pub_log = (out / f'{name}-publisher.log').open('w')
    sub = run('subscriber', env, sub_log)
    time.sleep(1.0)
    pub = run('publisher', env, pub_log)
    time.sleep(args.window)
    for process in (pub, sub):
        if process.poll() is None:
            process.send_signal(signal.SIGINT)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
    sub_log.close()
    pub_log.close()
    sub_text = (out / f'{name}-subscriber.log').read_text(errors='replace')
    pub_text = (out / f'{name}-publisher.log').read_text(errors='replace')
    observed = {
        'publisher_matched': 'Publisher matched' in pub_text,
        'subscriber_matched': 'Subscriber matched' in sub_text,
        'sent': pub_text.count('SENT'),
        'received': sub_text.count('RECEIVED'),
    }
    matched = observed['publisher_matched'] and observed['subscriber_matched'] and observed['received'] > 0
    nomatch = not observed['publisher_matched'] and not observed['subscriber_matched'] and observed['received'] == 0
    status = 'PASS' if (matched if expect == 'match' else nomatch) else 'FAIL'
    results.append({'case': name, 'profile': str(Path(profile).resolve()), 'expect': expect,
                    'observed': observed, 'status': status})

summary = {'status': 'PASS' if all(r['status'] == 'PASS' for r in results) else 'FAIL',
           'window_s': args.window, 'cases': results}
(out / 'result.json').write_text(json.dumps(summary, indent=2) + '\n')
print(json.dumps(summary, indent=2))
raise SystemExit(0 if summary['status'] == 'PASS' else 1)
