# Kết quả kiểm thử Task 3 — 16-10-2026

## Final verification trên Backend persistent đã vá

Trạng thái: **READY_FOR_MANUAL_LINUX_TESTING**. Không dùng bản sao `/tmp/dds-p0` làm bằng chứng kết luận cuối; các kết quả dưới đây được chạy trên Backend build/install trong workspace và Task 3 Debug/Release mới.

| Hạng mục | Kết quả | Bằng chứng |
| --- | --- | --- |
| Source Backend sạch, commit/tag và patch checksum | PASS | commit `314fcd6e9d888077497d815406fcf134c6754759`, tag `v2.5.0`, patch SHA `83019a4479ec8b05cdab0a2d0e97461d9e3d386bcda865ba5f89fbf02e383a9a` |
| Backend 2.5.0 persistent build/install | PASS | `.task3-deps/fastdds-statistics-backend-2.5.0-patched/`; library SHA `167f5afa4fd7c6b9991f84fdf40b7a99564c264515e10c1507da656e565b9003`, hai build cho cùng hash |
| SONAME, headers, symbol export, CMake selection, ldd | PASS | SONAME `.so.2`; 23.081 symbol; public headers byte-identical; CMake cache ghim prefix vá; runtime deps phân giải từ prefix dự kiến |
| Clean Task 3 Debug/Release + CTest | PASS | `.task3-deps/task3-builds/20261016T054514Z-2/`; CTest 1/1 mỗi cấu hình |
| Local integration | PASS 14/14 | `test-artifacts/p0-crash-20261016/persistent-final-local-14/results.json` |
| Continuation integration | PASS 15/15 | `.../persistent-final-continuation-15/results.json`; có SQLite, QML, Offline/Recovery, No Data, Master restart |
| Subscriber restart | PASS 6/6 | `.../persistent-final-subscriber-restart/results.json` |
| Publisher stop/restart | PASS 30/30 | `.../persistent-final-publisher-30/result.json`; Publisher exit 0 từng lượt, Master/GDB exit 0 |
| Runtime library mapping | PASS | Mỗi regression ghi `runtime-library-maps.json`/`runtime_backend_map`; Master map đúng file trong prefix persistent |
| SQLite integrity | PASS | `PRAGMA integrity_check=ok`; continuation ghi 10 event rows |
| DDS Discovery/Throughput/Latency/Alerts | PASS | Local integration 14/14 và continuation 15/15 trên artifact mới |
| Master shutdown/QML/process cleanup | PASS | Master shutdown sạch; không có QML TypeError/ReferenceError/binding loop; không còn test process |
| Windows/NTP hai máy/Linux–Windows vật lý | PENDING_EXTERNAL_TEST | Chưa kiểm chứng trong môi trường này |

Backend mặc định `/home/dev/dds-deps/install/lib/libfastdds_statistics_backend.so.2.5.0` giữ nguyên SHA `6aeb9f948f33c431d859321267c08cb7952d30c04fed07eb5ea6016e0ff0092f`; không bị ghi đè. Build/run manual Linux phải dùng các wrapper tại `task3/scripts/`.

## Kết quả các đợt trước

## Môi trường và nguồn bằng chứng

Xubuntu Linux, GCC 11.4, CMake 3.22, Qt 6.7.2, Fast DDS 3.6.1 và Statistics Backend 2.5.0. Tất cả Master, Agent, Hello World và probe đều chạy thành các tiến trình trên một máy Linux vật lý. Không sử dụng Windows, máy vật lý thứ hai, thay đổi NTP/service/firewall hoặc đo thời gian bằng thiết bị độc lập.

Source: implementation/. Checkpoint trước sửa 3881967, thêm asset baseline tại 52bbdea. Solutions giữ nguyên SHA-256 57e0de67e06239e22b558e441e010add14f8c79a7e324fa63835adc39e1eb47c.

## Build và hồi quy

