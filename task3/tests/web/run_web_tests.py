#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run task3/web in a headless Chromium browser against recorded MonitoringCore snapshots.

Needs only Python 3 and Chrome, Chromium or Edge, so the page can be checked on a machine without
Qt or Fast DDS. It does not replace running dds_monitor (QtWebEngine, QWebChannel, live Agent data):
the page is the real task3/web/index.html, with qrc:///qtwebchannel/qwebchannel.js replaced by
mock_channel.js and file: added to the Content-Security-Policy so the local files may load.

  python3 task3/tests/web/run_web_tests.py --output test-artifacts/<dir>
"""
import argparse
import html
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[3]
WEB = ROOT / "task3" / "web"
HERE = pathlib.Path(__file__).resolve().parent
FIXTURES = ROOT / "test-artifacts" / "final-fix-20261019" / "ui-1600"
SCENARIOS = ["overview", "resources", "network", "apps", "flows", "time", "offline", "events"]


def find_browser(explicit):
    candidates = [explicit, os.environ.get("TASK3_BROWSER"),
                  r"C:\Program Files\Google\Chrome\Application\chrome.exe",
                  r"C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
                  r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
                  r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"]
    candidates += [shutil.which(name) for name in
                   ["google-chrome", "google-chrome-stable", "chromium", "chromium-browser", "microsoft-edge", "msedge"]]
    for candidate in candidates:
        if candidate and pathlib.Path(candidate).is_file():
            return candidate
    sys.exit("No Chrome/Chromium/Edge found; pass --browser or set TASK3_BROWSER")


def replace_once(text, old, new):
    if text.count(old) != 1:
        sys.exit(f"index.html changed: expected exactly one {old!r}")
    return text.replace(old, new)


def build_page(out):
    fixtures = {
        "mapped": "mapped_application_and_flow.json",
        "offline": "offline_unverified_alert.json",
        "recovered": "recovered_event_page.json",
    }
    data = {key: json.loads((FIXTURES / name).read_text(encoding="utf-8")) for key, name in fixtures.items()}
    (out / "fixtures.js").write_text("window.TASK3_FIXTURES = " + json.dumps(data, ensure_ascii=False) + ";\n",
                                     encoding="utf-8")
    page = (WEB / "index.html").read_text(encoding="utf-8")
    url = lambda path: pathlib.Path(path).resolve().as_uri()
    page = replace_once(page, '<script src="qrc:///qtwebchannel/qwebchannel.js"></script>',
                        f'<script src="{url(out / "fixtures.js")}"></script>'
                        f'<script src="{url(HERE / "mock_channel.js")}"></script>')
    page = replace_once(page, '<link rel="stylesheet" href="style.css">',
                        f'<link rel="stylesheet" href="{url(WEB / "style.css")}">')
    page = replace_once(page, '<script src="app.js" defer></script>',
                        f'<script src="{url(WEB / "app.js")}" defer></script>'
                        f'<script src="{url(HERE / "perf.js")}" defer></script>'
                        f'<script src="{url(HERE / "selftest.js")}" defer></script>')
    page = replace_once(page, "script-src qrc:;", "script-src qrc: file:;")
    page = replace_once(page, "style-src qrc: ", "style-src qrc: file: ")
    target = out / "page.html"
    target.write_text(page, encoding="utf-8")
    return target


def run_browser(browser, profile, args, timeout):
    command = [browser, "--headless=new", "--disable-gpu", "--no-first-run", "--no-default-browser-check",
               "--disable-extensions", "--allow-file-access-from-files", "--hide-scrollbars",
               f"--user-data-dir={profile}", "--window-size=1600,806"] + args
    return subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace",
                          timeout=timeout)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--output", required=True, help="Directory for results.json, page.html and screenshots")
    parser.add_argument("--browser", help="Chrome/Chromium/Edge executable")
    parser.add_argument("--no-screenshots", action="store_true")
    args = parser.parse_args()
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")  # Vietnamese test names on Windows consoles
    out = pathlib.Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    browser = find_browser(args.browser)
    page = build_page(out)
    with tempfile.TemporaryDirectory(prefix="task3-web-") as profile:
        run = run_browser(browser, profile, ["--virtual-time-budget=120000", "--dump-dom",
                                             page.as_uri() + "?mode=test"], 300)
        match = re.search(r'<pre id="task3-web-result"[^>]*>(.*?)</pre>', run.stdout, re.S)
        if not match:
            (out / "browser-stderr.txt").write_text(run.stderr, encoding="utf-8")
            sys.exit("Self-test did not finish; see browser-stderr.txt")
        result = json.loads(html.unescape(match.group(1)))
        result["browser"] = browser
        # Wall-clock render cost: no virtual time, the page measures synchronously while loading.
        perf = run_browser(browser, profile, ["--dump-dom", page.as_uri() + "?mode=perf"], 120)
        perf_match = re.search(r'<pre id="task3-web-result"[^>]*>(.*?)</pre>', perf.stdout, re.S)
        result["performance"] = json.loads(html.unescape(perf_match.group(1))) if perf_match else None
        shots = []
        if not args.no_screenshots:
            for scenario in SCENARIOS:
                png = out / f"{scenario}-web.png"
                run_browser(browser, profile, ["--virtual-time-budget=8000", f"--screenshot={png}",
                                               page.as_uri() + f"?mode=shot&scenario={scenario}"], 120)
                shots.append({"scenario": scenario, "file": png.name, "written": png.is_file()})
        result["screenshots"] = shots
    (out / "results.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    for item in result["results"]:
        print(("PASS " if item["ok"] else "FAIL ") + item["name"] + ("" if item["ok"] else ": " + item["error"]))
    print(f"{result['passed']}/{result['total']} passed · performance {json.dumps(result['performance'])}")
    for shot in shots:
        print(f"screenshot {shot['file']}: {'ok' if shot['written'] else 'MISSING'}")
    ok = result["passed"] == result["total"] and result["performance"] and all(s["written"] for s in shots)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
