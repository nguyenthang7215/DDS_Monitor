#!/usr/bin/env bash
set -euo pipefail

task3_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
backend_script="$task3_root/task3/scripts/build_statistics_backend_patched.sh"
qt_prefix="${TASK3_QT_PREFIX:-/home/dev/Qt/6.7.2/gcc_64}"
dds_prefix="${TASK3_DDS_PREFIX:-/home/dev/dds-deps/install}"
backend_prefix="${TASK3_PATCHED_BACKEND_PREFIX:-$task3_root/.task3-deps/fastdds-statistics-backend-2.5.0-patched}"
jobs="${TASK3_BUILD_JOBS:-4}"

usage() {
    cat <<'EOF'
Usage: build_task3_patched.sh [--qt-prefix DIR] [--dds-prefix DIR]
       [--backend-prefix DIR] [--jobs N]

Builds the pinned patched Backend, then clean Task 3 Debug and Release trees,
and runs CTest in both. Build trees and the install prefix are Git-ignored.
EOF
}

while (($#)); do
    case "$1" in
        --qt-prefix) qt_prefix="$2"; shift 2 ;;
        --dds-prefix) dds_prefix="$2"; shift 2 ;;
        --backend-prefix) backend_prefix="$2"; shift 2 ;;
        --jobs) jobs="$2"; shift 2 ;;
        --help|-h) usage; exit 0 ;;
        *) printf 'Unknown argument: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

[[ -d "$qt_prefix" && -d "$dds_prefix" ]] || { printf 'Qt or DDS dependency prefix does not exist.\n' >&2; exit 2; }
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || { printf 'Invalid --jobs value: %s\n' "$jobs" >&2; exit 2; }
[[ -f "$qt_prefix/lib/cmake/Qt6/Qt6Config.cmake" ]] || { printf 'Qt6 CMake package missing: %s\n' "$qt_prefix" >&2; exit 2; }

export TASK3_QT_PREFIX="$qt_prefix"
export TASK3_DDS_PREFIX="$dds_prefix"
export TASK3_PATCHED_BACKEND_PREFIX="$backend_prefix"
export TASK3_BUILD_JOBS="$jobs"
bash "$backend_script" --prefix "$backend_prefix" --dds-prefix "$dds_prefix" --jobs "$jobs"

source "$task3_root/task3/scripts/patched_runtime_common.sh"
task3_prepare_environment

build_root="$task3_root/.task3-deps/task3-builds"
mkdir -p "$build_root"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$build_root/$run_id"
mkdir "$run_dir" || { printf 'Build run directory already exists: %s\n' "$run_dir" >&2; exit 2; }

for config in Debug Release; do
    build_dir="$run_dir/${config,,}"
    printf '\n=== Clean Task 3 %s build: %s ===\n' "$config" "$build_dir"
    cmake -S "$task3_root" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE="$config" \
        -DCMAKE_PREFIX_PATH="$qt_prefix;$backend_prefix;$dds_prefix" \
        -DQT_PATH="$qt_prefix" \
        -Dfastdds_statistics_backend_DIR="$backend_prefix/lib/cmake/fastdds_statistics_backend" \
        -Dfastdds_DIR="$dds_prefix/share/fastdds/cmake" \
        -Dfastcdr_DIR="$dds_prefix/lib/cmake/fastcdr" \
        -DTASK3_BUILD_TESTS=ON \
        -DBUILD_TESTS=OFF \
        -DCOMPLEX_MOCK=OFF \
        -DSTATIC_MOCK=OFF
    cmake --build "$build_dir" --parallel "$jobs"
    task3_verify_binary_linkage "$build_dir/dds_monitor" yes
    task3_verify_binary_linkage "$build_dir/task3/task3_agent" no
    (cd "$build_dir" && ctest --output-on-failure) | tee "$run_dir/${config,,}-ctest.log"
done

printf '%s\n' "$run_dir" > "$build_root/LATEST.tmp.$$"
mv "$build_root/LATEST.tmp.$$" "$build_root/LATEST"
printf '\nTASK3_BUILD_ROOT=%s\n' "$run_dir"
printf 'DEBUG_MASTER=%s/debug/dds_monitor\n' "$run_dir"
printf 'DEBUG_AGENT=%s/debug/task3/task3_agent\n' "$run_dir"
printf 'RELEASE_MASTER=%s/release/dds_monitor\n' "$run_dir"
printf 'RELEASE_AGENT=%s/release/task3/task3_agent\n' "$run_dir"
