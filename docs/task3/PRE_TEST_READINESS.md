# Rà soát sẵn sàng kiểm thử thủ công — 16-10-2026

## Kết quả tích hợp vào repository chính

Repository chính thức đã là `/home/dev/DDS-Monitor`; mã Task 3, script, cấu hình, test và patch đã được tích hợp tại đây. Backend được build từ source sạch vào `/home/dev/DDS-Monitor/.task3-deps/fastdds-statistics-backend-2.5.0-patched/`; library SHA-256 `167f5afa4fd7c6b9991f84fdf40b7a99564c264515e10c1507da656e565b9003`. Build Debug/Release và CTest đạt PASS; Local Integration 14/14, Continuation 15/15, Subscriber restart 6/6, Publisher restart 30/30 đều được chạy lại trên build của repo chính. Kết quả đầy đủ nằm trong `test-artifacts/main-project-20261016/`.

Kết luận hiện tại: **MAIN_PROJECT_READY_FOR_MANUAL_LINUX_TESTING**. Đây chỉ là readiness Linux cục bộ qua wrapper đã vá; Windows Agent, Linux–Windows giữa máy vật lý và NTP hai máy vẫn **PENDING_EXTERNAL_TEST**.

## Kết luận

**READY_FOR_MANUAL_LINUX_TESTING.** Patch race condition của Fast DDS Statistics Backend 2.5.0 đã được tái tạo từ source sạch vào prefix bền vững trong workspace. Master Debug/Release mới, CTest, Local Integration 14/14, Continuation Integration 15/15, Subscriber restart 6/6 và Publisher stop/restart 30/30 đều PASS trên artifact mới. `ldd`, cấu hình CMake và `/proc/<pid>/maps` xác nhận Master thực sự nạp thư viện đã vá. SQLite integrity, QML và shutdown sạch cũng PASS.

Kết luận này chỉ áp dụng cho kiểm thử thủ công trên Linux với các script `task3/scripts/run_*_patched.sh`. **Không** áp dụng cho `/home/dev/dds-deps/install`, vốn vẫn giữ Backend chưa vá, hoặc cho Windows, NTP liên máy và Linux–Windows vật lý.

**PENDING_EXTERNAL_TEST:** Windows native build/runtime, đồng bộ NTP giữa máy và kiểm thử giữa các máy vật lý chưa được chạy. Không thay đổi NTP, clock, firewall, service hệ thống hoặc cài package bằng sudo.

## Artifact và khả năng tái tạo

- Backend: Fast DDS Statistics Backend `2.5.0`, source commit `314fcd6e9d888077497d815406fcf134c6754759` (`v2.5.0`), source tree sạch.
- Patch: `test-artifacts/p0-crash-20261016/Fast-DDS-Statistics-Backend-2.5.0-database-queue-lifecycle.patch`; SHA-256 `83019a4479ec8b05cdab0a2d0e97461d9e3d386bcda865ba5f89fbf02e383a9a`.
- Prefix vá: `.task3-deps/fastdds-statistics-backend-2.5.0-patched/` (Git-ignored); library SHA-256 `167f5afa4fd7c6b9991f84fdf40b7a99564c264515e10c1507da656e565b9003`. Hai lần build sạch liên tiếp cho cùng hash sau khi cố định `__FILE__` bằng `-ffile-prefix-map`.
- Build Backend cuối: `.task3-deps/backend-builds/20261016T054619Z-7/`; GCC 11.4, CMake 3.22.1. Build shared, cấu hình build mặc định; script so sánh và yêu cầu bộ public headers cài đặt giống hệt dependency Backend 2.5.0 hiện có trước khi cho phép build tiếp.
- Task 3 Debug/Release mới: `.task3-deps/task3-builds/20261016T054514Z-2/{debug,release}/`.
- SHA-256 Backend mặc định `/home/dev/dds-deps/install/lib/libfastdds_statistics_backend.so.2.5.0` vẫn là `6aeb9f948f33c431d859321267c08cb7952d30c04fed07eb5ea6016e0ff0092f`; script không ghi đè prefix này.
- Patch giữ nguyên SONAME `libfastdds_statistics_backend.so.2`, có cùng tập 23.081 symbol động và public headers byte-identical với Backend 2.5.0 mặc định. CMake Debug/Release chọn package config từ prefix vá; `ldd` phân giải Fast DDS 3.6.1 và Fast CDR từ `/home/dev/dds-deps/install`.

Build lại bằng lệnh:

```bash
cd /home/dev/DDS-Monitor
bash task3/scripts/build_task3_patched.sh --jobs 4
```

Script tạo source staging sạch mỗi lượt, kiểm tra source commit/tag/tree và checksum patch, áp dụng patch một lần với kiểm tra context nghiêm ngặt, rồi build/install vào prefix riêng. Nó tạo build Debug/Release mới, chạy CTest cho cả hai và cập nhật `.task3-deps/task3-builds/LATEST`. Có thể cấu hình các prefix bằng `TASK3_BACKEND_SOURCE`, `TASK3_PATCHED_BACKEND_PREFIX`, `TASK3_DDS_PREFIX`, `TASK3_QT_PREFIX`, `TASK3_BUILD_JOBS`. Install prefix đã tồn tại mà không có owner marker phù hợp sẽ bị từ chối.

## Kết quả kiểm tra trên artifact bền vững

