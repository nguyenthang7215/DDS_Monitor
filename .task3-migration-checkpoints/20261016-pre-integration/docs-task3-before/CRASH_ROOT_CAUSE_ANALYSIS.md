# Phân tích nguyên nhân crash P0 khi Publisher dừng

Các đường dẫn `test-artifacts/...` trong báo cáo này tính từ `/home/dev/Downloads/Bao_cao_DDS_Monitor_Task3/implementation/`. Nếu tài liệu được sao chép sang thư mục khác, artifact vẫn ở workspace đó.

## Kết luận

Trạng thái hiện tại: **FIXED_AND_VERIFIED_ON_PERSISTENT_LINUX_ARTIFACT**. Kết luận này áp dụng cho Backend 2.5.0 được build từ source sạch + patch trong `.task3-deps/fastdds-statistics-backend-2.5.0-patched/`; không có nghĩa dependency mặc định `/home/dev/dds-deps/install` đã bị thay.

Crash được xác định là race trong vòng đời consumer của `DatabaseQueue` thuộc Fast DDS Statistics Backend 2.5.0. Khi một callback discovery gọi `stop_consumer()`, hàm chuyển thread cũ ra biến cục bộ rồi nhả `cv_mutex_` trước khi `join()` hoàn tất. Một callback khác có thể gọi `start_consumer()` trong khoảng đó; do member `consumer_thread_` đã rỗng và `consuming_` đã false, Backend tạo worker mới trong khi worker cũ chưa kết thúc. Hai worker sau đó cùng thao tác các queue double-buffer của một đối tượng `DatabaseQueue`. Điều này dẫn tới truy cập dữ liệu không đồng bộ, hỏng heap và cuối cùng crash.

Bản sửa thêm mutex riêng để khóa xuyên suốt thao tác stop/join và serialize nó với start. Patch được tái tạo từ source commit `314fcd6e9d888077497d815406fcf134c6754759` (`v2.5.0`) bằng `task3/scripts/build_statistics_backend_patched.sh`; SHA-256 patch là `83019a4479ec8b05cdab0a2d0e97461d9e3d386bcda865ba5f89fbf02e383a9a`. Build persistent mới đạt cùng SONAME và tập 23.081 symbol động với bản gốc. Master Debug/Release, CTest, local integration 14/14, continuation 15/15, Subscriber restart 6/6 và Publisher stop/restart 30/30 đều PASS trên artifact mới. Harness xác minh `/proc/<pid>/maps` của Master trỏ tới thư viện persistent; SQLite integrity, QML và shutdown sạch PASS. Dependency mặc định được giữ nguyên, do đó các lệnh chạy ngoài wrapper vẫn có thể nạp thư viện chưa vá.

## Phạm vi và cấu hình được xác minh

- Fast DDS Statistics Backend 2.5.0, source commit `314fcd6e9d888077497d815406fcf134c6754759`.
- Fast DDS 3.6.1 từ `/home/dev/dds-deps/install`.
- Task 3 Release và Debug mới, CMake ghim package Backend trong `.task3-deps/fastdds-statistics-backend-2.5.0-patched/lib/cmake/fastdds_statistics_backend`.
- Backend được cài đặt ban đầu: `/home/dev/dds-deps/install/lib/libfastdds_statistics_backend.so.2.5.0`.
- Không sửa repository source của Backend, thư viện trong `/home/dev/dds-deps/install`, source Task 3, đồng hồ, NTP, firewall hoặc dịch vụ hệ thống.

SHA-256 của hai thư viện cài đặt được ghi trước và sau điều tra không đổi:

| Thư viện | SHA-256 |
| --- | --- |
| `libfastdds_statistics_backend.so.2.5.0` | `6aeb9f948f33c431d859321267c08cb7952d30c04fed07eb5ea6016e0ff0092f` |
| `libfastdds.so.3.6` | `04aaca083b57a9da7f73184a0271b986e9bef0d29157e6b6066962fac61d2a62` |

Binary Task 3 thử nghiệm được kiểm tra bằng `ldd`; `libfastdds_statistics_backend.so.2` được nạp từ prefix vá, còn Fast DDS 3.6.1 được nạp từ prefix cài đặt. Với `LD_LIBRARY_PATH` đầy đủ, tất cả dependency được phân giải. Không dùng lượt `ldd` ban đầu thiếu đường dẫn `foonathan_memory` làm kết quả cuối; phép kiểm tra sau đã bổ sung đúng runtime path.

## Bằng chứng crash và đường gọi

Crash được tái hiện có kiểm soát khi Master chạy cùng Subscriber và nhiều lần tạo/dừng Publisher. Trên Backend chưa vá, 11 chu kỳ đầu hoàn tất; Master chết ở chu kỳ 12/30. Log ghi `malloc(): unsorted double linked list corrupted`, sau đó GDB bắt SIGSEGV.

