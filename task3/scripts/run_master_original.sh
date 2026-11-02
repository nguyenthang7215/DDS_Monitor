#!/usr/bin/env bash
# Launch the Task 3 Master against the unpatched Statistics Backend 2.5.0 built from clean v2.5.0 source.
# The binary's RUNPATH still names the patched prefix; LD_LIBRARY_PATH takes precedence and /proc/<pid>/maps
# is checked so the run never silently falls back to the patched library.
set -euo pipefail

task3_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
original_prefix="$(realpath -m "${TASK3_ORIGINAL_BACKEND_PREFIX:-/home/dev/dds-e2e/backend-original-install}")"
patched_prefix="$(realpath -m "${TASK3_PATCHED_BACKEND_PREFIX:-$task3_root/.task3-deps/fastdds-statistics-backend-2.5.0-patched}")"
dds_prefix="$(realpath -m "${TASK3_DDS_PREFIX:-/home/dev/dds-deps/install}")"
qt_prefix="$(realpath -m "${TASK3_QT_PREFIX:-/home/dev/Qt/6.7.2/gcc_64}")"
backend_lib="$original_prefix/lib/libfastdds_statistics_backend.so.2.5.0"

die() {
    printf 'Original runtime check failed: %s\n' "$*" >&2
    exit 1
}

[[ -f "$backend_lib" ]] || die "original Backend library is missing: $backend_lib"
[[ -d "$dds_prefix/lib" && -d "$qt_prefix/lib" ]] || die 'Qt or DDS dependency prefix is missing'
if [[ -f "$patched_prefix/lib/libfastdds_statistics_backend.so.2.5.0" ]] &&
    cmp -s "$backend_lib" "$patched_prefix/lib/libfastdds_statistics_backend.so.2.5.0"; then
    die "$backend_lib is byte-identical to the patched library"
fi
if [[ "${LD_PRELOAD:-}" == *libfastdds_statistics_backend* ]]; then
    die 'LD_PRELOAD names a Statistics Backend; remove it so the original library is unambiguous'
fi

if [[ -n "${TASK3_MASTER_BIN:-}" ]]; then
    master_bin="$TASK3_MASTER_BIN"
else
    latest_file="$task3_root/.task3-deps/task3-builds/LATEST"
    [[ -f "$latest_file" ]] || die 'no Task 3 build found; run task3/scripts/build_task3_patched.sh'
    master_bin="$(<"$latest_file")/${TASK3_BUILD_TYPE:-release}/dds_monitor"
fi
[[ -x "$master_bin" ]] || die "executable not found or not executable: $master_bin"

export LD_LIBRARY_PATH="$original_prefix/lib:$dds_prefix/lib:$qt_prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
resolved="$(ldd "$master_bin" | awk '$1 ~ /^libfastdds_statistics_backend\.so/ {print $3}')"
[[ "$(realpath "$resolved")" == "$(realpath "$backend_lib")" ]] || die "ldd resolves Statistics Backend to $resolved"

export TASK3_TELEMETRY_DOMAIN="${TASK3_TELEMETRY_DOMAIN:-42}"
export TASK3_TELEMETRY_TOPIC="${TASK3_TELEMETRY_TOPIC:-Task3/HostTelemetry/v1}"
export TASK3_TIMEOUT_MS="${TASK3_TIMEOUT_MS:-5000}"
export TASK3_DATA_DIR="${TASK3_DATA_DIR:-$task3_root/.task3-runtime/manual-master-original}"
export TASK3_FLOW_RULES="${TASK3_FLOW_RULES:-$task3_root/task3/config/flow-rules.json}"
mkdir -p "$TASK3_DATA_DIR"

printf 'Launching %s with ORIGINAL Statistics Backend %s\n' "$master_bin" "$backend_lib"
"$master_bin" "$@" &
pid=$!
trap 'kill -INT "$pid" 2>/dev/null || true; wait "$pid" || true; exit 130' INT
trap 'kill -TERM "$pid" 2>/dev/null || true; wait "$pid" || true; exit 143' TERM

for _ in $(seq 1 100); do
    if [[ -r "/proc/$pid/maps" ]] && grep -Fq "$(realpath "$backend_lib")" "/proc/$pid/maps"; then
        if grep -Fq "$patched_prefix/lib/libfastdds_statistics_backend" "/proc/$pid/maps"; then
            kill -TERM "$pid" 2>/dev/null || true
            die "process $pid also maps the patched Backend"
        fi
        printf 'Verified process %s maps %s\n' "$pid" "$(realpath "$backend_lib")"
        wait "$pid"
        exit $?
    fi
    kill -0 "$pid" 2>/dev/null || { wait "$pid" || true; die 'process exited before the Backend was mapped'; }
    sleep 0.1
done
kill -TERM "$pid" 2>/dev/null || true
die "runtime library mapping was not verified for process $pid"
