# Hoàn thiện UI/UX DDS Monitor — Đợt 6C

Ngày kiểm chứng: 16/10/2026. Project: `/home/dev/DDS-Monitor`.

## Phạm vi và file đã sửa

Giữ nguyên ba tab Hybrid, luồng Slave → Application → DDS Flow và một instance workspace DDS gốc. Không thêm chức năng giám sát, Qt module hoặc thư viện.

- `qml/main.qml`: tăng độ rõ của tab; ẩn Domain/Topic kỹ thuật trong tiêu đề khi telemetry khởi tạo thành công. Lỗi khởi tạo vẫn lấy từ trạng thái hiện có.
- `task3/Dashboard.qml`: chữ chính 14 px, chữ phụ 12–13 px; căn thống nhất cột bảng, nền dòng xen kẽ, bốn ô tổng quan cân bằng. Vùng chọn Slave có vạch đánh dấu; Application dùng nền nhẹ và chữ đậm. Định danh dài được elide, kèm tooltip; timestamp hiển thị gọn theo giờ máy Master.
- `task3/HostWorkspace.qml`: thu gọn thẻ CPU/RAM, tăng cỡ nhãn Disk/Network, căn các cột số liệu; ứng dụng dùng bố cục ngang khi đủ chỗ, dọc ở cửa sổ hẹp. Sửa vùng Network bị co chiều cao về 0 khi đổi tab.
- `task3/FlowDetails.qml`: làm rõ dòng đang chọn; giới hạn phần trạng thái, chia đều ba vùng nguồn–Topic–đích; chuỗi dài xuống dòng trong chi tiết, timestamp dễ đọc hơn. Vùng chi tiết vẫn cuộn được khi nội dung dài.
- `task3/EventHistoryView.qml`: tách Active/Recovered khỏi severity; phân biệt mức độ bằng chữ và màu, thêm vạch trạng thái và nền dòng xen kẽ; thời điểm bắt đầu/khôi phục dùng cột cố định, định dạng ngắn.
- `task3/Dashboard.cpp`: chỉ bổ sung hỗ trợ kiểm thử UI: kích thước cửa sổ theo biến môi trường khi bật exercise, geometry danh sách Network trong snapshot và xuất ảnh atomic bằng `QSaveFile`.
- `task3/tests/ui_integration.py`: thêm tham số kích thước, định danh dài, kiểm tra geometry Network và toàn vẹn/kích thước PNG bằng thư viện chuẩn Python.

Backend, Collector, Protocol, Registry, SQLite và NTP không sửa. [Checksum các thành phần được bảo vệ](../../test-artifacts/ui-6c-20261016/protected-sha256.txt) và [kết quả đối chiếu](../../test-artifacts/ui-6c-20261016/protected-check.log) khớp baseline. Chỉ `qml/main.qml` thay đổi trong cây QML gốc; không đổi `Panels`, explorer hoặc chart gốc. Các thay đổi chưa commit từ những đợt trước được giữ lại; không commit/push.

## Lỗi thực tế đã xử lý

1. **Network trống dù có telemetry:** snapshot đo được 6 interface, content height 426 px, nhưng ListView cao 0 px trong ColumnLayout lồng dưới StackLayout. Đưa ListView trực tiếp vào StackLayout, dùng header cho chú thích, khắc phục lỗi hiển thị mà không đổi dữ liệu. Regression mới yêu cầu vùng danh sách có kích thước hữu ích và có dòng dữ liệu.
2. **Ảnh DDS bị cắt giữa chừng:** công cụ sao chép ảnh có thể đọc khi ứng dụng đang ghi PNG. Xuất ảnh qua `QSaveFile` tránh file dở dang; integration kiểm tra CRC từng chunk, IEND và kích thước ảnh. Đây là sửa đường xuất ảnh kiểm thử, không phải xử lý DDS.
3. **Thẻ RAM bị co khi resize 900×650:** dòng Used/Total bị cắt trong ảnh kiểm tra bổ sung. Đặt chiều cao tối thiểu 104 px cho hàng CPU/RAM; [ảnh resize sau sửa](../../test-artifacts/ui-6c-20261016/continuation-final/dashboard.png.network.png) hiển thị đủ dòng này. Danh sách interface vẫn cuộn theo không gian còn lại.

Các thư mục `desktop-1280-long-initial`, `network-diagnostic`, `desktop-1280-long`, `network-ancestors` và `desktop-1280-long-final` giữ bằng chứng trung gian, không phải bộ ảnh bàn giao. Các thư mục `pre-height-*` lưu lượt trước bổ sung giới hạn chiều cao thẻ RAM. Bộ ảnh bàn giao cuối nằm trong hai thư mục `desktop-*-verified`, đã kiểm tra PNG đầy đủ.

## Kết quả trên artifact cuối