Backtrace có symbol cho thấy thread `dds.udp.7414` dừng tại `fastdds::statistics::Data::_d(this=0x0)`, được gọi từ `DatabaseDataQueue<fastdds::statistics::Data>::process_sample()` (`src/cpp/database/database_queue.cpp:1066–1068`). Cùng lúc, một thread xử lý `StatisticsParticipantListener::on_data_writer_discovery()` (`src/cpp/subscriber/StatisticsParticipantListener.cpp:184–186`) đang ở `DatabaseQueue::stop_consumer()` và `std::thread::join()`. Thread khác xử lý `on_participant_discovery()` (`StatisticsParticipantListener.cpp:100–107`) tiếp tục đưa discovery/disposal vào queue. Dữ liệu locals trong một số frame đã bị hỏng, phù hợp với thông báo heap corruption.

Một báo cáo trước đó ghi kiểu `ExtendedMonitorServiceStatusData` tại điểm cuối stack. Bản tái hiện có symbol đầy đủ xác định trực tiếp fault trong queue `fastdds::statistics::Data` tại `Data::_d()` với `this=0x0`; phân tích này dùng backtrace chi tiết hơn làm bằng chứng cuối, không suy ra root cause chỉ từ tên hàm trong stack cũ.

Source Backend cho thấy khoảng trống đồng bộ:

1. `DatabaseQueue::stop_consumer()` trong `src/cpp/database/database_queue.hpp:152–183` đặt `consuming_ = false`, move `consumer_thread_` sang biến cục bộ rồi nhả `cv_mutex_`.
2. Hàm gọi `notify_all()` và `join()` sau khi đã nhả mutex.
3. `DatabaseQueue::start_consumer()` ở dòng 192–203 chỉ khóa `cv_mutex_`. Trong thời gian `join()` đang chờ, hàm có thể thấy `!consuming_ && !consumer_thread_` và tạo worker mới.
4. Cả worker cũ lẫn mới đều có thể chạy `consume_all()`/`swap()` trên hai queue dùng chung (`database_queue.hpp:211–221`, `consume_all()` gần dòng 322). Các mutex queue bảo vệ thao tác cục bộ nhưng không ngăn hai consumer cùng thực hiện vòng đời/đổi foreground-background như một consumer duy nhất.
5. Listener gọi stop/start từ các callback discovery chạy trên thread RTPS. Callback Participant và Writer có thể đến đồng thời, nên cửa sổ stop/join/start là reachable khi thực thể DDS bị loại bỏ hoặc khởi tạo lại.

Null `shared_ptr` tại `Data::_d()` là biểu hiện của trạng thái queue đã bị hỏng, không phải bằng chứng độc lập cho thấy reader nhận một mẫu DDS null. Reproducer tối giản trên header Backend chưa vá cũng làm AddressSanitizer bắt lỗi truy cập trong lúc `std::queue::pop`/`consume_all`; cùng harness khi biên dịch với header đã vá hoàn tất và báo số consumer đồng thời tối đa bằng 1. Sanitizer này chỉ instrument reproducer queue, không phải toàn bộ ứng dụng và dependency.

## Bản sửa cô lập

Patch `test-artifacts/p0-crash-20261016/Fast-DDS-Statistics-Backend-2.5.0-database-queue-lifecycle.patch` thêm `lifecycle_mutex_` vào lớp nội bộ `DatabaseQueue`. Cả `start_consumer()` và toàn bộ thao tác `stop_consumer()` — bao gồm `join()` — lấy mutex này. `cv_mutex_` vẫn được nhả trước khi join, nên worker cũ có thể thoát mà không deadlock; worker mới chỉ được phép tạo sau khi join kết thúc.

Patch chỉ thay đổi source nội bộ `src/cpp/database/database_queue.hpp`. Không sửa header API công khai. Để giảm sai khác do cấu hình build, so sánh ABI được thực hiện bằng hai build dùng GCC 11.4, `BUILD_SHARED_LIBS=ON`, `CMAKE_BUILD_TYPE` rỗng và cùng dependency Fast DDS/fastcdr:

- `SONAME` vẫn là `libfastdds_statistics_backend.so.2`.
- Tập tên symbol động: 23.081 symbol ở cả bản cài đặt và bản vá; không có symbol chỉ xuất hiện ở một bên.
- Các header công khai giống hệt giữa source gốc và bản sao đã vá.
- Binary Task 3 được link và chạy với Backend 2.5.0 vá mà không đổi API `FASTDDS_MONITOR_APP`.

So sánh symbol/SONAME và chạy tích hợp là bằng chứng tương thích cho cấu hình này; chúng không thay thế kiểm thử package trên mọi toolchain hoặc nền tảng. Khi đưa vào sử dụng cần build lại toàn bộ Statistics Backend 2.5.0 từ cùng source tree đã vá, không trộn object/library từ hai bản build.

## Ma trận tái hiện và kiểm chứng

### Kiểm chứng artifact persistent — 16-10-2026

