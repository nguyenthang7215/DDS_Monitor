# Hồi quy Linux với Statistics Backend 2.5.0 Original — đợt b (02/11/2026)

Chạy tuần tự trên máy rảnh bằng `run_all.sh` và `run_integration.sh` (cùng thư mục). Kết quả từng bước: `summary.txt`.

| Bước | Kết quả |
| :--- | :--- |
| CTest Release | PASS |
| `local_integration.py` | 14/14 PASS (+3 PENDING_EXTERNAL_TEST: Windows build, Linux–Windows vật lý, NTP 2 máy) |
| `continuation_integration.py` (60 s) | 15/15 PASS (+1 PENDING_EXTERNAL_TEST) |
| Subscriber restart | 6/6 PASS, thoát sạch |
| Publisher stop/restart 30 chu kỳ × 5 | **4/5 crash SIGSEGV** (chu kỳ 26, 24, 7, 20); run3 30/30 nhưng treo khi thoát |

Backtrace từng lượt crash: `publisher-30-run*/crash-backtrace.txt`.
run1, run2, run4 crash trong `DatabaseQueue` / `DatabaseDataQueue` của Statistics Backend (P0).
run5 crash trong Fast DDS `DataReaderHistory::writer_not_alive`, chưa phân tích.

Lần đầu `local`/`continuation` báo rc=1 trong 0 s do gọi `python3 -I` (không import được module test cùng thư mục); đã chạy lại với `local-rerun`/`continuation-rerun`.
