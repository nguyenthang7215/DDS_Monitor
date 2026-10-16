# Checkpoint để tiếp tục phát triển

## Trạng thái P0 cuối — 16-10-2026

**READY_FOR_MANUAL_LINUX_TESTING** qua wrapper persistent. Fast DDS Statistics Backend 2.5.0 được build từ source commit `314fcd6e9d888077497d815406fcf134c6754759`, patch SHA-256 `83019a4479ec8b05cdab0a2d0e97461d9e3d386bcda865ba5f89fbf02e383a9a`, library SHA-256 `167f5afa4fd7c6b9991f84fdf40b7a99564c264515e10c1507da656e565b9003`; hai lần build liên tiếp cho cùng hash. Install prefix `.task3-deps/fastdds-statistics-backend-2.5.0-patched/`. Task 3 build hiện tại `.task3-deps/task3-builds/20261016T054514Z-2/`. Debug/Release, CTest, local 14/14, continuation 15/15, Subscriber restart 6/6, Publisher restart 30/30, SQLite/QML/shutdown đều PASS; runtime test xác minh Master map đúng library. `/home/dev/dds-deps/install` vẫn là bản chưa vá.

Build bằng `bash task3/scripts/build_task3_patched.sh --jobs 4`; chạy bằng `bash task3/scripts/run_master_patched.sh` và `bash task3/scripts/run_linux_agent_patched.sh`. Xem `docs/PRE_TEST_READINESS.md` cho ma trận kết quả và hướng dẫn người dùng. Windows/NTP liên máy/Linux–Windows vẫn PENDING_EXTERNAL_TEST.

Source duy nhất: /home/dev/Downloads/Bao_cao_DDS_Monitor_Task3/implementation.
Git được khởi tạo cục bộ trong thư mục này; baseline 3881967 và asset baseline 52bbdea. Các thay đổi tiếp nối đang ở working tree để review; xem git status --short, git diff và các file mới trong task3/. Không reset/clean. Không sửa repo /home/dev/DDS-Monitor vì repo đó không chứa phần triển khai Task 3.

## Đã thực hiện

RuntimeIdentity manifest theo PID/instance/GUID; nhiều process/Participant; bộ đếm write; FlowModel/ConnectionList freshness; một Main Window; raw Dynamic Chart snapshot; worker SQLite/retry/retention/migration; debounce/cooldown/hysteresis; checkpoint replay; queue DDS giới hạn; shutdown Qt và build Windows cấu hình prefix.

Binary Linux trong lần kiểm chứng cuối:
- `.task3-deps/task3-builds/20261016T054514Z-2/debug/dds_monitor`
- `.task3-deps/task3-builds/20261016T054514Z-2/release/dds_monitor`
- Agent và probe cùng nằm trong `debug/task3/` hoặc `release/task3/`.
- Build script đặt đường dẫn Release gần nhất trong `.task3-deps/task3-builds/LATEST`.

Đường dẫn dependency Linux: Qt `/home/dev/Qt/6.7.2/gcc_64`, Fast DDS/Fast CDR `/home/dev/dds-deps/install`, Statistics Backend đã vá `.task3-deps/fastdds-statistics-backend-2.5.0-patched`. Wrapper đặt Backend vá trước trong `LD_LIBRARY_PATH`, xác minh `ldd` và `/proc/<pid>/maps`. Không ghi khóa triển khai vào source.

## Kiểm tra gần nhất

Final P0 verification trên prefix persistent/library SHA nêu trên: Debug/Release build PASS, CTest 1/1 mỗi cấu hình, Local Integration 14/14, Continuation Integration 15/15, Subscriber restart 6/6, Publisher stop/restart 30/30. Master runtime map, SQLite `integrity_check=ok`, QML và shutdown đều PASS. Artifact cuối dưới `test-artifacts/p0-crash-20261016/persistent-final-*/`. Lượt sandbox bị chặn UDP được giữ riêng, lượt được cấp socket đã PASS. Xem `docs/TEST_RESULTS.md`.

Các lệnh đầy đủ nằm trong BUILD_AND_RUN.md. Test DDS cần socket; sandbox từ chối socket phải chạy lại bằng cơ chế cấp quyền, không tính là PASS.

## Công việc thực sự còn lại

1. Dynamic Chart aggregate/cumulative: Engine.cpp, DynamicDataChartBox và query window. Nhánh NONE đã sửa/test; giữ delay_time=30000. Cần tái hiện controlled trước khi thay aggregate semantics.
2. Windows: build/chạy task3/agent bằng MSVC/Qt/Fast DDS phù hợp; script build-windows.ps1 nhận prefix. windows-time-status.ps1/C++ chỉ cho evidence service/source/counter, chưa xác nhận synchronized native. Không khai Windows PASS.
3. Kiểm thử nhiều máy vật lý/NTP sau khi người dùng cho phép áp dụng OS config. Không sudo, không tự đổi clock/firewall/service.
4. Đo DDS mật độ cao: Backend get_data(raw) có thể cấp phát nhiều mẫu trước display cap. Đo migration DB rất lớn và chạy dài theo quy mô/thời gian do người dùng xác định.
5. Rà soát UX trên cửa sổ thật, service lifecycle thật trong môi trường test được cấp quyền và bookkeeping disposal upstream.

Checkpoint replay lưu gộp mỗi khoảng 100 ms, có flush normal shutdown; crash có khoảng chưa commit. Session mới phải vượt timestamp cao nhất đã lưu. DB hỏng thì telemetry fail closed. Đừng bỏ các bảo vệ này để làm test PASS.
