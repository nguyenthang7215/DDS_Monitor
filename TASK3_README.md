# Phần mở rộng Task 3 của DDS Monitor

Bắt đầu từ [hướng dẫn chạy](docs/task3/BUILD_AND_RUN.md), [báo cáo bàn giao](docs/task3/DELIVERY_REPORT.md), [ma trận FR](docs/task3/IMPLEMENTATION_STATUS.md), [kết quả kiểm thử](docs/task3/TEST_RESULTS.md) và [vấn đề còn mở](docs/task3/KNOWN_ISSUES.md).

Source chính nằm trong repository DDS Monitor này (`/home/dev/DDS-Monitor` trên máy phát triển). Bản implementation trong Downloads chỉ là backup. Requirements ở `docs/task3/TASK3_REQUIREMENTS.md` được giữ nguyên.

Tại thư mục gốc repo, build bằng `bash task3/scripts/build_task3_patched.sh --jobs 4`; chạy Master bằng `bash task3/scripts/run_master_patched.sh`, Agent bằng `bash task3/scripts/run_linux_agent_patched.sh`. Các script chọn binary từ `.task3-deps/task3-builds/LATEST`, xác minh dependency/runtime đã vá. Qt/Fast DDS/Fast CDR và source Backend ghim phải được chuẩn bị theo BUILD_AND_RUN.md; clone repo đơn thuần chưa cung cấp các dependency này.

Giao diện có ba tab trong một Main Window. Telemetry đi qua Topic DDS riêng, không cần TASK3_KEY/HMAC. Session/sequence và tối đa 256 session nghỉ gần nhất mỗi máy chỉ giữ trong bộ nhớ; SQLite giữ Event History, không lưu replay checkpoint.

Race Backend từng gây crash đã được vá và kiểm chứng trên prefix riêng; dependency mặc định vẫn chưa vá nên phải dùng wrapper. Kết quả và giới hạn hiện hành được ghi trong TEST_RESULTS.md và [pre-test readiness](docs/task3/PRE_TEST_READINESS.md). Windows, NTP/liên máy vật lý, Dynamic Chart aggregate/cumulative và kiểm thử dài hạn vẫn chưa được nghiệm thu đầy đủ.

Giấy phép GPL-3.0-or-later của DDS Monitor được giữ nguyên. Source Task 3 dùng SPDX tương ứng; Fast DDS/Statistics Backend là các dependency được liên kết riêng với giấy phép của chúng. Khi phân phối phải bảo toàn source/giấy phép theo nghĩa vụ áp dụng.