| Hạng mục | Kết quả | Bằng chứng |
| --- | --- | --- |
| Baseline Release/CTest | PASS | continuation-baseline và log kiểm tra đầu phiên |
| Debug thư mục build mới | PASS | continuation-final/debug-configure.log, debug-build.log |
| Release thư mục build mới | PASS | continuation-final/release-configure.log, release-build.log |
| Agent độc lập Linux | PASS; --once thành công | continuation-final/agent-build.log, agent-once.json |
| CTest Debug | 1/1 executable PASS | continuation-final/debug-ctest.log |
| CTest Release | 1/1 executable PASS | continuation-final/release-ctest.log |
| Qt Test | 37 mục PASS, 0 fail/skip; 35 kịch bản chính và setup/cleanup | build-continuation-debug/Testing/Temporary/LastTest.log và bản Release tương ứng |
| Tích hợp DDS cũ | 14/14 PASS | continuation-final/regression/results.json |
| Tích hợp mới | 15 kiểm tra PASS | continuation-final/runtime/results.json |
| Smoke binary cuối sau sửa tên executable dài | 15 kiểm tra PASS, lấy mẫu thêm 10 giây | continuation-final/runtime-final/results.json |
| Xác minh cuối sau build Debug/Release | CTest 1/1 mỗi cấu hình PASS; Qt Test 37 mục PASS | build-continuation-debug/Testing/Temporary/LastTest.log và build-continuation-release/Testing/Temporary/LastTest.log |
| Tích hợp cuối có quyền mở UDP socket | 15 kiểm tra hành vi PASS; SQLite integrity và QML runtime PASS | continuation-final/verification-20261016-socket/results.json |
| Hồi quy DDS cũ chạy lại cuối | 14/14 PASS | continuation-final/regression-verification-20261016/results.json |
| Windows/physical Linux–Windows/NTP hai máy | PENDING_EXTERNAL_TEST | Chưa có môi trường |

Các đường dẫn continuation-* trong tài liệu này nằm dưới test-artifacts/. Lượt verification-20261016 không có quyền socket bị Fast DDS từ chối khi restart process; artifact được giữ để truy vết. Chạy lại cùng kịch bản với quyền socket cục bộ và đạt đủ 15 kiểm tra tại verification-20261016-socket. Các chỉnh sửa cuối cho bộ chọn executable Linux dài được kiểm tra bằng tiến trình thật do unit test sở hữu.

## Kiểm thử mới và phạm vi kết luận

Các test cũ được giữ. Ca bổ sung xác nhận manifest/PID/instance, nhiều Participant, GUID mới qua restart, metadata mâu thuẫn → Unmapped, nhiều process tên executable dài, trạng thái process/service riêng, matching có/không có bằng chứng, periodic/event idle, hồi quy DynamicDataChartBox/DataModel gốc, Windows JSON parser, SQLite worker/lock/retention/schema cũ, debounce/cooldown/hysteresis, replay qua Master restart, hàng đợi và shutdown.

Runtime probe thật tạo hai Participant ở mỗi tiến trình pub/sub. Đã ghi nhận mapping, topology nhiều Reader, ConnectionList còn mới, Writer vẫn Alive khi ngừng write, restart với GUID mới và khôi phục No Data. Master và Agent được dừng bằng SIGTERM, thoát bình thường, khởi động lại; SQLite integrity_check trả ok. Không có QML TypeError/ReferenceError, undefined-property hoặc binding-loop trong log Master của lượt tích hợp mới.

Ảnh từng trang nằm trong continuation-final/runtime/dashboard.png.page-0.png đến page-4.png. Trang cuối là DDS nâng cao gốc. Matching có thể trở lại Unknown sau khi nguồn không cập nhật ConnectionList đủ mới; ảnh cuối không được dùng để khẳng định kết nối hiện thời còn được xác nhận.

## Số liệu tải quan sát được

- Release: 1024 snapshot có chữ ký, 128 host tổng hợp × 32 ứng dụng và 1000 gói sai được xử lý trong khoảng 174 ms ở lượt được ghi. Đây là đo Registry/protocol, không phải UI với 128 máy vật lý.
- Burst 2000 mẫu DDS đã làm bộ đếm bỏ snapshot tăng 1744 trong lượt kiểm thử; queue bổ sung chỉ giữ tối đa 256. Việc bỏ snapshot là chính sách quá tải có chủ đích, không phải bằng chứng truyền không mất mẫu.
- SQLite được giữ khóa ghi khi đưa vào 3000 sự kiện. Timer giao diện vẫn tiếp tục; sau khi mở khóa và shutdown, dữ liệu/trạng thái được đọc lại. Cache chỉ hiển thị 500 dòng.
- Lượt tích hợp mới lấy 120 mẫu trong khoảng 119,33 giây: RSS Master từ 205,16 tới 207,92 MiB, miền quan sát 205,16–207,97 MiB; CPU trung bình khoảng 16,08% của một logical core. ui_steps tăng từ 8 tới 167.
- Trong phép đo RSS/CPU, UI tự resize/chuyển trang và xuất JSON/PNG định kỳ; đây là chế độ chẩn đoán, không phải benchmark cách ly hoặc cam kết hiệu năng triển khai. Khoảng quan sát ngắn không chứng minh không rò rỉ bộ nhớ hay ổn định 24 giờ.

Dữ liệu gốc: continuation-final/runtime/resource-usage.json. Không tự gán các số này thành ngưỡng nghiệm thu.

