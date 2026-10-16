# Báo cáo bàn giao đợt tiếp tục Task 3 — 16-10-2026

## Tích hợp vào project gốc

Project chính thức hiện là `/home/dev/DDS-Monitor`, không còn lấy thư mục Downloads làm source để chạy. Task 3 đã được chuyển và build lại tại repo này; patch được lưu tại `test-artifacts/p0-crash-20261016/`, Backend cài riêng trong `.task3-deps/`. Kết quả mới: Debug/Release + CTest PASS, Local 14/14, Continuation 15/15, Subscriber 6/6, Publisher 30/30 PASS; hash Backend `167f5afa4fd7c6b9991f84fdf40b7a99564c264515e10c1507da656e565b9003`. Kết quả chi tiết và trạng thái pending có trong các mục sau.

## Kết quả hoàn tất P0 — 16-10-2026

**READY_FOR_MANUAL_LINUX_TESTING.** Backend Statistics 2.5.0 đã được build tái lập từ source commit đã ghim và patch SHA-256 `83019a4479ec8b05cdab0a2d0e97461d9e3d386bcda865ba5f89fbf02e383a9a`, cài ở `.task3-deps/fastdds-statistics-backend-2.5.0-patched/`; hai lần build liên tiếp cho cùng library SHA-256 `167f5afa4fd7c6b9991f84fdf40b7a99564c264515e10c1507da656e565b9003`. Task 3 Debug/Release và CTest PASS; Local 14/14, Continuation 15/15, Subscriber restart 6/6, Publisher restart 30/30 PASS. CMake/`ldd` và `/proc/<pid>/maps` xác nhận Master nạp đúng library persistent. SQLite integrity, QML và shutdown sạch PASS.

Không ghi đè `/home/dev/dds-deps/install`; prefix mặc định vẫn chưa vá. Người dùng cần chạy bằng scripts `task3/scripts/build_task3_patched.sh`, `run_master_patched.sh`, `run_linux_agent_patched.sh`. Windows, NTP liên máy và Linux–Windows vật lý vẫn **PENDING_EXTERNAL_TEST**. Báo cáo chi tiết: [PRE_TEST_READINESS.md](PRE_TEST_READINESS.md), [TEST_RESULTS.md](TEST_RESULTS.md), [CRASH_ROOT_CAUSE_ANALYSIS.md](CRASH_ROOT_CAUSE_ANALYSIS.md).

Các phần dưới ghi lại việc triển khai Task 3 và kết quả các đợt trước; chúng không thay thế trạng thái P0 cuối ở trên.

## A. Phần đã triển khai

Phần dưới ghi lịch sử triển khai trước khi hợp nhất vào project gốc. Repo chính đã được tích hợp; checkpoint trước thao tác được lưu tại `.task3-migration-checkpoints/20261016-pre-integration/`.

- RuntimeIdentity/Collectors/Registry/Protocol: nhiều process instance, nhiều Participant, manifest theo PID/định danh khởi chạy, GUID cập nhật khi restart, metadata mâu thuẫn giữ Unmapped. Bộ chọn process Linux hỗ trợ tên executable dài, tránh giới hạn comm 15 byte.
- DdsBridge/FlowModel/MonitoringCore: dùng GUID ConnectionList và thời hạn bằng chứng để dựng topology theo máy/ứng dụng; phân biệt Alive, Matched/Unmatched/Unknown, Active, No Data và Statistics Stale. Quy tắc periodic/event tách riêng.
- Runtime write counter: phát hiện Writer còn sống nhưng ngừng ghi. Cảnh báo theo luồng logic máy/ứng dụng/Topic được khôi phục khi instance/GUID mới ghi trở lại; đã sửa lỗi phát hiện trong lượt tích hợp đầu.
- QML: trang Task 3 và các màn hình DDS nâng cao gốc nằm trong cùng Main Window. Có chi tiết ứng dụng/Participant, topology, metric, cảnh báo, Last Seen/Stale và thời gian.
- SQLite/AlertStore: một worker sở hữu kết nối, transaction/retry, cache giao diện, migration schema cũ, retention tùy chọn, debounce/cooldown/hysteresis/dedup và lưu Active/Recovered.
- DdsTransport/Shutdown: queue giới hạn/coalescing, bộ đếm bỏ snapshot, xử lý Qt quit và flush khi thoát bình thường. Replay checkpoint tồn tại qua Master restart; database không đọc được thì telemetry fail closed.
- Dynamic Chart: sửa nhánh raw NONE bằng truy vấn lại cửa sổ và thay snapshot của DataModel gốc. Giữ delay_time=30000 và FASTDDS_MONITOR_APP. Nhánh aggregate/cumulative vẫn mở.
- Time/Windows: Chrony được kiểm tra cấu trúc/nguồn/offset rõ hơn; Windows có JSON CIM adapter và script build theo prefix. Chưa có bằng chứng synchronized native hoặc build/runtime Windows.

