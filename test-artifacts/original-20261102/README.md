# Bằng chứng kiểm thử 02/11/2026 (Linux, tự động)

Môi trường: Linux Master Xubuntu 22.04 · Qt 6.7.2 · Fast DDS 3.6.1 · build `.task3-deps/task3-builds/20261023T043556Z-72913/release`.
Không dùng máy Windows. Tất cả chạy offscreen (`QT_QPA_PLATFORM=offscreen`).

| Thư mục | Nội dung | Kết quả |
| :--- | :--- | :--- |
| `graph01/` | Test QML tự động GRAPH-01 trên `DomainGraphLayout.qml` gốc và bản có patch 1 dòng | Gốc 3/6 · sửa 6/6 |
| `qos-local/`, `qos-profiles/` | E2E-12 cục bộ: 3 cặp QoS HelloWorld | PASS 3/3 |
| `publisher-30/` | P0 stress 30 chu kỳ Publisher, Backend **Original** | 30/30, nhưng Master không thoát trong 25 s (bị kill, `master_gdb_exit = -9`) |
| `shutdown-check-1/`, `shutdown-check-2/` | 3 chu kỳ, Backend Original | PASS, thoát sạch |
| `publisher-30-rerun/` | P0 stress 30 chu kỳ, Backend **Original** (chạy song song với test QML) | **FAIL: SIGSEGV chu kỳ 2** trong `DatabaseDataQueue::process_sample()` (backtrace: `publisher-30-rerun/crash-backtrace.txt`) |
| `patched-publisher-30-run1/`, `run2/` | Cùng kịch bản, Backend **Patched** | **PASS 30/30 · 30/30**, thoát sạch |

Lệnh chạy lại:

```bash
# GRAPH-01
QT_QPA_PLATFORM=offscreen qmltestrunner -input task3/tests/qml/graph -import imports
# QoS
LD_LIBRARY_PATH=/home/dev/dds-deps/install/lib python3 task3/tests/qos_match_local.py \
  --hello /home/dev/dds-tests/hello_world/build/hello_world --output <dir> \
  --case compatible=<qos-profiles>/compatible.xml=match \
  --case writer_best_effort_reader_reliable=<qos-profiles>/incompatible-writer.xml=nomatch \
  --case both_best_effort=<qos-profiles>/both-best-effort.xml=match
# P0 (Original)
LD_LIBRARY_PATH=/home/dev/dds-e2e/backend-original-install/lib:/home/dev/dds-deps/install/lib:/home/dev/Qt/6.7.2/gcc_64/lib \
TASK3_PATCHED_BACKEND_PREFIX=/home/dev/dds-e2e/backend-original-install \
python3 test-artifacts/p0-crash-20261016/publisher_stop_restart_reproducer.py \
  --master <build>/release/dds_monitor --probe <build>/release/task3/task3_probe --output <dir> --cycles 30 --dwell 2
```
