#!/usr/bin/env bash

task3_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
task3_expected_commit="314fcd6e9d888077497d815406fcf134c6754759"
task3_expected_version="2.5.0"
task3_expected_patch_sha="83019a4479ec8b05cdab0a2d0e97461d9e3d386bcda865ba5f89fbf02e383a9a"
task3_patch_file="$task3_root/test-artifacts/p0-crash-20261016/Fast-DDS-Statistics-Backend-2.5.0-database-queue-lifecycle.patch"
task3_patched_prefix="$(realpath -m "${TASK3_PATCHED_BACKEND_PREFIX:-$task3_root/.task3-deps/fastdds-statistics-backend-$task3_expected_version-patched}")"
task3_dds_prefix="$(realpath -m "${TASK3_DDS_PREFIX:-/home/dev/dds-deps/install}")"
task3_qt_prefix="$(realpath -m "${TASK3_QT_PREFIX:-/home/dev/Qt/6.7.2/gcc_64}")"

task3_die() {
    printf 'Task 3 runtime check failed: %s\n' "$*" >&2
    exit 1
}

task3_verify_patched_install() {
    local manifest="$task3_patched_prefix/share/task3-patched-backend/MANIFEST"
    local library="$task3_patched_prefix/lib/libfastdds_statistics_backend.so.$task3_expected_version"
    local actual_sha
    [[ -f "$manifest" ]] || task3_die "patched Backend manifest is missing; run task3/scripts/build_statistics_backend_patched.sh first"
    [[ -f "$library" ]] || task3_die "patched Backend library is missing: $library"
    grep -Fxq "version=$task3_expected_version" "$manifest" || task3_die "manifest version is not $task3_expected_version"
    grep -Fxq "source_commit=$task3_expected_commit" "$manifest" || task3_die 'manifest source commit is not the pinned v2.5.0 commit'
    grep -Fxq "patch_sha256=$task3_expected_patch_sha" "$manifest" || task3_die 'manifest patch checksum does not match the reviewed patch'
    [[ -f "$task3_patch_file" ]] || task3_die "pinned patch is missing: $task3_patch_file"
    [[ "$(sha256sum "$task3_patch_file" | awk '{print $1}')" == "$task3_expected_patch_sha" ]] || task3_die 'working-tree patch checksum differs from the pinned patch'
    actual_sha="$(sha256sum "$library" | awk '{print $1}')"
    grep -Fxq "library_sha256=$actual_sha" "$manifest" || task3_die 'installed Backend library hash differs from the build manifest'
    [[ -d "$task3_dds_prefix" && -d "$task3_qt_prefix" ]] || task3_die 'Qt or DDS dependency prefix is missing'
}

task3_prepare_environment() {
    task3_verify_patched_install
    if [[ "${LD_PRELOAD:-}" == *libfastdds_statistics_backend* ]]; then
        task3_die 'LD_PRELOAD names a Statistics Backend; remove it so the pinned library is unambiguous'
    fi
    export LD_LIBRARY_PATH="$task3_patched_prefix/lib:$task3_dds_prefix/lib:$task3_qt_prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
}

task3_verify_binary_linkage() {
    local binary="$1"
    local require_backend="${2:-yes}"
    local output backend_path fastdds_path fastcdr_path
    [[ -x "$binary" ]] || task3_die "executable not found or not executable: $binary"
    output="$(ldd "$binary")" || task3_die "ldd failed for $binary"
    if grep -q 'not found' <<<"$output"; then
        printf '%s\n' "$output" >&2
        task3_die "one or more shared-library dependencies are unresolved for $binary"
    fi
    backend_path="$(awk '$1 ~ /^libfastdds_statistics_backend\.so/ {print $3; count++} END {if (count != 1) exit 1}' <<<"$output")" || backend_path=""
    if [[ "$require_backend" == yes ]]; then
        [[ -n "$backend_path" ]] || task3_die "$binary does not link exactly one Statistics Backend library"
        [[ "$(realpath "$backend_path")" == "$task3_patched_prefix/lib/libfastdds_statistics_backend.so.$task3_expected_version" ]] || {
            task3_die "$binary resolves Statistics Backend outside the pinned prefix: $backend_path"
        }
    elif [[ -n "$backend_path" ]]; then
        [[ "$(realpath "$backend_path")" == "$task3_patched_prefix/lib/libfastdds_statistics_backend.so.$task3_expected_version" ]] || {
            task3_die "$binary resolves Statistics Backend outside the pinned prefix: $backend_path"
        }
    fi
    fastdds_path="$(awk '$1 == "libfastdds.so.3.6" {print $3; exit}' <<<"$output")"
    fastcdr_path="$(awk '$1 == "libfastcdr.so.2" {print $3; exit}' <<<"$output")"
    [[ -n "$fastdds_path" && "$(realpath "$fastdds_path")" == "$(realpath "$task3_dds_prefix/lib/libfastdds.so.3.6")" ]] || task3_die "$binary does not resolve Fast DDS 3.6.1 from $task3_dds_prefix"
    [[ -n "$fastcdr_path" && "$(realpath "$fastcdr_path")" == "$(realpath "$task3_dds_prefix/lib/libfastcdr.so.2")" ]] || task3_die "$binary does not resolve Fast CDR from $task3_dds_prefix"
}

task3_require_key() {
    if [[ -z "${TASK3_KEY:-}" ]]; then
        read -r -s -p 'Nhập TASK3_KEY (ít nhất 16 byte; không hiển thị): ' TASK3_KEY || task3_die 'cannot read TASK3_KEY from this terminal'
        printf '\n'
    fi
    [[ "$(LC_ALL=C printf '%s' "$TASK3_KEY" | wc -c | tr -d ' ')" -ge 16 ]] || task3_die 'TASK3_KEY must contain at least 16 bytes'
    export TASK3_KEY
}

task3_forward_signal() {
    local signal_name="$1"
    kill -s "$signal_name" "$pid" 2>/dev/null || true
    wait "$pid" || true
    if [[ "$signal_name" == INT ]]; then
        exit 130
    fi
    exit 143
}

task3_run_and_verify_maps() {
    local binary="$1"
    local require_backend="$2"
    shift 2
    task3_verify_binary_linkage "$binary" "$require_backend"
    "$binary" "$@" &
    local pid=$!
    trap 'task3_forward_signal INT' INT
    trap 'task3_forward_signal TERM' TERM
    local expected_map="$(realpath "$task3_dds_prefix/lib/libfastdds.so.3.6")"
    if [[ "$require_backend" == yes ]]; then
        expected_map="$(realpath "$task3_patched_prefix/lib/libfastdds_statistics_backend.so.$task3_expected_version")"
    fi
    local attempt
    for attempt in $(seq 1 100); do
        if [[ -r "/proc/$pid/maps" ]] && grep -Fq "$expected_map" "/proc/$pid/maps"; then
            printf 'Verified running process %s maps %s\n' "$pid" "$expected_map"
            if wait "$pid"; then
                trap - INT TERM
                return 0
            else
                local status=$?
                trap - INT TERM
                return "$status"
            fi
        fi
        if ! kill -0 "$pid" 2>/dev/null; then
            local status=0
            wait "$pid" || status=$?
            task3_die "process exited ($status) before the required library appeared in /proc/$pid/maps"
        fi
        sleep 0.1
    done
    kill -TERM "$pid" 2>/dev/null || true
    wait "$pid" || true
    trap - INT TERM
    task3_die "runtime library mapping was not verified for process $pid"
}
