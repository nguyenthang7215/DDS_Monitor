#!/usr/bin/env python3
"""Real local Agent + extended DDS Monitor + existing Fast DDS Hello World.
No physical multi-host or Windows claims. Only spawned child processes are stopped.
"""
import argparse
import json
import os
from pathlib import Path
import signal
import subprocess
import tempfile
import time
from runtime_library_verification import verify_statistics_backend_mapped

parser = argparse.ArgumentParser()
parser.add_argument("--build", required=True)
parser.add_argument("--hello-world", required=True)
parser.add_argument("--output", required=True)
parser.add_argument("--debug-master", action="store_true")
args = parser.parse_args()
build = Path(args.build).resolve()
output = Path(args.output).resolve()
output.mkdir(parents=True, exist_ok=True)
results = {}
children = []
streams = []
runtime_library_maps = []

def start(name, argv, env):
    log = (output / (name + ".log")).open("w")
    streams.append(log)
    process = subprocess.Popen(argv, env=env, stdout=log, stderr=subprocess.STDOUT)
    children.append(process)
    if name == "master" and not args.debug_master:
        runtime_library_maps.append(verify_statistics_backend_mapped(
            process.pid, env.get("TASK3_PATCHED_BACKEND_PREFIX")))
    return process

def stop(process):
    if process.poll() is None:
        process.send_signal(signal.SIGINT)
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

def snapshot():
    try:
        return json.loads((output / "snapshot.json").read_text())
    except (FileNotFoundError, json.JSONDecodeError):
        return {}

def wait_for(name, predicate, timeout=15):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        data = snapshot()
        if predicate(data):
            results[name] = "PASS"
            (output / (name + ".json")).write_text(json.dumps(data, indent=2))
            return data
        if master.poll() is not None:
            raise RuntimeError("Master exited: " + str(master.returncode))
        time.sleep(0.2)
    results[name] = "FAIL"
    raise RuntimeError("Timed out: " + name)

env = os.environ.copy()
env.update(QT_QPA_PLATFORM="offscreen", QSG_RHI_BACKEND="software",
           TASK3_KEY="integration-test-key-not-for-deployment",
           TASK3_DATA_DIR=str(output / "state"),
           TASK3_SNAPSHOT_PATH=str(output / "snapshot.json"),
           TASK3_SCREENSHOT_PATH=str(output / "dashboard.png"),
           TASK3_TIMEOUT_MS="1800", TASK3_TELEMETRY_DOMAIN="96",
           TASK3_EXPECTED_TOPICS="hello_world_topic", TASK3_DDS_TIMEOUT_MS="3000",
           TASK3_EXIT_AFTER_MS="110000")
config = {
    "host_id": "local-real-agent", "telemetry_domain": 96, "interval_ms": 300,
    "disk_paths": ["/", "/tmp"],
    "applications": [{"application_id": "hello-world", "process_name": "hello_world",
                      "participant_guids": []},
                     {"application_id": "missing-test", "process_name": "task3-no-such-process",
                      "service": "task3-nonexistent-test.service"}]
}
config_path = output / "agent.json"
config_path.write_text(json.dumps(config))
try:
    master_command = [str(build / "dds_monitor")]
    if args.debug_master:
        master_command = ["gdb", "--batch", "-ex", "run", "-ex", "bt", "--args"] + master_command
    master = start("master", master_command, env)
    agent = start("agent", [str(build / "task3" / "task3_agent"), "--config", str(config_path)], env)
    first = wait_for("telemetry_real", lambda d: len(d.get("hosts", [])) == 1 and d["hosts"][0]["online"])
    results["master_launch"] = "PASS"
    wait_for("metrics_real", lambda d: bool(d.get("hosts")) and
             d["hosts"][0]["metrics"]["ram_total"] > 0 and
             d["hosts"][0]["metrics"]["cpu_percent"] is not None and
             len(d["hosts"][0]["metrics"].get("interfaces", [])) > 0)
    wait_for("process_stopped", lambda d: any(e["kind"] == "process_stopped" and e["state"] == "Active"
             for e in d.get("events", [])))
    stop(agent)
    wait_for("agent_timeout", lambda d: bool(d.get("hosts")) and not d["hosts"][0]["online"])
    agent = start("agent_reconnect", [str(build / "task3" / "task3_agent"), "--config", str(config_path)], env)
    wait_for("agent_recovery", lambda d: bool(d.get("hosts")) and d["hosts"][0]["online"] and
             any(e["kind"] == "agent_timeout" and e["state"] == "Recovered" for e in d.get("events", [])))

    stats_env = env.copy()
    stats_env["FASTDDS_STATISTICS"] = "HISTORY_LATENCY_TOPIC;NETWORK_LATENCY_TOPIC;PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;PHYSICAL_DATA_TOPIC;DISCOVERY_TOPIC"
    sub = start("subscriber", [args.hello_world, "subscriber"], stats_env)
    pub = start("publisher", [args.hello_world, "publisher"], stats_env)
    wait_for("dds_discovery", lambda d: {"Participant", "Topic", "Writer", "Reader"}.issubset(
             {e.get("kind") for e in d.get("entities", [])}), timeout=20)
    measured = wait_for("dds_throughput", lambda d: any(
        e.get("kind") == "Writer" and e.get("topic") == "hello_world_topic" and
        e.get("throughput_bps", 0) > 0 for e in d.get("entities", [])), timeout=45)
    wait_for("dds_subscription_throughput", lambda d: any(
        e.get("kind") == "Reader" and e.get("topic") == "hello_world_topic" and
        e.get("throughput_bps", 0) > 0 for e in d.get("entities", [])), timeout=30)
    wait_for("dds_latency", lambda d: any(link.get("latency_us") is not None
        for e in d.get("entities", []) for link in e.get("links", [])), timeout=25)
    old_guids = {e["guid"] for e in measured.get("entities", []) if e.get("kind") == "Writer"}
    stop(pub)
    wait_for("dds_no_data_rule", lambda d: any(e["kind"] == "dds_no_data" and e["state"] == "Active"
             for e in d.get("events", [])), timeout=15)
    pub = start("publisher_restart", [args.hello_world, "publisher"], stats_env)
    wait_for("dds_restart_discovered", lambda d: any(
        e.get("kind") == "Writer" and e.get("alive") and e.get("guid") not in old_guids
        for e in d.get("entities", [])), timeout=20)
    wait_for("dds_no_data_recovery", lambda d: any(
        e["kind"] == "dds_no_data" and e["state"] == "Recovered"
        for e in d.get("events", [])), timeout=35)
    results["ui_snapshot_event_loop"] = "PASS"
    results["windows_build"] = "PENDING_EXTERNAL_TEST"
    results["linux_windows_physical"] = "PENDING_EXTERNAL_TEST"
    results["ntp_two_machine"] = "PENDING_EXTERNAL_TEST"
except Exception as error:
    results["error"] = str(error)
finally:
    for process in reversed(children):
        stop(process)
    for stream in streams:
        stream.close()
    (output / "results.json").write_text(json.dumps(results, indent=2))
    (output / "runtime-library-maps.json").write_text(json.dumps(runtime_library_maps, indent=2))
    print(json.dumps(results, indent=2))
if "error" in results:
    raise SystemExit(1)
