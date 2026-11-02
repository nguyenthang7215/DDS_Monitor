# Hồi quy Linux với Statistics Backend 2.5.0 Patched (02/11/2026)

Cùng kịch bản và cùng build với `../original-20261102b/`, chỉ thay thư viện Backend bằng bản vá
(`.task3-deps/fastdds-statistics-backend-2.5.0-patched`, patch SHA-256 `83019a44…`). Chạy bằng `run_all.sh`, máy rảnh, tuần tự.
`runtime-library-maps.json` xác nhận Master nạp đúng thư viện Patched.

| Bước | Patched (thư mục này) | Original (`original-20261102b`) |
| :--- | :--- | :--- |
| CTest Release | PASS | PASS |
| `local_integration.py` | 14/14 PASS | 14/14 PASS |
| `continuation_integration.py` | 15/15 PASS | 15/15 PASS |
| Subscriber restart | 6/6 PASS | 6/6 PASS |
| Publisher stop/restart 30 chu kỳ × 5 | **5/5 PASS, 150/150 chu kỳ, thoát sạch, 0 SIGSEGV** | **4/5 crash**, 1 treo khi thoát |

Mục PENDING_EXTERNAL_TEST (Windows, Linux–Windows vật lý, NTP 2 máy) giống nhau ở cả hai, cần máy Windows.
