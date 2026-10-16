#!/usr/bin/env bash
set -euo pipefail

task3_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
expected_commit="314fcd6e9d888077497d815406fcf134c6754759"
expected_version="2.5.0"
expected_patch_sha="83019a4479ec8b05cdab0a2d0e97461d9e3d386bcda865ba5f89fbf02e383a9a"
patch_file="$task3_root/test-artifacts/p0-crash-20261016/Fast-DDS-Statistics-Backend-2.5.0-database-queue-lifecycle.patch"
source_repo="${TASK3_BACKEND_SOURCE:-/home/dev/dds-deps/Fast-DDS-statistics-backend}"
dds_prefix="${TASK3_DDS_PREFIX:-/home/dev/dds-deps/install}"
install_prefix="${TASK3_PATCHED_BACKEND_PREFIX:-$task3_root/.task3-deps/fastdds-statistics-backend-$expected_version-patched}"
build_root="${TASK3_BACKEND_BUILD_ROOT:-$task3_root/.task3-deps/backend-builds}"
jobs="${TASK3_BUILD_JOBS:-4}"

usage() {
    cat <<'EOF'
Usage: build_statistics_backend_patched.sh [--source DIR] [--dds-prefix DIR]
       [--prefix DIR] [--build-root DIR] [--jobs N]

Environment equivalents: TASK3_BACKEND_SOURCE, TASK3_DDS_PREFIX,
TASK3_PATCHED_BACKEND_PREFIX, TASK3_BACKEND_BUILD_ROOT, TASK3_BUILD_JOBS.
EOF
}

while (($#)); do
    case "$1" in
        --source) source_repo="$2"; shift 2 ;;
        --dds-prefix) dds_prefix="$2"; shift 2 ;;
        --prefix) install_prefix="$2"; shift 2 ;;
        --build-root) build_root="$2"; shift 2 ;;
        --jobs) jobs="$2"; shift 2 ;;
        --help|-h) usage; exit 0 ;;
        *) printf 'Unknown argument: %s\n' "$1" >&2; usage >&2; exit 2 ;;
    esac
done

for tool in git cmake c++ tar patch sha256sum readelf nm ldd realpath diff strings; do
    command -v "$tool" >/dev/null || { printf 'Missing required tool: %s\n' "$tool" >&2; exit 2; }
done
[[ -f "$patch_file" ]] || { printf 'Pinned patch is missing: %s\n' "$patch_file" >&2; exit 2; }
[[ -d "$source_repo/.git" || -f "$source_repo/.git" ]] || { printf 'Backend source is not a Git checkout: %s\n' "$source_repo" >&2; exit 2; }
[[ -d "$dds_prefix" ]] || { printf 'Fast DDS dependency prefix does not exist: %s\n' "$dds_prefix" >&2; exit 2; }
[[ -f "$dds_prefix/share/fastdds/cmake/fastdds-config.cmake" ]] || { printf 'Fast DDS CMake package missing under %s\n' "$dds_prefix" >&2; exit 2; }
[[ -f "$dds_prefix/lib/cmake/fastcdr/fastcdr-config.cmake" ]] || { printf 'Fast CDR CMake package missing under %s\n' "$dds_prefix" >&2; exit 2; }
[[ "$jobs" =~ ^[1-9][0-9]*$ ]] || { printf 'Invalid --jobs value: %s\n' "$jobs" >&2; exit 2; }

actual_patch_sha="$(sha256sum "$patch_file" | awk '{print $1}')"
[[ "$actual_patch_sha" == "$expected_patch_sha" ]] || {
    printf 'Patch checksum mismatch; refusing to build. Expected %s, got %s\n' "$expected_patch_sha" "$actual_patch_sha" >&2
    exit 2
}
actual_commit="$(git -C "$source_repo" rev-parse HEAD)"
[[ "$actual_commit" == "$expected_commit" ]] || {
    printf 'Backend source commit mismatch; expected %s, got %s\n' "$expected_commit" "$actual_commit" >&2
    exit 2
}
git -C "$source_repo" describe --tags --exact-match HEAD 2>/dev/null | grep -Fxq "v$expected_version" || {
    printf 'Backend source is not tagged v%s at HEAD.\n' "$expected_version" >&2
    exit 2
}
if [[ -n "$(git -C "$source_repo" status --porcelain --untracked-files=all)" ]]; then
    printf 'Backend source tree is dirty; refusing to archive anything other than the pinned clean source.\n' >&2
    git -C "$source_repo" status --short >&2
    exit 2
fi

protected_prefix="$(realpath -m /home/dev/dds-deps/install)"
resolved_prefix="$(realpath -m "$install_prefix")"
case "$resolved_prefix/" in
    "$protected_prefix/"*) printf 'Refusing to install into the protected default dependency prefix: %s\n' "$resolved_prefix" >&2; exit 2 ;;
