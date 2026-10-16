# Kế hoạch tiếp nối và checkpoint

Source duy nhất được phát triển trong đợt này là implementation/ của workspace báo cáo. Repo /home/dev/DDS-Monitor không được chuyển thành source Task 3. Solutions docs/TASK3_REQUIREMENTS.md được giữ nguyên; SHA-256: 57e0de67e06239e22b558e441e010add14f8c79a7e324fa63835adc39e1eb47c.

Bản sao triển khai trước đó chưa có Git. Đã khởi tạo repository cục bộ và tạo checkpoint 3881967 trước khi sửa các module; một commit tiếp theo bổ sung các asset upstream chưa được theo dõi. Không reset, clean hoặc xóa source/artifact cũ. Git diff hiện biểu diễn phần tiếp tục phát triển so với checkpoint này, không phải diff trực tiếp với upstream 94bbd6d.

| Giai đoạn | Công việc đã thực hiện | Điểm còn mở |
| --- | --- | --- |
| A | Đọc yêu cầu/tài liệu, đối chiếu source/CMake/API, checkpoint, Release/CTest và tích hợp baseline | Không có Windows/máy thứ hai |
| B | Instance/runtime GUID, topology ConnectionList, luồng periodic/event, service/process adapter | Instrumentation bắt buộc cho ánh xạ chắc chắn; dịch vụ thật chưa được thay đổi |
| C | Một Main Window, nhóm ứng dụng/Participant, topology, sửa raw Dynamic Chart bằng snapshot | Gộp/cumulative chart còn mở; UX cửa sổ thật cần rà soát |
| D | Chrony validation, Windows CIM adapter, SQLite worker, retention, debounce/cooldown/hysteresis, replay và queue bounds | Windows clock synchronization chưa đủ bằng chứng; crash persistence có khoảng chưa commit |
| E | Agent standalone, script build Windows có prefix cấu hình, hướng dẫn vận hành | Build/runtime Windows chờ môi trường |
| F | Build Debug/Release mới, unit/regression, tích hợp cũ và mới, QML/resize/shutdown, resource sampling, tài liệu/FR matrix | Nghiệm thu vật lý, 24 giờ và hiệu chuẩn chưa thực hiện |

Bằng chứng baseline ở test-artifacts/continuation-baseline/. Lần tích hợp runtime-01 đã tìm ra lỗi No Data không khôi phục khi GUID đổi; giữ nguyên artifact thất bại đó. runtime-02 đã kiểm tra bản sửa. Kết quả cuối và các lệnh chạy nằm trong TEST_RESULTS.md/BUILD_AND_RUN.md. Không tự giảm yêu cầu FR để đổi trạng thái còn mở thành hoàn tất.
