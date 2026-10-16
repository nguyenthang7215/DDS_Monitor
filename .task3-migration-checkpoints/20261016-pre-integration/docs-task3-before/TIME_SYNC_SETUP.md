# Đồng bộ thời gian và phạm vi xác minh

Ứng dụng chỉ đọc trạng thái. Các mẫu trong task3/config/time-sync chưa được cài. Việc áp dụng cấu hình, bật dịch vụ, đổi firewall hoặc đồng hồ thuộc thao tác quản trị riêng cần người vận hành phê duyệt.

## Master Linux và Slave Linux

Quản trị viên xác định địa chỉ Master, subnet Slave được phép, nguồn upstream và độ lệch cho phép; sao lưu cấu hình Chrony hiện có. Master cung cấp nguồn tham chiếu theo master.conf.example. Slave trỏ tới Master theo slave.conf.example, bật dịch vụ khi khởi động và cấu hình kết nối lại. Không chạy hai daemon đồng hồ xung đột trên cùng máy. Nguồn local stratum cô lập cần quyết định riêng, mẫu không tự bật.

Sau khi được phép áp dụng, kiểm tra chronyc sources và chronyc tracking lúc khởi động, mất nguồn, kết nối lại và hội tụ. Trên từng máy, lưu nguồn, offset, trạng thái và thời điểm quan sát; không thay độ lệch thiếu dữ liệu bằng 0.

Collector gọi chronyc -c tracking với LC_ALL=C, kiểm tra cấu trúc, số hữu hạn, stratum và leap state. Daemon phản hồi khác với nguồn hợp lệ; nguồn hợp lệ khác với xác minh đúng Master được chỉ định. source_reachable là Unknown vì collector tracking không tự đo reachability. Chỉ trạng thái Normal cùng nguồn/stratum hợp lệ mới báo synced. Stratum 0/16 hoặc leap không đồng bộ báo unhealthy; thiếu lệnh/quyền/daemon hoặc kết quả không hợp lệ báo unknown.

## Slave Windows

Quản trị viên cấu hình Windows Time Service dùng Master làm NTP peer, tự khởi động và đồng bộ lại theo chính sách máy/domain. Xác minh w32tm query status/source và độ lệch thực trên máy đích sau khi được phép cấu hình.

Collector không phân tích nhãn thông báo theo ngôn ngữ hiển thị. Script windows-time-status.ps1 dùng tên thuộc tính CIM có cấu trúc để lấy trạng thái dịch vụ và Windows Time performance counters; w32tm /query /source chỉ cung cấp giá trị nguồn. C++ kiểm tra JSON và chuyển ComputedTimeOffset từ microsecond sang second khi có counter và số nguồn hợp lệ. Đây là ước lượng độ lệch tuyệt đối, không phải bằng chứng signed offset hoặc synchronized hiện thời.

Chỉ dịch vụ Running, số nguồn và offset chưa đủ để xác nhận đồng hồ đồng bộ. clock_synced/source_valid vẫn null và state Unknown khi chưa có nguồn bằng chứng đáng tin cậy hơn. Nhánh native Windows Time status hoàn chỉnh vẫn còn mở; không giả định Win32 API export/RPC ABI chưa được kiểm tra là hợp lệ.

Tài liệu đối chiếu: [Windows Time tools/settings](https://learn.microsoft.com/en-us/windows-server/networking/windows-time-service/windows-time-service-tools-and-settings), [Microsoft mô tả performance counters và đơn vị microsecond](https://techcommunity.microsoft.com/blog/networkingblog/top-10-networking-features-in-windows-server-2019-10-accurate-network-time/339739), [MS-W32T QueryStatus](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-w32t/7e80a465-f5f4-4c3c-87ef-12f76e45f8d1). Đọc đặc tả RPC không đồng nghĩa đã có client ABI native được xác minh.

FR-08 chưa VERIFIED toàn phần: chưa cấu hình đồng bộ hai máy thật, chưa đo hội tụ/offset liên máy và chưa chạy Windows. Parser Chrony/JSON Windows đã được unit test trên Linux; script CIM chưa được thực thi trên Windows.
