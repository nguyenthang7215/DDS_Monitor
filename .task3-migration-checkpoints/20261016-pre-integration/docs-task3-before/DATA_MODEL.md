# Mô hình dữ liệu

## Máy, ứng dụng và lần chạy

host_id là định danh máy ổn định, độc lập với IP; application_id là định danh logic của ứng dụng. session_id và sequence thuộc phiên Agent. sequence là chuỗi số nguyên không dấu, tránh mất chính xác do số JSON.

applications[].instances[] liệt kê PID, instance_id, state, participant_guids, binding_source và writer_counts. Linux dùng boot_id:start_ticks để xác định lần chạy; Windows dùng thời điểm tạo tiến trình FILETIME. PID và instance_id ngoài danh sách là một tiến trình đại diện thật cho consumer schema 1 cũ; danh sách instances mới chứa thông tin đầy đủ.

Manifest runtime được chấp nhận khi:
- application_id khớp cấu hình;
- PID và định danh khởi chạy khớp hệ điều hành;
- tuổi bản ghi từ -2 đến 10 giây;
- không có bản ghi trùng gây mâu thuẫn.

Manifest chỉ là bằng chứng từ instrumentation cục bộ đáng tin cậy. Thư mục phải giới hạn quyền ghi. Tên giống nhau hoặc PID đơn lẻ không đủ ánh xạ. GUID cấu hình tĩnh vẫn là cơ chế thủ công; phải cập nhật khi ứng dụng restart. Trường hợp thiếu metadata hiển thị mapping_state=unknown, tức Unmapped.

writer_counts là object GUID Writer → chuỗi bộ đếm lần ghi thành công. Bộ đếm chỉ được gắn với cùng instance; giảm bộ đếm hoặc đổi instance sẽ khởi tạo lại thời gian quan sát. Đây không phải Throughput và không chứng minh giao hàng tới Reader.

## Snapshot schema 1

Các trường chính: schema_version, type, host_id, hostname, os, dds_host_identifier, session_id, sequence, timestamp_ms, metrics, applications, time_sync và collector_health. Các trường instance/runtime mới là mở rộng tùy chọn; envelope và HMAC không đổi.

CPU tính từ chênh lệch giữa hai lần đọc, mẫu đầu là null. RAM và dung lượng ổ đĩa dùng byte. Tốc độ mạng là byte/giây theo từng interface; rate_valid=false khi chưa có khoảng so sánh hoặc bộ đếm bị đặt lại. collector_health phân biệt nguồn không khả dụng với phép đo bằng 0.

time_sync chứa state, service_running, source, source_valid, source_reachable, clock_synced, offset_seconds, checked_ms và detail khi có bằng chứng tương ứng. Giá trị chưa biết là null/Unknown; không đổi offset thiếu dữ liệu thành 0. Offset Windows từ performance counter là ước lượng độ lệch tuyệt đối; chưa chứng minh đồng hồ synchronized.

## Trường do Master tính

host_state, online, metrics_state, last_seen, cpu_history, alert_count và address. Máy khai báo trước nhưng chưa nhận telemetry hợp lệ vẫn Unknown. Offline chỉ cho biết Agent không phản hồi đúng hạn, không xác định được nguyên nhân máy tắt/mạng/Agent.

Dữ liệu DDS có id, kind, GUID, metadata máy/tiến trình, Participant cha, Topic, alive, QoS, trạng thái ánh xạ, ConnectionList GUID và timestamp, Throughput, mẫu thô và latency đã ghi nhận. Đơn vị raw/normalized/display của Throughput được lưu riêng.

flows chứa Writer/Reader ID, máy và ứng dụng hai đầu, Topic, match_state, confirmed, flow_state và phép đo. Chỉ confirmed=true mới tạo mũi tên; quan hệ Backend hoặc phép đo lịch sử không tự xác nhận matching hiện tại.

## Sự kiện và checkpoint

Mỗi chuyển đổi Active/Recovered có rule_id, alert_id, kind, source, severity, message, triggered_ms, recovered_ms và time. Cùng trạng thái không tạo sự kiện lặp.

SQLite giữ bảng events theo schema cũ, thêm alert_state và metadata. Checkpoint lưu session hiện tại, sequence cao nhất, timestamp cao nhất và tối đa 128 session nghỉ/máy. Việc lưu checkpoint được gộp theo chu kỳ worker; flush bình thường trước khi thoát.