| Hạng mục | Kết quả | Artifact/bằng chứng |
| --- | --- | --- |
| Source và patch | PASS; commit/tag, source tree sạch, checksum đúng | `.task3-deps/backend-builds/20261016T054619Z-7/` và manifest |
| Backend build/install | PASS; SHA-256 `167f5afa4fd7c6b9991f84fdf40b7a99564c264515e10c1507da656e565b9003`; hai build liên tiếp trùng hash | `.task3-deps/fastdds-statistics-backend-2.5.0-patched/` |
| SONAME/ABI/dependency | PASS; `.so.2`, 23.081 symbol, public headers byte-identical; Fast DDS 3.6.1/Fast CDR từ DDS prefix | Manifest, CMake cache và `ldd` |
| Debug/Release + CTest | PASS; 1/1 mỗi cấu hình | `.task3-deps/task3-builds/20261016T054514Z-2/` |
| Local/continuation integration | PASS 14/14 và 15/15; Master map đúng library | `test-artifacts/p0-crash-20261016/persistent-final-local-14/` và `persistent-final-continuation-15/` |
| Subscriber restart | PASS 6/6; GUID remap, Master sống và shutdown sạch | `.../persistent-final-subscriber-restart/` |
| Publisher stop/restart | PASS 30/30; Master sống mỗi lượt, GDB/Master exit 0; map đúng library | `.../persistent-final-publisher-30/result.json` |
| SQLite/QML/shutdown | PASS; integrity `ok`, không lỗi QML runtime, shutdown bình thường | Continuation results, `qml-errors.json` và SQLite state |

Backend mặc định tại `/home/dev/dds-deps/install` vẫn giữ nguyên SHA-256 `6aeb9f948f33c431d859321267c08cb7952d30c04fed07eb5ea6016e0ff0092f`; nó không bị ghi đè. Kết luận readiness là **READY_FOR_MANUAL_LINUX_TESTING** chỉ với script build/run persistent. Các kết quả ở `/tmp/dds-p0` bên dưới là bằng chứng chẩn đoán lịch sử, không thay thế các lượt test artifact persistent này.

| Kịch bản | Kết quả | Diễn giải |
| --- | --- | --- |
| Task 3 Release + Backend chưa vá, Publisher stop/restart | FAIL ở chu kỳ 12/30; 11 chu kỳ trước Master còn sống | Tái hiện crash đã biết. `controlled-task3-30cycles/result.json` và `master.log`. |
| DDS Monitor gốc + cùng Backend chưa vá và cùng loại probe, 30 chu kỳ | 30/30 chu kỳ Master còn sống | Không tái hiện trong 30 lượt. GDB wrapper trả 255 khi tiến trình gốc nhận SIGTERM lúc dọn dẹp; đây không phải crash giữa chu kỳ. Kết quả không chứng minh đường tích hợp gốc miễn nhiễm race. |
| Task 3 Release + Backend vá, Publisher stop/restart | 30/30 PASS; Publisher exit 0 từng lượt; Master/GDB exit 0 | `fresh-patched-task3-30cycles/result.json`. |
| Task 3 Release + Backend vá, Subscriber dừng và khởi động lại | 6/6 điều kiện PASS | Master sống, publisher còn hoạt động, Subscriber được ánh xạ lại bằng GUID mới, Master shutdown sạch. `fresh-subscriber-restart/results.json`. |
| Queue lifecycle reproducer, Backend header chưa vá + ASan/UBSan | FAIL với memory fault khi worker queue xử lý/pop mẫu | Reproducer cô lập race của primitive queue; không phải ASan report của toàn bộ app. |
| Queue lifecycle reproducer, Backend header đã vá + ASan/UBSan | PASS; tối đa một consumer cùng chạy | `database_queue_lifecycle_regression.cpp`. |
| Build Debug/Release và CTest với Backend vá | PASS; CTest 1/1 mỗi cấu hình | Build mới ở `/tmp/dds-p0/task3-debug` và `task3-release`. |
| Local integration / continuation integration với Release mới và Backend vá | PASS 14/14 và 15/15 | Bao gồm metrics, discovery, throughput, latency, No Data/recovery, GUID remapping, offline/recovery, UI, SQLite integrity và shutdown sạch. |
| Windows, NTP giữa máy, Linux–Windows trên máy vật lý | PENDING_EXTERNAL_TEST | Không chạy trong môi trường này; không thay đổi NTP, đồng hồ hoặc firewall. |

30 chu kỳ PASS không phải chứng minh toán học rằng mọi race đã biến mất. Kết luận dựa trên cả cơ chế tranh chấp được thấy trong source/backtrace, reproducer ASan có khả năng phân biệt bản chưa vá và đã vá, cùng hồi quy trên ứng dụng thật.

## Giới hạn và lịch sử artifact tạm

Các thư mục `/tmp/dds-p0/backend-patched-src`, `/tmp/dds-p0/patched-prefix` và `/tmp/dds-p0/patched-abi-prefix` thuộc điều tra ban đầu; chúng không còn là runtime dependency. Quy trình persistent nằm trong `task3/scripts/build_statistics_backend_patched.sh` và `task3/scripts/build_task3_patched.sh`, với prefix riêng trong `.task3-deps/`. Binary `/home/dev/dds-deps/install` và source repository Backend không bị thay đổi.

Không kết luận Windows hoặc đồng bộ thời gian liên máy đã đạt. Các biểu đồ Dynamic Chart, `delay_time=30000 ms`, kiến trúc Task 3 và API `FASTDDS_MONITOR_APP` không thuộc phạm vi sửa P0 này.
