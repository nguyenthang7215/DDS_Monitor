# Build và chạy Task 3 trên Linux

Repository chính thức: `/home/dev/DDS-Monitor`. Tất cả lệnh dưới đây chạy từ repo này; không cần source tree trong Downloads. Build đã xác minh ngày 16-10-2026 dùng Backend library SHA-256 `167f5afa4fd7c6b9991f84fdf40b7a99564c264515e10c1507da656e565b9003`.

## Build với Statistics Backend đã vá

Chạy trong repo chính. Lệnh dưới tạo Backend 2.5.0 từ source commit đã ghim, áp dụng patch đã kiểm chứng, build sạch Task 3 Debug/Release và chạy CTest. Các thư viện/build lớn nằm trong `.task3-deps/` và bị Git ignore.

```bash
cd /home/dev/DDS-Monitor
bash task3/scripts/build_task3_patched.sh --jobs 4
```

Có thể chỉ build/cài Statistics Backend bằng `bash task3/scripts/build_statistics_backend_patched.sh --jobs 4`. Mặc định prefix vá là `.task3-deps/fastdds-statistics-backend-2.5.0-patched/`; build hiện hành được ghi vào `.task3-deps/task3-builds/LATEST`. Script kiểm tra commit/tag/source sạch và SHA-256 patch trước khi build. Nó từ chối prefix đã có dữ liệu nhưng không được script đánh dấu sở hữu, đồng thời không cài đè `/home/dev/dds-deps/install`.

Để chuyển đường dẫn cài đặt, đặt `TASK3_BACKEND_SOURCE`, `TASK3_PATCHED_BACKEND_PREFIX`, `TASK3_DDS_PREFIX`, `TASK3_QT_PREFIX` hoặc `TASK3_BUILD_JOBS` trước khi gọi script. CMake ghim `fastdds_statistics_backend_DIR` vào package trong prefix vá; không dùng build directory/cache cũ. Xem checksum, source commit, library SHA, SONAME và compiler trong `share/task3-patched-backend/MANIFEST`.

Không dùng lệnh CMake chung bên dưới với prefix DDS mặc định để build bản thử Publisher/Subscriber lifecycle: prefix `/home/dev/dds-deps/install` vẫn nguyên bản chưa vá. Runtime wrapper sẽ dừng với lỗi rõ ràng nếu prefix vá, manifest, library hash hoặc linkage không đúng.

### Cấu hình CMake và generated sources

Root CMake tạo các target chính của Monitor rồi gọi `add_subdirectory(task3)`. `task3/CMakeLists.txt` thêm `task3_transport` (Fast DDS/Fast CDR), `task3_collectors` (Qt Core/Network), `task3_core` (Qt Core/Network/Sql/Concurrent và Statistics Backend đã vá), `task3_agent`, `task3_probe`, `task3_tests` và tài nguyên QML. Debug và Release được configure vào các thư mục build mới riêng bởi script; không tái sử dụng cache cũ.

`task3/Telemetry.idl` ghi lại wire contract CDR, nhưng hiện không được biên dịch bằng `fastrtpsgen`/`idlc` trong quy trình CMake. `task3/DdsTransport.cpp` hiện triển khai `TelemetryType` trực tiếp, vì vậy build hiện tại không sinh mã C++ từ IDL và không cần bước generated-source riêng. Nếu sau này đổi sang code generation, cần thêm rõ công cụ, target đầu ra và dependency vào CMake trước khi xem IDL là nguồn sinh mã.

## Khởi động Master

Terminal 1:

```bash
cd /home/dev/DDS-Monitor
bash task3/scripts/run_master_patched.sh
```

Wrapper chọn Release từ build gần nhất, đưa Backend vá lên đầu `LD_LIBRARY_PATH`, xác minh `ldd` và đợi thấy library đúng trong `/proc/<pid>/maps`. Master và Agent không dùng HMAC/TASK3_KEY; không có prompt nhập khóa.

## Khởi động Linux Agent

Terminal 2:

```bash
cd /home/dev/DDS-Monitor
bash task3/scripts/run_linux_agent_patched.sh
```

Không cần chia sẻ khóa giữa các terminal. Agent mặc định dùng `task3/config/linux-agent.json`; đổi config bằng biến `TASK3_AGENT_CONFIG` hoặc truyền đối số sau tên script. Agent không link Statistics Backend trực tiếp, vì vậy wrapper xác minh Fast DDS/Fast CDR của nó; Master mới là tiến trình phải map Statistics Backend đã vá.

## Kiểm tra Host, Offline và Recovery

Trong Master, chọn Host `linux-slave-a`, kiểm tra CPU, RAM, Disk, Network, ứng dụng và Last Seen. `Unknown`/`Stale` không đồng nghĩa với trạng thái hiện tại. Dừng Agent bằng `Ctrl+C`, giữ Master chạy và chờ quá `TASK3_TIMEOUT_MS` mặc định 5000 ms. Xác nhận Host Offline và xem Alerts/Event History. Chạy lại script Agent trong Terminal 2, không cần khóa, và xác nhận Host Online/Recovered.