## Lỗi tìm ra và phần còn mở

continuation-runtime-01 đã phát hiện cảnh báo No Data dùng ID Writer cũ không khôi phục sau restart. Đã sửa khóa theo máy/ứng dụng/Topic và kiểm tra lại trong runtime-02 cùng lượt cuối. Các artifact thất bại vẫn được giữ.

Lỗi con trỏ bỏ qua mẫu muộn được xác lập trong nhánh raw Dynamic Chart và sửa bằng truy vấn lại/thay snapshot; chưa đóng lỗi aggregate/cumulative hoặc toàn bộ hiện tượng PoC. Windows native time synchronization, service lifecycle thật, NTP liên máy, mật độ DDS cao và nghiệm thu 24 giờ vẫn chưa xác minh.

Báo cáo không chuyển trạng thái Windows hoặc nghiệm thu Master–Slave vật lý thành PASS dựa trên build/test Linux.

## Điều tra P0 và hồi quy với patch Backend cô lập — 16-10-2026

Kết quả crash ban đầu vẫn được giữ làm bằng chứng: Task 3 Release với Statistics Backend chưa vá crash ở chu kỳ Publisher thứ 12/30, sau 11 chu kỳ thành công. Full GDB backtrace ghi nhận heap corruption, `Data::_d(this=0x0)` trong `DatabaseDataQueue<fastdds::statistics::Data>::process_sample()` và callback discovery khác đang dừng/join consumer. Phân tích nguyên nhân ở `CRASH_ROOT_CAUSE_ANALYSIS.md`.

| Kiểm thử | Kết quả mới nhất | Artifact |
| --- | --- | --- |
| Task 3 Release + Backend chưa vá, Publisher stop/restart | FAIL chu kỳ 12/30 | `test-artifacts/p0-crash-20261016/controlled-task3-30cycles/` |
| DDS Monitor gốc + cùng Backend chưa vá, 30 chu kỳ | 30/30 chu kỳ PASS; GDB exit 255 khi dọn tiến trình bằng SIGTERM | `test-artifacts/p0-crash-20261016/controlled-original-30cycles/result.json` |
| Queue lifecycle reproducer chưa vá, ASan/UBSan | FAIL với memory fault trong queue pop/consume | Source reproducer: `test-artifacts/p0-crash-20261016/database_queue_lifecycle_regression.cpp` |
| Queue lifecycle reproducer đã vá, ASan/UBSan | PASS; tối đa một consumer chạy đồng thời | Cùng source reproducer, biên dịch với header đã vá trong `/tmp/dds-p0` |
| Task 3 Release + Backend vá, Publisher stop/restart | 30/30 PASS; Master/GDB thoát mã 0 | `test-artifacts/p0-crash-20261016/fresh-patched-task3-30cycles/result.json` |
| Task 3 Release + Backend vá, Subscriber dừng/khởi động lại | 6/6 điều kiện PASS, GUID mới được ánh xạ, Master và Publisher còn sống | `test-artifacts/p0-crash-20261016/fresh-subscriber-restart/results.json` |
| Build Debug/Release mới và CTest | Build PASS; CTest 1/1 cho mỗi cấu hình | `/tmp/dds-p0/task3-debug`, `/tmp/dds-p0/task3-release` |
| Local integration với Release mới | 14/14 PASS | `test-artifacts/p0-crash-20261016/fresh-release-local-14/results.json` |
| Continuation integration với Release mới | 15/15 PASS; SQLite integrity, shutdown, QML PASS | `test-artifacts/p0-crash-20261016/fresh-release-continuation-15/results.json` |
| ABI kiểm tra trong cấu hình build tương đương | SONAME và toàn bộ 23.081 tên symbol động giống nhau; public headers không đổi | Bản cài đặt và `/tmp/dds-p0/patched-abi-prefix` |

Các dòng `/tmp/dds-p0` và `fresh-*` phía trên là kết quả điều tra trước khi có quy trình persistent. Kết luận hiện tại được thay bởi phần “Final verification trên Backend persistent đã vá” ở đầu tài liệu; prefix mặc định vẫn chưa vá nên chỉ dùng wrapper đã ghi trong `BUILD_AND_RUN.md`. Windows, NTP liên máy và Linux–Windows trên hai máy vật lý vẫn **PENDING_EXTERNAL_TEST**.

Các đường dẫn `test-artifacts/...` trong phần P0 tính từ thư mục `/home/dev/Downloads/Bao_cao_DDS_Monitor_Task3/implementation/`; khi sao chép tài liệu sang repo khác, các artifact vẫn ở workspace này.
