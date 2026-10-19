# Giao tiếp telemetry và kiểm tra bản tin

Agent dùng Fast DDS, Topic Task3/HostTelemetry/v1 trên Domain 42 mặc định, tách khỏi Domain thống kê ứng dụng (mặc định 0). Kiểu task3::TelemetryEnvelopeV1 là chuỗi CDR v1 có giới hạn theo task3/Telemetry.idl.

Payload là JSON compact trực tiếp, được truyền trong mẫu CDR qua Topic DDS telemetry riêng. Schema và giới hạn kích thước được kiểm tra ở Master; cơ chế này không cung cấp xác thực nguồn gửi hay mã hóa.

## QoS và tiếp nhận

RELIABLE, VOLATILE, KEEP_LAST; Writer depth 32/max samples 64, Reader depth 256/max samples 512. Writer chặn tối đa 100 ms trong tiến trình Agent. Snapshot đầy đủ được gửi theo chu kỳ nên có thể phục hồi khi mất mẫu mà không cần giao thức đồng bộ riêng.

Callback DDS không trực tiếp cập nhật Registry hoặc QML. Hàng đợi bổ sung có 256 snapshot; quá tải bỏ snapshot cũ, tăng droppedPackets và hiển thị cảnh báo số lượng. Tối đa 32 snapshot được xử lý mỗi lượt giao diện. Đã có kiểm thử quá tải cục bộ; đây không phải xác nhận chống DoS trên mạng không tin cậy.

## Kiểm tra và chống phát lại

Master kiểm tra JSON/schema, kích thước, định danh, miền số, trạng thái, số instance/GUID/counter và sequence thập phân. Session/sequence trong bộ nhớ loại bản tin trùng hoặc cũ trong khi Master đang chạy. Bản tin sai không cập nhật last_seen.

Trong cùng session, sequence phải tăng. Registry giữ tối đa 256 session đã nghỉ gần nhất mỗi máy và từ chối bản tin từ các session này, kể cả khi sequence hoặc timestamp lớn hơn. Session chưa từng thấy được tiếp nhận nếu schema/sequence hợp lệ; đồng hồ Slave lùi không cản Agent restart và Recovery. Timeout/Last Seen dùng thời điểm nhận tại Master, không dùng timestamp của Slave để quyết định Online.

Không còn HMAC/TASK3_KEY hoặc checkpoint replay lưu bền. SQLite chỉ lưu lịch sử/trạng thái cảnh báo và metadata migration. Database lỗi không chặn telemetry nhưng không bảo đảm lưu sự kiện; giao diện hiển thị lỗi Event database. Dữ liệu checkpoint cũ trong database, nếu có, không bị xóa và không còn được sử dụng.

Registry tối đa 512 máy. Bộ nhớ session bị xóa khi Master restart; session đã ra ngoài cửa sổ 256 hoặc chưa từng được Master quan sát không thể phân biệt chắc chắn với session mới khi đồng hồ có thể lùi. Đây là lọc bản tin cũ/trùng trong mạng tin cậy, không phải cơ chế xác thực hay chống replay bảo mật.

Agent/Master không yêu cầu shared key. File runtime identity dựa vào quyền truy cập cục bộ. Chỉ dùng trên mạng DDS tin cậy; DDS Security/chứng thực cho mạng không tin cậy chưa nằm trong phạm vi đã kiểm chứng.
