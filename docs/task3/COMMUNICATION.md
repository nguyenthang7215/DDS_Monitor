# Giao tiếp telemetry và xác thực

Agent dùng Fast DDS, Topic Task3/HostTelemetry/v1 trên Domain 42 mặc định, tách khỏi Domain thống kê ứng dụng (mặc định 0). Kiểu task3::TelemetryEnvelopeV1 là chuỗi CDR v1 có giới hạn theo task3/Telemetry.idl.

Payload là JSON compact trực tiếp, được truyền trong mẫu CDR qua Topic DDS telemetry riêng. Schema và giới hạn kích thước được kiểm tra ở Master; cơ chế này không cung cấp xác thực nguồn gửi hay mã hóa.

## QoS và tiếp nhận

RELIABLE, VOLATILE, KEEP_LAST; Writer depth 32/max samples 64, Reader depth 256/max samples 512. Writer chặn tối đa 100 ms trong tiến trình Agent. Snapshot đầy đủ được gửi theo chu kỳ nên có thể phục hồi khi mất mẫu mà không cần giao thức đồng bộ riêng.

Callback DDS không trực tiếp cập nhật Registry hoặc QML. Hàng đợi bổ sung có 256 snapshot; quá tải bỏ snapshot cũ, tăng droppedPackets và hiển thị cảnh báo số lượng. Tối đa 32 snapshot được xử lý mỗi lượt giao diện. Đã có kiểm thử quá tải cục bộ; đây không phải xác nhận chống DoS trên mạng không tin cậy.

## Kiểm tra và chống phát lại

Master kiểm tra JSON/schema, kích thước, định danh, miền số, trạng thái, số instance/GUID/counter và sequence thập phân. Session/sequence trong bộ nhớ loại bản tin trùng hoặc cũ trong khi Master đang chạy. Bản tin sai không cập nhật last_seen.

Trong cùng session, sequence phải tăng. Session đã nghỉ bị từ chối. Session mới phải có timestamp lớn hơn timestamp cao nhất đã nhận của máy, kể cả sau timeout hoặc Master restart. Trong cùng session, đồng hồ có thể lùi nhưng sequence vẫn bảo vệ thứ tự. Nếu đồng hồ Agent bị lùi rồi Agent restart, session mới sẽ bị từ chối cho đến khi timestamp vượt checkpoint; cần xử lý nguồn thời gian và dữ liệu checkpoint có kiểm soát.

Checkpoint được lưu trong SQLite cùng transaction metadata. Worker gộp cập nhật khoảng 100 ms; dừng bình thường sẽ flush. Crash/mất điện có thể để lại một khoảng chưa được lưu. Khi database không đọc/khởi tạo được, Master từ chối telemetry thay vì coi checkpoint rỗng là an toàn; sửa database rồi khởi động lại. Trạng thái DDS qua Backend vẫn có thể được quan sát.

128 session nghỉ gần nhất được giữ cùng timestamp cao nhất; session cũ bị loại khỏi danh sách vẫn không vượt được điều kiện timestamp. Registry/checkpoint tối đa 512 máy.

Shared key không phân quyền từng máy: thành viên có khóa có thể giả danh nhau. File runtime identity cũng dựa vào quyền truy cập cục bộ. Với mạng không tin cậy, cần DDS Security và chứng thực riêng từng máy; chưa tích hợp hoặc xác minh cấu hình đó trong phiên này.
