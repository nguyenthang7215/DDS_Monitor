# Cấu hình vận hành

Các giá trị mặc định là lựa chọn triển khai, không phải ngưỡng nghiệm thu.

## Agent

Cấu hình JSON mẫu nằm trong task3/config/. host_id phải duy nhất và ổn định. telemetry_domain mặc định 42, telemetry_topic mặc định Task3/HostTelemetry/v1, interval_ms từ 100 đến 60000 ms. disk_paths liệt kê ổ/phân vùng cần theo dõi.

Mỗi applications[] có application_id, process_name khớp chính xác, service tùy chọn và participant_guids cấu hình thủ công. Có thể cấu hình:
- identity_directory ở cấp Agent: thư mục manifest runtime; đọc lại mỗi chu kỳ.
- identity_required=true ở cấp ứng dụng: chỉ dùng các instance có metadata runtime; thiếu metadata giữ Unknown.
- argument_token ở cấp ứng dụng: trên Linux, yêu cầu một phần tử cmdline khớp chính xác. Windows chưa hỗ trợ bộ lọc này và giữ Unknown khi được yêu cầu; dùng instrumentation để nhận diện instance.
- dds_host_identifier ở cấp Agent: ghi đè định danh Physical Data nếu cấu hình middleware khác mặc định.

Nhiều tiến trình khớp được giữ trong instances[], không chọn một PID để suy đoán ánh xạ.

## Master

| Biến | Mặc định / ý nghĩa |
| --- | --- |
| TASK3_TELEMETRY_DOMAIN / TASK3_TELEMETRY_TOPIC | 42 / Task3/HostTelemetry/v1 |
| TASK3_AUTO_DOMAIN | 0; off tắt tự khởi tạo Statistics Monitor |
| TASK3_TIMEOUT_MS | 5000, tối thiểu 100 |
| TASK3_EXPECTED_HOSTS | ID máy phân tách bằng dấu phẩy; Unknown trước bản tin đầu |
| TASK3_DATA_DIR | Qt AppLocalDataLocation/task3; chứa events.sqlite cho lịch sử/trạng thái cảnh báo; không có replay checkpoint |
| TASK3_EXPECTED_TOPICS | Topic chu kỳ phân tách bằng dấu phẩy, giữ tương thích cấu hình cũ |
| TASK3_DDS_TIMEOUT_MS | 15000, tối thiểu 1000; timeout quy tắc Topic cũ |
| TASK3_FLOW_RULES | Đường dẫn JSON liệt kê topic, mode=periodic/event và deadline_ms (100–3600000) |
| TASK3_THROUGHPUT_MIN | Không đặt; khi bật dùng B/s |
| TASK3_THROUGHPUT_HYSTERESIS | 0 B/s; đang cảnh báo thì cần đạt min+hysteresis mới khôi phục |
| TASK3_ALERT_TRIGGER_MS / TASK3_ALERT_RECOVERY_MS / TASK3_ALERT_COOLDOWN_MS | 0; debounce kích hoạt/khôi phục và thời gian chờ tái kích hoạt |
| TASK3_EVENT_RETENTION_DAYS | 0 = không xóa. Giá trị dương xóa các cặp sự kiện đã khôi phục và quá hạn; giữ cảnh báo đang hoạt động |
| TASK3_QUERY_INTERVAL_MS | 2000, giới hạn 200–60000 |
| TASK3_QUERY_WINDOW_MS | 120000, giới hạn 1000–3600000 |
| TASK3_MAX_SAMPLES | 256 mẫu hiển thị, giới hạn 16–10000 |
| TASK3_SNAPSHOT_PATH / TASK3_SCREENSHOT_PATH | Không đặt; xuất JSON/chụp giao diện để chẩn đoán |
| TASK3_EXIT_AFTER_MS | Không đặt; thoát Qt bình thường sau thời gian chỉ định |
| TASK3_UI_TAB | 0; trang Task 3 ban đầu, từ 0 đến 3 |
| TASK3_UI_EXERCISE | Không đặt; chỉ bật trong kiểm thử để tự chuyển trang và resize |
| TASK3_WINDOWS_TIME_SCRIPT | Agent Windows: đường dẫn script CIM, mặc định cạnh executable |

Ví dụ flow rules:

~~~json
[
  {"topic":"Task3/RuntimeProbe","mode":"periodic","deadline_ms":1500},
  {"topic":"BusinessEvents","mode":"event","deadline_ms":15000}
]
~~~

Không tạo hai Statistics Monitor trên cùng Domain. Telemetry Domain và application statistics Domain là hai cấu hình riêng. Agent/Master phải thống nhất Domain/Topic/type/QoS, không dùng TASK3_KEY/HMAC. Discovery, NIC và firewall cần xác minh trên mạng triển khai thật.