| Hạng mục | Kết quả | Bằng chứng |
| --- | --- | --- |
| Build Backend sạch từ source + patch ghim checksum | PASS | `.task3-deps/backend-builds/20261016T054619Z-7/`; manifest, SONAME, ABI symbol set, public headers và ldd được kiểm tra |
| Build Task 3 Debug và Release | PASS | `.task3-deps/task3-builds/20261016T054514Z-2/` |
| CTest Debug và Release | PASS | 1/1 test executable ở mỗi cấu hình |
| CMake dependency selection và dynamic linking | PASS | Cache ghi `fastdds_statistics_backend_DIR` trong prefix vá; `ldd` không có `not found` |
| Master runtime mapping | PASS | Local/continuation/lifecycle artifacts ghi `/proc/<pid>/maps` trỏ đúng library SHA đã ghim |
| Local integration | PASS 14/14 | `test-artifacts/p0-crash-20261016/persistent-final-local-14/results.json`; có Discovery, Throughput, Latency, Agent Offline/Recovery và alert lifecycle |
| Continuation integration | PASS 15/15 | `.../persistent-final-continuation-15/results.json`; có GUID remapping, topology/matching, No Data/recovery, restart, QML và SQLite |
| Subscriber stop/restart | PASS 6/6 | `.../persistent-final-subscriber-restart/results.json`; disposal, GUID remapping và Master shutdown |
| Publisher stop/restart | PASS 30/30 | `.../persistent-final-publisher-30/result.json`; Publisher exit 0 từng lượt, Master sống, GDB/Master exit 0 |
| SQLite | PASS | `PRAGMA integrity_check=ok`; continuation ghi 10 event rows, local integration DB integrity được kiểm tra riêng |
| QML/runtime | PASS | Không phát hiện `TypeError`, `ReferenceError`, binding-loop hoặc lỗi runtime nghiêm trọng trong continuation |
| Master shutdown và tiến trình test | PASS | Shutdown sạch trong continuation, Subscriber regression và Publisher regression; không còn tiến trình test |
| Windows/NTP liên máy/Linux–Windows vật lý | PENDING_EXTERNAL_TEST | Chưa có môi trường kiểm chứng |

Lượt Local Integration không được cấp quyền mở UDP ban đầu thất bại với `Operation not permitted`; artifact thất bại được giữ riêng. Chạy lại cùng test-owned processes với quyền socket cục bộ được cấp đã PASS 14/14. Đây là giới hạn sandbox, không phải kết quả được tính PASS.

## Tự kiểm thử Linux

Mở Terminal 1 và chạy Master. Script chọn Release mới nhất, yêu cầu `TASK3_KEY` qua prompt ẩn, xác nhận liên kết động rồi xác minh map thư viện trong tiến trình thực tế:

```bash
cd /home/dev/DDS-Monitor
bash task3/scripts/run_master_patched.sh
```

Mở Terminal 2 và chạy Linux Agent. Nhập lại cùng khóa tại prompt ẩn:

```bash
cd /home/dev/DDS-Monitor
bash task3/scripts/run_linux_agent_patched.sh
```

Agent dùng `task3/config/linux-agent.json`. Trong Master, chọn Host để xem CPU/RAM/Disk/Network, Applications và Last Seen. Nhấn `Ctrl+C` ở Terminal 2; đợi quá timeout 5 giây để kiểm tra Offline/alert. Chạy lại script Agent và nhập cùng khóa để kiểm tra Online/Recovery và lịch sử sự kiện.

Để quan sát DDS Statistics, mở Terminal 3 và 4, đặt cùng đường dẫn runtime đã vá ở đầu `LD_LIBRARY_PATH`, bật Statistics rồi chạy Publisher/Subscriber. Các lệnh không chứa khóa triển khai:

```bash
cd /home/dev/DDS-Monitor
export TASK3_PATCHED_BACKEND_PREFIX="$PWD/.task3-deps/fastdds-statistics-backend-2.5.0-patched"
export DDS_PREFIX=/home/dev/dds-deps/install
export QT_PREFIX=/home/dev/Qt/6.7.2/gcc_64
export LD_LIBRARY_PATH="$TASK3_PATCHED_BACKEND_PREFIX/lib:$DDS_PREFIX/lib:$QT_PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export FASTDDS_STATISTICS="HISTORY_LATENCY_TOPIC;NETWORK_LATENCY_TOPIC;PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;PHYSICAL_DATA_TOPIC;DISCOVERY_TOPIC;MONITOR_SERVICE_TOPIC"
/home/dev/dds-tests/hello_world/build/hello_world subscriber
```

Terminal 4 dùng cùng phần `export` trên, sau đó:

```bash
/home/dev/dds-tests/hello_world/build/hello_world publisher
```

Trong Master kiểm tra DDS Entities, Discovery, Throughput, Latency và Alerts/Event History. Khi kết thúc, dừng Publisher/Subscriber, Agent rồi Master bằng `Ctrl+C`. Wrapper xác nhận thư viện đã vá trong Master đang chạy; không thay `LD_LIBRARY_PATH` sau khi khởi chạy.

Kiểm tra DB sau shutdown:

```bash
python3 -c 'import sqlite3; db=sqlite3.connect(".task3-runtime/manual-master/events.sqlite"); print(db.execute("PRAGMA integrity_check").fetchone()[0]); db.close()'
```

## Giới hạn chưa đóng

- Windows native build/service lifecycle, Windows Time synchronized, đồng bộ NTP và offset giữa hai máy vẫn cần kiểm thử ngoài môi trường này.
- Thử nghiệm 30 chu kỳ không chứng minh toán học race không thể tái diễn; kết luận dựa trên root cause, patch, regression queue và hồi quy ứng dụng thật.
- Dynamic Chart aggregate/cumulative còn known issue; `delay_time=30000 ms` và API `FASTDDS_MONITOR_APP` được giữ nguyên.
- Không suy diễn thành nghiệm thu nhiều máy vật lý, mật độ DDS cao hoặc chạy 24 giờ.
