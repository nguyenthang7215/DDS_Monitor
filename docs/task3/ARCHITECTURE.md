# Kiến trúc hệ thống sau đợt hoàn thiện

Repository chính thức là `/home/dev/DDS-Monitor`, phát triển từ eProsima DDS Monitor 4.2.0. Task 3 được gắn vào ứng dụng Qt/QML gốc thông qua target `task3/` và trang Dashboard trong cùng Main Window; Engine, Controller và màn hình DDS gốc vẫn được giữ. Không thay toàn bộ ứng dụng bằng một project Qt độc lập.

Source phát triển hiện nằm trong `/home/dev/DDS-Monitor`, được mở rộng từ eProsima DDS Monitor. Một Main Window có ba tab trên cùng: Giám sát hệ thống, DDS chuyên sâu, Cảnh báo & lịch sử. Engine, Controller, Statistics Backend, topology, QoS và biểu đồ gốc vẫn được tái sử dụng.

## Thành phần và luồng dữ liệu

1. Agent chạy trong tiến trình riêng, lấy tài nguyên và trạng thái tiến trình/dịch vụ của hệ điều hành.
2. Agent gửi snapshot JSON schema 1 qua Topic DDS riêng; Domain mặc định là 42.
3. Callback DDS đưa bản tin vào hàng đợi có giới hạn. Một tác vụ Qt lấy tối đa 32 snapshot mỗi lượt và cập nhật Registry/MonitoringCore trên luồng giao diện.
4. DdsBridge truy vấn Statistics Backend bằng QtConcurrent, tối đa một job đang chạy. Registry ánh xạ Participant trước rồi chuyển liên kết cho Writer/Reader.
5. FlowModel dựng quan hệ máy/ứng dụng/Writer/Topic/Reader. Chỉ vẽ mũi tên khi có ConnectionList còn mới và GUID đối tác tương ứng.
6. AlertStore giữ cache sự kiện trên luồng giao diện; một worker QThread riêng sở hữu duy nhất kết nối SQLite, thực hiện transaction và thử lại khi khóa. Không lưu checkpoint replay.
7. Đồng bộ đồng hồ chạy độc lập bằng dịch vụ hệ điều hành. Ứng dụng chỉ đọc trạng thái.

## Định danh và luồng dữ liệu

RuntimeIdentity là instrumentation tùy chọn cho ứng dụng DDS. Ứng dụng ghi manifest gồm PID, định danh khởi chạy do hệ điều hành cấp và các GUID đang dùng. Agent đọc lại manifest mỗi chu kỳ và đối chiếu với tiến trình hiện có. Một ứng dụng có thể có nhiều process instance; mỗi instance có nhiều Participant. Thiếu hoặc mâu thuẫn metadata thì giữ Unmapped.

Với instrumentation, bộ đếm lần ghi thành công của Writer giúp phân biệt ứng dụng còn chạy nhưng không phát. Quy tắc chu kỳ được áp dụng cho luồng logic máy/ứng dụng/Topic: chỉ cảnh báo khi không Writer được instrument nào ghi mới trong deadline. Khóa này ổn định qua thay đổi PID/GUID. Bộ đếm ghi không chứng minh Reader đã xử lý dữ liệu. Topic theo sự kiện được phép im lặng.

Không có instrumentation thì chỉ kết luận No Data theo quy tắc thận trọng tất cả Writer đã phát hiện đều inactive trong timeout. Thiếu Statistics là Stale/Unavailable riêng biệt. Matching trở lại Unknown nếu ConnectionList không còn hiệu lực sau 15 giây.

## Luồng xử lý và giới hạn

- Envelope: 48.000 byte; tối đa 128 ứng dụng, 128 instance/ứng dụng, 32 GUID và 64 bộ đếm/instance. Giới hạn tổng envelope vẫn được ưu tiên.
- Registry: tối đa 512 máy, sequence hiện tại và 256 session nghỉ gần nhất/máy trong bộ nhớ. Không dùng đồng hồ Slave làm điều kiện chấp nhận session mới; giới hạn lọc replay được mô tả trong COMMUNICATION.md.
- Hàng đợi callback DDS: 256 snapshot, bỏ bản cũ khi quá tải và công khai bộ đếm bỏ. DDS Reader vẫn có history giới hạn riêng.
- SQLite: 4096 sự kiện chờ; trạng thái cảnh báo giới hạn 8192; ban đầu tải 500 sự kiện, có tải thêm lịch sử. Khi không nhận thêm được chuyển đổi, báo lỗi và thử lại nếu điều kiện được đánh giá tiếp.
- DdsBridge: 4096 dòng thực thể, tối đa 64 quan hệ Reader/Writer, topology tối đa 8192 dòng. Mặc định hiển thị 256 mẫu trên cửa sổ 120 giây. Backend có thể cấp phát nhiều mẫu thô trước khi cắt giới hạn hiển thị; chưa có bảo đảm bộ nhớ cứng cho mọi mật độ thống kê.
- CPU history giữ 120 điểm. Marker thời gian nhận và timeout dùng đồng hồ monotonic của Master.

## Lưu trữ và shutdown

Migrate database chạy trên worker và được chờ lúc khởi động, trước khi hiển thị; database cũ lớn có thể kéo dài thời gian này. Retention mặc định tắt. Khi bật, worker xóa theo lô các cặp Active/Recovered đã hết hạn, giữ nguyên cảnh báo đang hoạt động.

SIGINT/SIGTERM được chuyển thành yêu cầu Qt quit. Trước khi Engine bị hủy, hệ thống dừng polling, đợi job Backend, đóng telemetry và flush SQLite. Nếu database lỗi hoặc không ghi được lúc thoát thì ghi lỗi rõ ràng; không có cam kết lưu thành công lên thiết bị lưu trữ đang hỏng.

Nhánh NONE của Dynamic Chart gốc đọc lại cửa sổ hiển thị và thay snapshot DataModel. Nhánh gộp/tích lũy vẫn đang mở. delay_time=30000 ms tiếp tục là cấu hình thử nghiệm PoC.
