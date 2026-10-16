#!/usr/bin/env bash
set -euo pipefail

task3_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "$task3_root/task3/scripts/patched_runtime_common.sh"
task3_prepare_environment
if [[ -n "${TASK3_MASTER_BIN:-}" ]]; then
    master_bin="$TASK3_MASTER_BIN"
else
    latest_file="$task3_root/.task3-deps/task3-builds/LATEST"
    [[ -f "$latest_file" ]] || task3_die 'no patched Task 3 build found; run task3/scripts/build_task3_patched.sh'
    latest_build="$(<"$latest_file")"
    master_bin="$latest_build/release/dds_monitor"
fi
task3_verify_binary_linkage "$master_bin" yes

export TASK3_TELEMETRY_DOMAIN="${TASK3_TELEMETRY_DOMAIN:-42}"
export TASK3_TELEMETRY_TOPIC="${TASK3_TELEMETRY_TOPIC:-Task3/HostTelemetry/v1}"
export TASK3_TIMEOUT_MS="${TASK3_TIMEOUT_MS:-5000}"
export TASK3_DATA_DIR="${TASK3_DATA_DIR:-$task3_root/.task3-runtime/manual-master}"
export TASK3_FLOW_RULES="${TASK3_FLOW_RULES:-$task3_root/task3/config/flow-rules.json}"
mkdir -p "$TASK3_DATA_DIR"
printf 'Launching Master with pinned Statistics Backend %s.\n' "$task3_expected_version"
task3_run_and_verify_maps "$master_bin" yes "$@"
