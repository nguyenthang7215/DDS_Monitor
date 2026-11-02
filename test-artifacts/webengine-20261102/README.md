# Giao diện Task 3 trên Qt WebEngine — so sánh với bản QML (02/11/2026)

Build: Qt 6.8.3 (`.task3-deps/qt683-builds/release`). Công cụ: `task3_dashboard_preview` dựng **cùng một snapshot**
MonitoringCore (`test-artifacts/final-fix-20261019/ui-1600/*.json`) bằng `Dashboard.qml` (QML cũ) và `WebDashboard.qml` (WebEngine).
Chạy lại: `bash test-artifacts/webengine-20261102/compare_qml_web.sh` (cần Pillow để ghép ảnh).

Mỗi file `shots/<trang>-pair.png`: **trên = QML, dưới = WebEngine**, kích thước vùng dashboard 1600×806.

| Trang | Pixel khác (ngưỡng 48/255) |
| :--- | ---: |
| Tổng quan | 3.73 % |
| Tài nguyên | 2.36 % |
| Mạng / interface | 2.78 % |
| Ứng dụng | 3.81 % |
| Luồng DDS | 4.37 % |
| Đồng bộ giờ | 2.73 % |
| Agent Offline | 4.03 % |
| Cảnh báo & lịch sử | 4.36 % |

Phần khác chủ yếu do khử răng cưa chữ của Chromium khác Qt và lệch 1–4 px; bố cục, nội dung, màu và trạng thái giống nhau.

Kiểm tra tương tác (bấm trong trang web → trạng thái QML), tất cả đúng:

| Thao tác | Kết quả |
| :--- | :--- |
| Bấm dòng `linux-agent-b` ở bảng tổng quan | `selectedHostId = linux-agent-b` |
| Bấm host → ứng dụng `publisher` → tab Luồng DDS → luồng → "DDS chuyên sâu" | `detailTab=2`, `selectedFlowKey=52/24`, phát `openDdsWorkspace("52")` |
| Bấm tab "Mạng / interface" | `resourceTab=1`; `hostNetworkMetrics` = 1292×291, 6 dòng |
| Bấm "Tất cả Slave" | bỏ chọn host |

Chưa kiểm tra: chạy `dds_monitor` đầy đủ (thiếu module Qt5Compat cho Qt 6.8.3), dữ liệu live từ Agent, máy Windows.
