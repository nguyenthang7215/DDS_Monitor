# Kế hoạch tiếp nối và checkpoint

Cơ chế trong các mốc lịch sử bên dưới không phải hướng dẫn vận hành hiện hành: HMAC/TASK3_KEY và replay checkpoint SQLite đã được loại bỏ. Source hiện giữ session/sequence trong bộ nhớ và Event History trong SQLite; xem COMMUNICATION.md và BUILD_AND_RUN.md.

## Trạng thái hiện tại sau khi tích hợp

Repository nguồn chính thức hiện là `/home/dev/DDS-Monitor`; mã Task 3, CMake, script, cấu hình, kiểm thử, patch và tài liệu đã được tích hợp tại đó. Tài liệu này được giữ lại từ workspace triển khai trước để bảo toàn lịch sử kế hoạch, không còn mô tả vị trí source hiện hành. Chi tiết trạng thái và kết quả xác minh trên repo chính xem `IMPLEMENTATION_STATUS.md`, `TEST_RESULTS.md` và `PRE_TEST_READINESS.md`.

`docs/TASK3_REQUIREMENTS.md` từ workspace báo cáo được giữ nguyên khi tích hợp; SHA-256: `57e0de67e06239e22b558e441e010add14f8c79a7e324fa63835adc39e1eb47c`.

## Lịch sử checkpoint trước tích hợp

Bản sao triển khai trước đó chưa có Git. Đã khởi tạo repository cục bộ và tạo checkpoint 3881967 trước khi sửa các module; một commit tiếp theo bổ sung các asset upstream chưa được theo dõi. Không reset, clean hoặc xóa source/artifact cũ. Git diff của bản sao đó biểu diễn phần tiếp tục phát triển so với checkpoint này, không phải diff trực tiếp với upstream 94bbd6d. Bản sao và checkpoint nguồn được giữ nguyên để truy vết; các thay đổi đã xác minh được tích hợp riêng vào repository chính.

| Giai đoạn | Công việc đã thực hiện | Điểm còn mở |
| --- | --- | --- |
| A | Đọc yêu cầu/tài liệu, đối chiếu source/CMake/API, checkpoint, Release/CTest và tích hợp baseline | Không có Windows/máy thứ hai |
| B | Instance/runtime GUID, topology ConnectionList, luồng periodic/event, service/process adapter | Instrumentation bắt buộc cho ánh xạ chắc chắn; dịch vụ thật chưa được thay đổi |
| C | Một Main Window, nhóm ứng dụng/Participant, topology, sửa raw Dynamic Chart bằng snapshot | Gộp/cumulative chart còn mở; UX cửa sổ thật cần rà soát |
| D | Chrony validation, Windows CIM adapter, SQLite worker, retention, debounce/cooldown/hysteresis, replay và queue bounds | Windows clock synchronization chưa đủ bằng chứng; crash persistence có khoảng chưa commit |
| E | Agent standalone, script build Windows có prefix cấu hình, hướng dẫn vận hành | Build/runtime Windows chờ môi trường |
| F | Build Debug/Release mới, unit/regression, tích hợp cũ và mới, QML/resize/shutdown, resource sampling, tài liệu/FR matrix | Nghiệm thu vật lý, 24 giờ và hiệu chuẩn chưa thực hiện |

Bằng chứng baseline ở test-artifacts/continuation-baseline/. Lần tích hợp runtime-01 đã tìm ra lỗi No Data không khôi phục khi GUID đổi; giữ nguyên artifact thất bại đó. runtime-02 đã kiểm tra bản sửa. Kết quả cuối và các lệnh chạy nằm trong TEST_RESULTS.md/BUILD_AND_RUN.md. Không tự giảm yêu cầu FR để đổi trạng thái còn mở thành hoàn tất.