## Publisher, Subscriber và DDS Statistics

Terminal 3 — Subscriber:

```bash
cd /home/dev/DDS-Monitor
export TASK3_PATCHED_BACKEND_PREFIX="$PWD/.task3-deps/fastdds-statistics-backend-2.5.0-patched"
export DDS_PREFIX=/home/dev/dds-deps/install
export QT_PREFIX=/home/dev/Qt/6.7.2/gcc_64
export LD_LIBRARY_PATH="$TASK3_PATCHED_BACKEND_PREFIX/lib:$DDS_PREFIX/lib:$QT_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export FASTDDS_STATISTICS="HISTORY_LATENCY_TOPIC;NETWORK_LATENCY_TOPIC;PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;PHYSICAL_DATA_TOPIC;DISCOVERY_TOPIC;MONITOR_SERVICE_TOPIC"
/home/dev/dds-tests/hello_world/build/hello_world subscriber
```

Terminal 4: dùng lại các lệnh `export` trên rồi chạy:

```bash
/home/dev/dds-tests/hello_world/build/hello_world publisher
```

Trong Master mở `DDS entities & data flow` và DDS nâng cao để xem Discovery, Throughput, Latency. Cảnh báo chỉ xuất hiện khi có rule và bằng chứng đầu vào phù hợp; Hello World chưa instrument không tự cung cấp mọi liên kết `application_id`/Writer counter. Không diễn giải thiếu dữ liệu thành `Matched` hoặc `No Data` nếu chưa đủ bằng chứng.

Để kiểm tra Writer im lặng, GUID mapping và No Data có instrumentation, dùng `task3/config/runtime-agent.json` và `task3_probe` theo ví dụ trong `PRE_TEST_READINESS.md` hoặc chạy continuation integration.

## Shutdown và SQLite

SQLite chỉ lưu lịch sử/trạng thái cảnh báo; không còn replay checkpoint lưu bền. Khi database không ghi được, telemetry vẫn được tiếp nhận nhưng không thể bảo đảm lưu sự kiện. Thanh trạng thái và tooltip hiển thị lỗi Event database hoặc dropped telemetry; không xem các sự kiện đang hiện trong bộ nhớ là bằng chứng chúng đã được lưu thành công.

Dừng Publisher/Subscriber trước, sau đó Agent và Master bằng `Ctrl+C`. DB mặc định của Master runner ở `.task3-runtime/manual-master/events.sqlite`. Kiểm tra integrity sau khi Master thoát:

```bash
python3 -c 'import sqlite3; db=sqlite3.connect(".task3-runtime/manual-master/events.sqlite"); print(db.execute("PRAGMA integrity_check").fetchone()[0]); db.close()'
```

## Chạy lại regression

Các bài integration dùng UDP; nếu sandbox từ chối socket (`Operation not permitted`) thì lượt đó không phải PASS. Chạy lại trong môi trường được phép mở socket cục bộ. Cần dùng đúng build Release mới nhất và runtime prefix đã vá:

```bash
cd /home/dev/DDS-Monitor
export TASK3_PATCHED_BACKEND_PREFIX="$PWD/.task3-deps/fastdds-statistics-backend-2.5.0-patched"
export LD_LIBRARY_PATH="$TASK3_PATCHED_BACKEND_PREFIX/lib:/home/dev/dds-deps/install/lib:/home/dev/Qt/6.7.2/gcc_64/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
TASK3_BUILD_ROOT="$(<.task3-deps/task3-builds/LATEST)"
python3 task3/tests/local_integration.py --build "$TASK3_BUILD_ROOT/release" --hello-world /home/dev/dds-tests/hello_world/build/hello_world --output test-artifacts/manual-local
python3 task3/tests/continuation_integration.py --build "$TASK3_BUILD_ROOT/release" --output test-artifacts/manual-continuation --duration 60
python3 test-artifacts/p0-crash-20261016/subscriber_restart_regression.py --build "$TASK3_BUILD_ROOT/release" --output test-artifacts/manual-subscriber-restart
python3 test-artifacts/p0-crash-20261016/publisher_stop_restart_reproducer.py --master "$TASK3_BUILD_ROOT/release/dds_monitor" --probe "$TASK3_BUILD_ROOT/release/task3/task3_probe" --cycles 30 --dwell 1 --output test-artifacts/manual-publisher-30
```

Các harness kiểm tra `/proc/<pid>/maps` của Master để xác nhận thư viện vá thực tế được nạp. Chạy tuần tự để không trộn DDS Domain 0.

## Windows

`task3/scripts/build-windows.ps1` chỉ chuẩn bị quy trình build. Chưa có kết quả build/runtime Windows hoặc xác nhận Windows Time synchronized; cần Windows/MSVC/Qt/Fast DDS thật. Không lấy kết quả Linux làm PASS cho Windows.
