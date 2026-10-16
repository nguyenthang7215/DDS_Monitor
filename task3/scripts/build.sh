#!/usr/bin/env bash
set -euo pipefail
task3_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
task3_qt="${TASK3_QT_PREFIX:-/home/dev/Qt/6.7.2/gcc_64}"
task3_dds="${TASK3_DDS_PREFIX:-/home/dev/dds-deps/install}"
cmake -S "$task3_root" -B "$task3_root/build" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$task3_qt;$task3_dds"
cmake --build "$task3_root/build" -j "${TASK3_BUILD_JOBS:-4}"
export LD_LIBRARY_PATH="$task3_dds/lib:$task3_qt/lib:${LD_LIBRARY_PATH:-}"
ctest --test-dir "$task3_root/build" --output-on-failure
