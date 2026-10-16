"""Runtime checks for explicitly selected Task 3 shared dependencies."""
from pathlib import Path
import time


def verify_statistics_backend_mapped(pid, prefix, timeout=10.0):
    if not prefix:
        raise RuntimeError('TASK3_PATCHED_BACKEND_PREFIX is required for runtime verification')
    expected = (Path(prefix) / 'lib/libfastdds_statistics_backend.so.2.5.0').resolve(strict=True)
    maps_path = Path(f'/proc/{pid}/maps')
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            maps = maps_path.read_text()
        except OSError:
            maps = ''
        if any(str(expected) in line for line in maps.splitlines()):
            return {'pid': pid, 'library': str(expected), 'verified': True}
        time.sleep(0.05)
    raise RuntimeError(f'process {pid} did not map the pinned Statistics Backend: {expected}')