esac
case "$protected_prefix/" in
    "$resolved_prefix/"*) printf 'Refusing to install into an ancestor of the protected dependency prefix: %s\n' "$resolved_prefix" >&2; exit 2 ;;
esac
[[ "$resolved_prefix" != / && "$resolved_prefix" != "$source_repo" ]] || { printf 'Unsafe install prefix: %s\n' "$resolved_prefix" >&2; exit 2; }

mkdir -p "$build_root" "$(dirname "$resolved_prefix")"
run_id="$(date -u +%Y%m%dT%H%M%SZ)-$$"
run_dir="$build_root/$run_id"
mkdir "$run_dir" || { printf 'Build run directory already exists: %s\n' "$run_dir" >&2; exit 2; }
source_stage="$run_dir/source"
build_dir="$run_dir/build"
mkdir "$source_stage"

owner_marker="$resolved_prefix/.task3-patched-backend-owned"
if [[ -e "$resolved_prefix" ]]; then
    [[ -f "$owner_marker" ]] || {
        printf 'Install prefix already exists and is not owned by this build script; refusing to overwrite: %s\n' "$resolved_prefix" >&2
        exit 2
    }
    grep -Fxq "repo=$task3_root" "$owner_marker" || { printf 'Install prefix ownership marker belongs to another workspace: %s\n' "$resolved_prefix" >&2; exit 2; }
else
    mkdir -p "$resolved_prefix"
    {
        printf 'repo=%s\n' "$task3_root"
        printf 'source_commit=%s\n' "$expected_commit"
        printf 'patch_sha256=%s\n' "$expected_patch_sha"
    } > "$owner_marker"
fi

printf 'Creating clean source archive at pinned commit %s...\n' "$expected_commit"
git -C "$source_repo" archive "$expected_commit" | tar -x -C "$source_stage"
queue_header="$source_stage/src/cpp/database/database_queue.hpp"
if grep -Fq 'lifecycle_mutex_' "$queue_header"; then
    printf 'Pinned clean source already contains the lifecycle patch marker; refusing a double application.\n' >&2
    exit 2
fi
patch --dry-run --batch --fuzz=0 -p1 -d "$source_stage" < "$patch_file"
patch --batch --fuzz=0 -p1 -d "$source_stage" < "$patch_file"
reverse_check="$run_dir/reverse-patch-check.txt"
patch --dry-run --batch --fuzz=0 -R -p1 -d "$source_stage" < "$patch_file" > "$reverse_check" 2>&1 || {
    cat "$reverse_check" >&2
    printf 'Could not verify the applied patch in reverse.\n' >&2
    exit 2
}
if grep -Fq 'Unreversed patch detected' "$reverse_check"; then
    cat "$reverse_check" >&2
    printf 'Patch did not apply to the expected clean source context.\n' >&2
    exit 2
fi
grep -Fq 'lifecycle_mutex_' "$queue_header" || {
    printf 'Patched queue lifecycle guard is absent after applying the pinned patch.\n' >&2
    exit 2
}

printf 'Configuring Fast DDS Statistics Backend %s (shared, default build type)...\n' "$expected_version"
source_path_map="/usr/src/fastdds-statistics-backend-$expected_version"
cmake -S "$source_stage" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE= \
    -DCMAKE_INSTALL_PREFIX="$resolved_prefix" \
    -DCMAKE_PREFIX_PATH="$dds_prefix" \
    -Dfastdds_DIR="$dds_prefix/share/fastdds/cmake" \
    -Dfastcdr_DIR="$dds_prefix/lib/cmake/fastcdr" \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_TESTS=OFF \
    -DBUILD_DOCS=OFF \
    -DCOMPILE_EXAMPLES=OFF \
    -DCMAKE_C_FLAGS="-ffile-prefix-map=$source_stage=$source_path_map" \
    -DCMAKE_CXX_FLAGS="-ffile-prefix-map=$source_stage=$source_path_map"
cmake --build "$build_dir" --parallel "$jobs"
cmake --install "$build_dir"

backend_lib="$resolved_prefix/lib/libfastdds_statistics_backend.so.$expected_version"
baseline_lib="$dds_prefix/lib/libfastdds_statistics_backend.so.$expected_version"
[[ -f "$backend_lib" ]] || { printf 'Expected installed shared library is missing: %s\n' "$backend_lib" >&2; exit 1; }
[[ -f "$baseline_lib" ]] || { printf 'Baseline library needed for ABI comparison is missing: %s\n' "$baseline_lib" >&2; exit 1; }
if strings "$backend_lib" | grep -Fq "$run_dir/source"; then
    printf 'Build staging path leaked into the shared library; file-prefix mapping did not take effect.\n' >&2
    exit 1
