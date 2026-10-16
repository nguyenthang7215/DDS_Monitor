# Bàn giao giao diện DDS Monitor — Đợt 6B

Ngày kiểm chứng: 16/10/2026. Project: `/home/dev/DDS-Monitor`.

## Phạm vi đã triển khai

Một cửa sổ Desktop, ba tab trên cùng: **Giám sát hệ thống**, **DDS chuyên sâu**, **Cảnh báo & lịch sử**. Giao diện mở mặc định bảng tất cả Slave. Sidebar và vùng chi tiết đi theo Slave → Application → DDS Flow; lựa chọn máy và ứng dụng được giữ khi đổi khu vực.

Chi tiết Slave gồm Tài nguyên, Ứng dụng, Luồng DDS và Đồng bộ giờ. CPU dùng lịch sử hiện có; RAM, Disk và Network dùng số liệu collector hiện có. Không bổ sung lịch sử tài nguyên mới. Các trang chi tiết được tải theo tab đang mở; dữ liệu và định danh lựa chọn vẫn nằm trong model/QML hiện có.

Workspace DDS gốc (`Panels`, explorer, Domain Graph, QoS/Status, Dynamic/Historic Charts và các dialog) được giữ một instance, chỉ thay đổi khả năng hiển thị khi điều hướng. Phần giám sát không sao chép explorer hoặc cơ chế biểu đồ DDS. Nút mở DDS chuyên sâu có thể chọn endpoint qua `controller.endpoint_click()` hiện có.

Cảnh báo dùng `MonitoringCore.events` và `loadOlderEvents()` hiện có. Bộ lọc chỉ áp dụng trên các dòng đã tải. Active, Recovered, thời điểm phát sinh/khôi phục và trạng thái chưa xác minh khi Agent Offline được giữ nguyên.

## File nguồn trong đợt này

- `qml/main.qml`: điều hướng ba tab, giữ workspace DDS, tiêu đề và thao tác kiểm thử UI.
- `task3/Dashboard.qml`: bảng tổng quan, sidebar, lựa chọn và định dạng dữ liệu.
- `task3/HostWorkspace.qml`: tài nguyên, ứng dụng, Time Sync; bố cục co giãn.
- `task3/FlowDetails.qml`: tóm tắt luồng và chi tiết theo đối tượng đã chọn.
- `task3/EventHistoryView.qml`: cảnh báo/lịch sử, lọc trên dữ liệu đã tải, tải thêm.
- `task3/CMakeLists.txt`: đăng ký ba QML resource mới.
- `task3/Dashboard.cpp`: tiêu đề DDS Monitor, snapshot trạng thái UI và ảnh kiểm thử; không thay đổi xử lý dữ liệu.
- `task3/tests/ui_integration.py`: kiểm thử cửa sổ Qt với Agent/Publisher/Subscriber thật và state riêng.

Checksum trước/sau của Collectors, Protocol, Registry, DdsTransport, DdsBridge, FlowModel, RuntimeIdentity, MonitoringCore, AlertStore và thư viện Statistics Backend đã vá khớp nhau. Xem [danh sách checksum](../../test-artifacts/ui-6b-20261016/protected-sha256.txt). Bản sao các file giao diện trước khi sửa nằm tại `/tmp/dds-ui6b-baseline.TTyccv`.

## Kết quả kiểm chứng

| Kiểm tra | Kết quả | Bằng chứng |
| --- | --- | --- |
| Debug build / CTest | PASS / 1 target PASS | [build](../../test-artifacts/ui-6b-20261016/debug-build.log), [CTest](../../test-artifacts/ui-6b-20261016/debug-ctest.log) |
| Release build / CTest | PASS / 1 target PASS | [build](../../test-artifacts/ui-6b-20261016/release-build.log), [CTest](../../test-artifacts/ui-6b-20261016/release-ctest.log) |
| Local integration | 14/14 PASS | [kết quả](../../test-artifacts/ui-6b-20261016/local-14/results.json) |
| Continuation integration | 15/15 PASS; đoạn theo dõi ổn định 30 giây | [kết quả](../../test-artifacts/ui-6b-20261016/continuation-final/results.json) |
| UI integration trên X11 | 15/15 PASS | [kết quả](../../test-artifacts/ui-6b-20261016/desktop-delivery/results.json), [quan sát điều hướng](../../test-artifacts/ui-6b-20261016/desktop-delivery/ui-observations.json) |
| QML, SQLite integrity, shutdown | PASS | Các integration trên; mọi tiến trình của bài UI thoát mã 0 |

UI integration dùng cửa sổ X11 1600×900, hai Agent trên cùng máy Linux, Publisher/Subscriber và một Host cấu hình chưa nhận telemetry. Continuation kiểm tra resize 900×650 / 1280×820, restart Agent/Master, mapping, matching, No Data/Recovery, SQLite và shutdown. Runtime maps xác nhận dùng Backend tại prefix đã vá.

Các thư mục `desktop`, `desktop-repro`, `desktop-layout-check` giữ lại lần kiểm tra trung gian chưa đạt; `continuation-15` là lượt chạy chung bị ngắt và không có kết luận PASS. Kết quả bàn giao lấy từ `desktop-delivery` và `continuation-final`.

## Ảnh chụp giao diện thực tế

- [Giám sát — tổng quan tất cả Slave](../../test-artifacts/ui-6b-20261016/desktop-delivery/overview.png).
- [DDS chuyên sâu — workspace gốc](../../test-artifacts/ui-6b-20261016/desktop-delivery/dds-online.png).
- [Cảnh báo — Active và Recovered](../../test-artifacts/ui-6b-20261016/desktop-delivery/events-recovery.png).
- Chi tiết: [tài nguyên](../../test-artifacts/ui-6b-20261016/desktop-delivery/resources-online.png), [Network](../../test-artifacts/ui-6b-20261016/desktop-delivery/network-online.png), [ứng dụng](../../test-artifacts/ui-6b-20261016/desktop-delivery/system-1-online.png), [luồng DDS](../../test-artifacts/ui-6b-20261016/desktop-delivery/system-2-online.png), [đồng bộ giờ](../../test-artifacts/ui-6b-20261016/desktop-delivery/system-3-online.png).
- Trạng thái bất thường: [Host Offline](../../test-artifacts/ui-6b-20261016/desktop-delivery/host-offline.png), [cảnh báo chưa xác minh](../../test-artifacts/ui-6b-20261016/desktop-delivery/events-unverified.png).

## Giới hạn

Kết quả là kiểm thử Linux cục bộ, không phải nhiều máy vật lý. Windows runtime, NTP liên máy và tải lớn với nhiều Slave vẫn PENDING_EXTERNAL_TEST. Các tên Topic/Participant do ứng dụng kiểm thử cung cấp được giữ nguyên; nhãn sản phẩm và tiêu đề cửa sổ dùng DDS Monitor. Không cấu hình OS/NTP, cài dependency, thay đổi backend hoặc bắt đầu Đợt 6C.
