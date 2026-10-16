# Phần mở rộng Task 3 của DDS Monitor

Bắt đầu từ [hướng dẫn chạy](docs/BUILD_AND_RUN.md), [báo cáo bàn giao](docs/DELIVERY_REPORT.md), [ma trận FR](docs/IMPLEMENTATION_STATUS.md), [kết quả kiểm thử](docs/TEST_RESULTS.md) và [vấn đề còn mở](docs/KNOWN_ISSUES.md).

Source phát triển nằm trong thư mục implementation này; có repository Git cục bộ với checkpoint 3881967/52bbdea. Repo /home/dev/DDS-Monitor không phải bản source chứa Task 3 và không được sửa trong đợt tiếp tục này. Solutions giữ nguyên.

Binary Linux mới: build-continuation-release/dds_monitor và build-continuation-release/task3/task3_agent. Dashboard Task 3 và DDS nâng cao dùng chung một Main Window. Telemetry đi qua Topic DDS riêng; không thêm Web/REST/Prometheus.

Debug/Release/Agent Linux build đạt; bộ Qt Test có 37 mục đạt, gồm 35 kịch bản chính và setup/cleanup. Bộ tích hợp cũ có 14 kiểm tra đạt; bộ runtime mới có 15 kiểm tra đạt. Windows, NTP/liên máy vật lý, Dynamic Chart aggregate/cumulative và kiểm thử dài hạn vẫn còn mở. Các dữ liệu tải giả lập không đại diện nhiều máy vật lý.

Audit readiness mới nhất đã tái hiện Master Release thoát `-11` khi Publisher dừng. Vì vậy các kết quả PASS ở trên là bằng chứng từ các lượt trước, không phải xác nhận sẵn sàng end-to-end. Xem [báo cáo pre-test readiness](docs/PRE_TEST_READINESS.md).

Giấy phép GPL-3.0-or-later của DDS Monitor được giữ nguyên. Source Task 3 dùng SPDX tương ứng; Fast DDS/Statistics Backend là các dependency được liên kết riêng với giấy phép của chúng. Khi phân phối phải bảo toàn source/giấy phép theo nghĩa vụ áp dụng.