Kiến trúc Solutions được giữ: Agent DDS riêng, Statistics Backend, Registry/Core, Desktop Qt và đồng bộ đồng hồ độc lập. SQLite chuyển sang worker và dashboard được nhúng vào cửa sổ chính là các thay đổi nội bộ, không bổ sung giao vận hoặc dịch vụ web.

## B. Build và kiểm thử

| Hạng mục | Kết quả |
| --- | --- |
| Debug và Release trong thư mục build mới | PASS |
| Agent độc lập Linux, --once | PASS |
| CTest Debug/Release | 1/1 executable PASS mỗi cấu hình |
| Qt Test | 37 mục PASS = 35 kịch bản chính + setup/cleanup; 0 fail/skip |
| Tích hợp DDS cũ | 14/14 PASS |
| Tích hợp runtime mới | 15 kiểm tra PASS |
| Runtime lấy mẫu tài nguyên | 120 mẫu / khoảng 119,33 giây; tiến trình và GUI tiếp tục hoạt động |
| QML offscreen/resize/chuyển trang/shutdown | PASS trong kịch bản đã chạy |
| Windows, Linux–Windows/NTP vật lý | PENDING_EXTERNAL_TEST |

Bằng chứng ở [test-artifacts/continuation-final](../test-artifacts/continuation-final/). Xem [TEST_RESULTS.md](TEST_RESULTS.md) để biết từng môi trường, ca kiểm thử và giới hạn. Lượt runtime-01 thất bại được giữ nguyên và đã dẫn tới sửa khóa cảnh báo No Data.

RSS Master quan sát 205,16–207,97 MiB, CPU trung bình 16,08% của một logical core khi tự chuyển trang, resize và xuất JSON/PNG. Không suy ra khả năng chạy 24 giờ hoặc nghiệm thu quy mô lớn từ phép đo ngắn này. Kiểm thử 128 host/32 ứng dụng là dữ liệu tổng hợp; queue burst thực sự làm bỏ snapshot theo thiết kế.

## C. Ma trận FR tóm tắt

Ma trận đầy đủ có module, file code, build, test, bằng chứng và giới hạn ở [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md).

| FR | Trạng thái xác thực |
| --- | --- |
| FR-01 | Danh sách/chọn Slave: VERIFIED_LOCAL |
| FR-02 | Online/Offline/Unknown/Recovery: VERIFIED_LOCAL |
| FR-03 | CPU Linux: VERIFIED_LOCAL |
| FR-04 | RAM Linux: VERIFIED_LOCAL; Windows PENDING |
| FR-05 | Ổ đĩa Linux: VERIFIED_LOCAL trong các đường dẫn đã thử |
| FR-06 | Mạng theo interface Linux: VERIFIED_LOCAL; live churn chưa thử |
| FR-07 | Process thật VERIFIED_LOCAL; service transition UNIT; dịch vụ thật/Windows PENDING |
| FR-08 | Collector/parser PARTIAL; NTP hai máy và Windows synchronized chưa xác minh |
| FR-09 | Discovery/entity Linux cục bộ: VERIFIED_LOCAL |
| FR-10 | Runtime mapping/restart có instrumentation: VERIFIED_LOCAL; app thiếu metadata vẫn Unmapped |
| FR-11 | Topology đa ứng dụng cùng máy VERIFIED_LOCAL; liên máy chỉ UNIT/PENDING vật lý |
| FR-12 | Matching thật/freshness và No Data instrumented: VERIFIED_LOCAL trong nguồn đã thử |
| FR-13 | Throughput/Latency VERIFIED_LOCAL; raw model UNIT; aggregate/cumulative còn mở |
| FR-14 | Cảnh báo/khôi phục VERIFIED_LOCAL; policy UNIT; giới hạn overload/cooldown restart còn ghi nhận |
| FR-15 | SQLite worker/history/migration/retention: VERIFIED_LOCAL và SQLite UNIT |