| Kiểm tra | Kết quả và bằng chứng |
| --- | --- |
| Debug build / CTest | PASS, 1/1 target — [build](../../test-artifacts/ui-6c-20261016/build-debug.log), [CTest](../../test-artifacts/ui-6c-20261016/ctest-debug.log) |
| Release build / CTest | PASS, 1/1 target — [build](../../test-artifacts/ui-6c-20261016/build-release.log), [CTest](../../test-artifacts/ui-6c-20261016/ctest-release.log) |
| UI X11, 1600×900 | 17/17 PASS — [kết quả](../../test-artifacts/ui-6c-20261016/desktop-1600-verified/results.json) |
| UI X11, 1280×800, định danh dài | 17/17 PASS — [kết quả](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/results.json) |
| Local integration | 14/14 PASS — [kết quả](../../test-artifacts/ui-6c-20261016/local-final/results.json) |
| Continuation cuối | 15/15 PASS, theo dõi ổn định 30 giây — [kết quả](../../test-artifacts/ui-6c-20261016/continuation-final/results.json) |

Hai lượt UI chạy hai Agent cùng máy Linux, Publisher/Subscriber thật và một Host cấu hình chưa có phản hồi. Kiểm tra ba tab, giữ lựa chọn máy/ứng dụng, một workspace DDS, Network, Online/Offline/Unknown, cảnh báo chưa xác minh, Recovery, SQLite integrity và shutdown. Mọi tiến trình trong hai lượt UI thoát mã 0. Runtime maps trong từng thư mục xác nhận thư viện Statistics Backend tại prefix đã vá, không dùng `/tmp/dds-p0`.

Continuation thực hiện resize luân phiên 900×650 / 1280×820 trên cùng cửa sổ, restart Agent/Master, DDS matching, No Data/Recovery, lưu sự kiện và shutdown. Log QML cuối không có lỗi được bộ kiểm tra phát hiện. Đã xem ảnh ba tab và chi tiết ở hai kích thước mục tiêu; dùng elide/tooltip hoặc cuộn cho nội dung dài, không cố dồn mọi dòng vào một màn hình.

## Ảnh thực tế để đánh giá

Ở **1600×900**:

- Ba tab: [Giám sát hệ thống](../../test-artifacts/ui-6c-20261016/desktop-1600-verified/overview.png), [DDS chuyên sâu](../../test-artifacts/ui-6c-20261016/desktop-1600-verified/dds-online.png), [Cảnh báo & lịch sử](../../test-artifacts/ui-6c-20261016/desktop-1600-verified/events-recovery.png).
- Chi tiết: [Slave/tài nguyên](../../test-artifacts/ui-6c-20261016/desktop-1600-verified/resources-online.png), [Network](../../test-artifacts/ui-6c-20261016/desktop-1600-verified/network-online.png), [Application](../../test-artifacts/ui-6c-20261016/desktop-1600-verified/system-1-online.png), [DDS Flow](../../test-artifacts/ui-6c-20261016/desktop-1600-verified/system-2-online.png), [Time Sync](../../test-artifacts/ui-6c-20261016/desktop-1600-verified/system-3-online.png).

Ở **1280×800**, dùng định danh dài:

- Ba tab: [Giám sát hệ thống](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/overview.png), [DDS chuyên sâu](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/dds-online.png), [Cảnh báo & lịch sử](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/events-recovery.png).
- Chi tiết: [Slave/tài nguyên](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/resources-online.png), [Network](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/network-online.png), [Application](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/system-1-online.png), [DDS Flow](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/system-2-online.png).
- Bất thường: [Host Offline](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/host-offline.png), [cảnh báo chưa xác minh](../../test-artifacts/ui-6c-20261016/desktop-1280-verified/events-unverified.png).

Ảnh DDS chuyên sâu thể hiện workspace gốc sau discovery, không phải một bài kiểm thử mới toàn bộ Dynamic/Historic Charts. Tên `Task3 Runtime Probe` và `Task3/RuntimeProbe` trong dữ liệu thử là tên Participant/Topic thật; không đổi hoặc che chúng để làm đẹp ảnh. Matching chưa đủ bằng chứng vẫn hiển thị Unknown, không biến thành mũi tên xác nhận.

## Giới hạn

Đã quan sát ảnh ở hai kích thước Desktop với màn hình Linux hiện tại. Chưa xác minh HiDPI/font scaling khác, tải nhiều Slave thực tế, Windows runtime hoặc NTP liên máy; các phần này vẫn PENDING_EXTERNAL_TEST. Không cấu hình OS, đồng hồ, firewall hoặc dependency. Đợt này chỉ hoàn thiện UI và kiểm thử hồi quy, chưa bắt đầu nghiệm thu hệ thống; đánh giá thẩm mỹ cuối cùng chờ người dùng xem ảnh.