fi
if ! diff -qr "$resolved_prefix/include/fastdds_statistics_backend" "$dds_prefix/include/fastdds_statistics_backend" > "$run_dir/public-header-diff.txt"; then
    cat "$run_dir/public-header-diff.txt" >&2
    printf 'Installed Backend headers differ from the pinned 2.5.0 dependency headers; refusing a mixed header/library build.\n' >&2
    exit 1
fi
soname="$(readelf -d "$backend_lib" | sed -n 's/.*SONAME.*\[\([^]]*\)\].*/\1/p')"
[[ "$soname" == "libfastdds_statistics_backend.so.2" ]] || { printf 'Unexpected SONAME: %s\n' "$soname" >&2; exit 1; }
[[ "$(realpath "$resolved_prefix/lib/libfastdds_statistics_backend.so.2")" == "$(realpath "$backend_lib")" ]] || {
    printf 'Installed SONAME symlink does not resolve to the pinned 2.5.0 library.\n' >&2
    exit 1
}

nm -D --defined-only --format=posix "$backend_lib" | awk 'NF {print $1}' | sort -u > "$run_dir/patched-symbols.txt"
nm -D --defined-only --format=posix "$baseline_lib" | awk 'NF {print $1}' | sort -u > "$run_dir/baseline-symbols.txt"
if ! cmp -s "$run_dir/patched-symbols.txt" "$run_dir/baseline-symbols.txt"; then
    printf 'Exported symbol set differs from the installed Backend 2.5.0 baseline; see %s\n' "$run_dir" >&2
    comm -3 "$run_dir/baseline-symbols.txt" "$run_dir/patched-symbols.txt" | head -80 >&2 || true
    exit 1
fi

runtime_path="$resolved_prefix/lib:$dds_prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
if LD_LIBRARY_PATH="$runtime_path" ldd "$backend_lib" | tee "$run_dir/ldd-backend.txt" | grep -q 'not found'; then
    printf 'Backend runtime dependency resolution failed with configured prefixes; see %s\n' "$run_dir/ldd-backend.txt" >&2
    exit 1
fi
resolved_fastdds="$(LD_LIBRARY_PATH="$runtime_path" ldd "$backend_lib" | awk '$1 == "libfastdds.so.3.6" {print $3; exit}')"
resolved_fastcdr="$(LD_LIBRARY_PATH="$runtime_path" ldd "$backend_lib" | awk '$1 == "libfastcdr.so.2" {print $3; exit}')"
[[ -n "$resolved_fastdds" && "$(realpath "$resolved_fastdds")" == "$(realpath "$dds_prefix/lib/libfastdds.so.3.6")" ]] || {
    printf 'Patched Backend did not resolve Fast DDS 3.6.1 from %s\n' "$dds_prefix" >&2
    exit 1
}
[[ -n "$resolved_fastcdr" && "$(realpath "$resolved_fastcdr")" == "$(realpath "$dds_prefix/lib/libfastcdr.so.2")" ]] || {
    printf 'Patched Backend did not resolve Fast CDR from %s\n' "$dds_prefix" >&2
    exit 1
}

manifest_dir="$resolved_prefix/share/task3-patched-backend"
mkdir -p "$manifest_dir"
{
    printf 'version=%s\n' "$expected_version"
    printf 'source_commit=%s\n' "$expected_commit"
    printf 'patch_sha256=%s\n' "$expected_patch_sha"
    printf 'library_sha256=%s\n' "$(sha256sum "$backend_lib" | awk '{print $1}')"
    printf 'soname=%s\n' "$soname"
    printf 'abi_symbol_count=%s\n' "$(wc -l < "$run_dir/patched-symbols.txt" | tr -d ' ')"
    printf 'build_run=%s\n' "$run_dir"
    printf 'dds_prefix=%s\n' "$dds_prefix"
    printf 'compiler=%s\n' "$(c++ --version | head -1)"
    printf 'cmake=%s\n' "$(cmake --version | head -1)"
} > "$manifest_dir/MANIFEST.tmp"
mv "$manifest_dir/MANIFEST.tmp" "$manifest_dir/MANIFEST"

printf 'PATCHED_BACKEND_PREFIX=%s\n' "$resolved_prefix"
printf 'PATCHED_BACKEND_LIBRARY=%s\n' "$backend_lib"
printf 'PATCHED_BACKEND_MANIFEST=%s\n' "$manifest_dir/MANIFEST"
printf 'BUILD_LOGS=%s\n' "$run_dir"