## D. Chạy thử

Lệnh đầy đủ cho Master, Agent, Hello World, runtime instrumentation, Offline/Recovery, Alerts, Time Sync, SQLite và Windows ở [BUILD_AND_RUN.md](BUILD_AND_RUN.md).

Lệnh build và chạy hiện hành dùng artifact persistent đã vá. Khởi chạy Master trong Terminal 1:

~~~bash
cd /home/dev/DDS-Monitor
bash task3/scripts/run_master_patched.sh
~~~

Trong Terminal 2, nhập lại cùng khóa ở prompt ẩn và chạy Agent:

~~~bash
bash task3/scripts/run_linux_agent_patched.sh
~~~

Chọn Slave để xem CPU/RAM, disk/network, ứng dụng và trạng thái thời gian. Dừng Agent bằng Ctrl+C để kiểm tra Offline; chạy lại và nhập cùng khóa để kiểm tra Recovered. Hai wrapper tự chọn build mới nhất, kiểm tra dependency và không chứa sẵn khóa.

Publisher/Subscriber DDS và kiểm tra Alerts/SQLite có lệnh đầy đủ ở [BUILD_AND_RUN.md](BUILD_AND_RUN.md). Cần bật Statistics trong ứng dụng DDS trước khi kỳ vọng Throughput/Latency. Trạng thái Time Sync Unknown hoặc Matching Unknown phản ánh thiếu bằng chứng, không được thay bằng trạng thái thành công.

Windows chạy build-windows.ps1 với QtPrefix và DdsPrefix thực tế, sau đó cấu hình DLL PATH. Không có đường dẫn MSVC/Qt cố định được giả định là đúng cho máy người dùng.

## E. Phần chưa hoàn tất hoặc chưa xác minh

1. Dynamic Chart aggregate/cumulative và tái hiện đầy đủ lỗi PoC; cần regression cho đúng kiểu thống kê trước khi đóng lỗi hoặc thay delay.
2. Windows build/runtime và nguồn trạng thái đồng hồ synchronized đáng tin cậy; cần Windows/MSVC/Qt/Fast DDS thật. Hiện chỉ JSON parser được test trên Linux.
3. Master–Slave vật lý, NTP/Chrony/Windows Time, offset/hội tụ và NIC/firewall; cần môi trường cùng phê duyệt thao tác quản trị. Không thay cấu hình hệ thống trong phiên.
4. Mật độ DDS cao, migration database rất lớn, UX cửa sổ thật, dịch vụ thật trong môi trường được phép và chạy dài theo tiêu chí thống nhất. Không có kết quả 24 giờ.
5. Session/sequence chỉ được theo dõi trong bộ nhớ Master; sau khi Master khởi động lại, bản tin cũ có thể được nhận lại. Telemetry không xác thực nguồn gửi hoặc mã hóa; DDS Security là bước triển khai riêng nếu phạm vi triển khai yêu cầu.
6. App bên thứ ba không instrument vẫn cần GUID cấu hình; không có mapping hoặc No Data giả từ tên/PID/Statistics thiếu.

[CONTINUATION_CHECKPOINT.md](CONTINUATION_CHECKPOINT.md) ghi source root, binary, checkpoint, lệnh và điểm cần tiếp tục; [KNOWN_ISSUES.md](KNOWN_ISSUES.md) ghi chi tiết các giới hạn vận hành.
