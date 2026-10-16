#!/usr/bin/env bash
set -euo pipefail

task3_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "$task3_root/task3/scripts/patched_runtime_common.sh"
task3_prepare_environment
task3_require_key

if [[ -n "${TASK3_AGENT_BIN:-}" ]]; then
    agent_bin="$TASK3_AGENT_BIN"
else
    latest_file="$task3_root/.task3-deps/task3-builds/LATEST"
    [[ -f "$latest_file" ]] || task3_die 'no patched Task 3 build found; run task3/scripts/build_task3_patched.sh'
    latest_build="$(<"$latest_file")"
    agent_bin="$latest_build/release/task3/task3_agent"
fi
task3_verify_binary_linkage "$agent_bin" no

if (($#)); then
    agent_args=("$@")
else
    agent_args=(--config "${TASK3_AGENT_CONFIG:-$task3_root/task3/config/linux-agent.json}")
fi
printf 'Launching Linux Agent; Fast DDS resolves from %s. The Agent does not link the Statistics Backend directly.\n' "$task3_dds_prefix"
task3_run_and_verify_maps "$agent_bin" no "${agent_args[@]}"
