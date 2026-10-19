# Final Fix DDS Monitor — 19/10/2026

Project: `/home/dev/DDS-Monitor`. Baseline trước sửa: commit `6b3e652`, worktree sạch. Requirements giữ nguyên; không sửa dependency/patch Statistics Backend, NTP/OS/firewall hoặc tự push GitHub.

## Nguyên nhân và bản sửa

**P1-01 — SQLite lỗi nhưng UI không hiển thị:** Label ở `qml/main.qml` thay cả status khi bắt đầu bằng `DDS telemetry domain`, làm mất phần `Event database` và `dropped telemetry` phía sau. Bản sửa chỉ bỏ prefix kỹ thuật, giữ các chẩn đoán sau dấu ` | ` và thêm tooltip. Snapshot kiểm thử đọc `text`/`visible` từ Label thật qua `Dashboard.cpp`. Regression tạo database hỏng riêng, chạy Master/QML thật và lưu ảnh; không đụng database người dùng.

**P1-02 — Không phục hồi sau clock rollback và Agent restart:** Registry trước đây yêu cầu timestamp session mới lớn hơn mức cao nhất trước đó. Bản sửa giữ thứ tự bằng sequence trong session, nhớ 256 session đã nghỉ gần nhất/máy để từ chối bản tin quay lại; không dùng đồng hồ Slave để chấp nhận session mới. Schema, kiểu dữ liệu, giới hạn kích thước và kiểm tra bản tin vẫn giữ nguyên; không đổi telemetry protocol.

Giới hạn cần nêu rõ: lọc này chỉ trong bộ nhớ, mất khi Master restart; session chưa từng thấy hoặc đã ra ngoài cửa sổ 256 không thể phân biệt chắc chắn với một phiên mới khi đồng hồ có thể lùi. Đây không phải bảo đảm chống replay bảo mật. Không bổ sung HMAC, handshake hoặc checkpoint bền vào hệ thống mạng tin cậy hiện tại.

**P1-03 — Verified sai khi Unknown/thiếu ứng dụng:** `MonitoringCore::events()` trước đây chỉ xem Host Online. Bản sửa đối chiếu đúng ứng dụng và đúng trường process/service hiện hành. Khi thiếu bằng chứng vi phạm, cảnh báo vẫn Active nhưng ghi Not verified; logic tạo/khôi phục cảnh báo và SQLite không đổi.

**Agent standalone:** khai báo `Concurrent` và `Qt6::Concurrent` trong target độc lập vì collector đã dùng `QtConcurrent::run`, nhất quán với root CMake. Configure/build sạch và collector `--once` PASS trên Linux. Không khẳng định bản trước đã build FAIL trên Windows, cũng không lấy kết quả Linux thay cho Windows.

## Test và bằng chứng

| Hạng mục | Kết quả | Bằng chứng dưới `test-artifacts/final-fix-20261019/` |
| --- | --- | --- |
| P1-01 trước sửa | FAIL đúng lỗi UI che database; core đã báo lỗi | `p1-ui-red/results.json` |
| P1-02/03 trước sửa | 3 trường hợp FAIL: rollback, Unknown, thiếu ứng dụng | `p1-unit-red.log` |
| Regression riêng sau sửa, Debug/Release | PASS | `p1-unit-green.log`, `p1-unit-release.log` |
| Database error trên Label QML thật, Debug/Release | PASS | `p1-ui-debug/results.json`, `p1-ui-release/results.json`, `p1-ui-release/database-error.png` |
| Debug/Release build + CTest | PASS, mỗi cấu hình 1/1 target; Qt Test 44 mục gồm setup/cleanup | `build-debug.log`, `build-release.log`, `ctest-*.log`, `qt-tests-*.log` |
| Agent standalone Linux build sạch và `--once` | PASS | `standalone-configure.log`, `standalone-build.log`, `standalone-telemetry.json` |
| Local integration | 14/14 PASS | `local/results.json` |
| Continuation integration | 15/15 PASS, theo dõi ổn định 30 giây | `continuation/results.json` |
| UI integration X11 1600×900 | 17/17 PASS; giữ lựa chọn ba tab, SQLite/QML/shutdown PASS | `ui-1600/results.json` |

Fixture `repeatedAgentRestartsStayBounded` cũ chưa đưa session gốc vào Registry trước khi gọi nó là replay. Đã bổ sung bước nhận ban đầu, giữ kiểm tra từ chối session đó và tăng lên 300 lần restart để xác nhận danh sách chỉ giữ 256. Lượt thất bại khi đối chiếu fixture được giữ tại `p1-unit-first-attempt.log` và `ctest-debug-first-attempt.log`, không tính PASS.

Các test mới không thay đồng hồ hệ điều hành: chỉ đặt timestamp T và T−60.000 trong payload fixture. Stopped → Unknown/thiếu ứng dụng → Running kiểm tra cùng alert_id, Active chưa có recovered_ms khi thiếu bằng chứng, và Recovered sau snapshot Running hợp lệ trong unit fixture. CTest giữ kiểm tra SQLite migration, lịch sử trên 500 bản ghi, lock/retry và trạng thái qua restart; continuation xác nhận integrity, history và shutdown.

Runtime maps trong các integration xác nhận Master thực tế dùng Backend persistent đã vá. `protected.sha256`/`protected-check.log` xác nhận Collectors, Protocol, DdsTransport, DdsBridge, FlowModel, AlertStore, Engine, SyncBackendConnection, DynamicStatisticsChartView và thư viện Backend không đổi so với baseline.

## File thay đổi

Source/test: `qml/main.qml`, `task3/Dashboard.cpp` (chỉ snapshot kiểm thử), `task3/Registry.h`, `task3/Registry.cpp`, `task3/MonitoringCore.cpp`, `task3/agent/CMakeLists.txt`, `task3/tests/CoreTests.cpp`, `task3/tests/final_fix_ui_regression.py` (mới).

Tài liệu: `TASK3_README.md` và các file trong `docs/task3/`: `ARCHITECTURE.md`, `BUILD_AND_RUN.md`, `COMMUNICATION.md`, `CONFIGURATION.md`, `CONTINUATION_CHECKPOINT.md`, `DATA_MODEL.md`, `DELIVERY_REPORT.md`, `IMPLEMENTATION_PLAN.md`, `IMPLEMENTATION_STATUS.md`, `KNOWN_ISSUES.md`, `PRE_TEST_READINESS.md`, `TEST_PLAN.md`, `TEST_RESULTS.md` và báo cáo này. Chỉ đồng bộ source root/link/lệnh, bỏ chỉ dẫn key/checkpoint không còn đúng, và ghi bằng chứng mới; không chỉnh Requirements.

## Giới hạn và bàn giao

Windows native build/runtime, Linux–Windows vật lý và NTP liên máy vẫn PENDING_EXTERNAL_TEST. Không chạy thêm stress Publisher 30 chu kỳ hoặc xác minh Dynamic Chart aggregate/cumulative trong đợt sửa này; không sao chép kết quả lịch sử thành kết quả mới. Chưa xác nhận clone/build trên máy khác không có dependency chuẩn bị sẵn.

Trạng thái: **đủ điều kiện tạo commit cho phạm vi Final Fix đã kiểm chứng trên Linux**, sau khi chọn source/tests/docs đúng phạm vi. `git diff --check` sạch và checksum thành phần được bảo vệ khớp baseline. Đây không phải nghiệm thu Windows/NTP hay toàn hệ thống liên máy. Chưa tạo commit hoặc push; artifact sinh tự động, database và build binary không phải phần source cần commit.
