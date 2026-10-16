# Xác minh đơn vị thống kê

Phạm vi: Fast DDS 3.6.1 và Statistics Backend 2.5.0 đang cài, được kiểm tra ngày 16-10-2026. Không áp dụng kết luận này cho phiên bản khác nếu chưa xác minh lại.

## Throughput: đường đi dữ liệu đã xác minh

1. `Fast-DDS/src/cpp/rtps/writer/StatefulWriter.cpp` và `StatelessWriter.cpp` truyền `payload_length` vào `on_publish_throughput()`.
2. `Fast-DDS/src/cpp/statistics/rtps/writer/StatisticsWriterImpl.cpp:209` tính `notification.data(payload / duration<float>(current_timepoint - former_timepoint).count())`.
3. `StatisticsReaderImpl.cpp:148` dùng phép tính tương ứng cho dữ liệu nhận.
4. Backend `src/cpp/database/database_queue.cpp:561` gán `sample.data = item.data()` mà không đổi thang đơn vị Throughput.
5. Backend lưu và truy xuất giá trị `EntityDataSample` mà không đổi độ lớn Throughput.
6. `DDS Monitor SyncBackendConnection::change_unit_magnitude()` chỉ đổi thang Latency; `get_data_kind_units()` gắn nhãn Throughput là B/s.

Với đúng các phiên bản này, giá trị Throughput tại đường API được kiểm tra có đơn vị byte/giây (B/s), dù chú thích trong `Backend types/types.hpp` ghi Mb/s. Không nhân giá trị này với 125000. Kiểm thử Hello World trực tiếp ghi lại giá trị API thô và nhãn B/s trong `throughput_measurement`.

`task3/Units.h` có kiểm thử riêng phép đổi Mb/s SI: 1 Mb/s = 125000 B/s. Phép đổi này không được tự động áp dụng lên giá trị Backend đang cài.

## Latency

Trên đường xử lý được kiểm tra, dữ liệu thống kê có thang nanô giây. Monitor gốc chia cho 1000 trước khi hiển thị µs. `DdsBridge` gọi trực tiếp Backend nên cũng chia đúng một lần cho 1000; không gọi wrapper đã chuẩn hóa của bản gốc rồi chia lần nữa.

Các dòng mẫu mới giữ timestamp đo. Latency ghi nhận giữa Writer–Reader không chứng minh hai endpoint đang ghép cặp ở hiện tại; Network Latency và Fast DDS Latency là hai loại dữ liệu khác nhau.

## Phạm vi kết luận

Kiểm thử đơn vị xác nhận phép đổi và nhãn hiển thị; tích hợp cục bộ xác nhận có mẫu Throughput/Latency thật. Chưa dùng thiết bị đo thời gian độc lập hoặc hiệu chuẩn băng thông giữa các máy. Độ chính xác trên máy vật lý khác vẫn cần kiểm chứng. Kiểm tra mã nguồn chỉ xác lập đơn vị cho đúng phiên bản đã dùng, không chứng minh API luôn có cùng đơn vị ở mọi phiên bản.

## Điều tra Dynamic Chart trong đợt tiếp tục

DynamicDataChartBox trước đây luôn đẩy time_from tới time_to sau mỗi poll, kể cả khi Backend chưa trả mẫu. Mẫu đến muộn với timestamp thuộc khoảng đã bỏ qua không được truy vấn lại. Engine hiện đọc lại cửa sổ hiển thị cho StatisticKind::NONE; DynamicDataChartBox thay snapshot DataModel thay vì append kết quả overlap. DataModel sắp thứ tự ổn định, loại số không hữu hạn, giữ mẫu khác nhau cùng timestamp và giới hạn số điểm (mặc định 10000 nếu không đặt max_points).

legacyDynamicRawSnapshotRegression chạy chính các lớp gốc để kiểm tra mẫu muộn, thứ tự và không nhân đôi giữa các poll. Đây là bằng chứng cho nhánh raw/model; chưa tái hiện đầy đủ mọi cấu hình biểu đồ PoC. Nhánh aggregate/cumulative còn mở và delay_time=30000 ms giữ nguyên. Không nâng delay để coi như đã sửa nguyên nhân.
