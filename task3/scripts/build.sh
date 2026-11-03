#!/usr/bin/env bash
set -euo pipefail
task3_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
source "$task3_root/task3/scripts/qt_prefix.sh"
task3_qt="$(task3_default_qt_prefix)"
task3_dds="${TASK3_DDS_PREFIX:-/home/dev/dds-deps/install}"
task3_check_qt_modules "$task3_qt" || exit 2
cmake -S "$task3_root" -B "$task3_root/build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$task3_qt;$task3_dds" \
    -DTASK3_WEB_UI="${TASK3_WEB_UI:-ON}"
cmake --build "$task3_root/build" -j "${TASK3_BUILD_JOBS:-4}"
export LD_LIBRARY_PATH="$task3_dds/lib:$task3_qt/lib:${LD_LIBRARY_PATH:-}"
ctest --test-dir "$task3_root/build" --output-on-failure
