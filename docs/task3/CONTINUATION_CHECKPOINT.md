# Checkpoint để tiếp tục phát triển

## Final Fix — 19/10/2026

Ba P1 đã được tái hiện rồi sửa: nhãn QML giữ lỗi database/dropped telemetry, session restart không bị chặn do clock rollback, cảnh báo Active không còn gắn Verified khi thiếu bằng chứng. Debug/Release + CTest 1/1 mỗi bản, regression P1, local 14/14, continuation 15/15 và UI 17/17 PASS; Agent standalone build/`--once` PASS trên Linux. Build đang dùng: `.task3-deps/task3-builds/20261016T085922Z-251510/`; wrapper lấy build từ LATEST. Xem [FINAL_FIX_RESULTS.md](FINAL_FIX_RESULTS.md). Các kết quả Publisher 30 chu kỳ/Subscriber 6 kiểm tra bên dưới là lịch sử, không chạy lại trong đợt này. Chưa commit/push; Windows/NTP liên máy vẫn PENDING_EXTERNAL_TEST.

## Source of truth sau tích hợp

Repo chính: `/home/dev/DDS-Monitor` (branch `main`, upstream trước tích hợp `94bbd6d`). Repo implementation trong Downloads được giữ nguyên làm backup; không còn là runtime/build root. Checkpoint trước tích hợp gồm Git status/diff/log và bản sao docs: `.task3-migration-checkpoints/20261016-pre-integration/`.

Trạng thái repo chính: **MAIN_PROJECT_READY_FOR_MANUAL_LINUX_TESTING**. Build root `.task3-deps/task3-builds/20261016T060144Z-169555/`; Backend prefix `.task3-deps/fastdds-statistics-backend-2.5.0-patched/`; Backend SHA-256 `167f5afa4fd7c6b9991f84fdf40b7a99564c264515e10c1507da656e565b9003`. Debug/Release + CTest, Local 14/14, Continuation 15/15, Subscriber 6/6, Publisher 30/30 đều PASS trên repo chính. Artifacts mới: `test-artifacts/main-project-20261016/`.

Build từ root bằng `bash task3/scripts/build_task3_patched.sh --jobs 4`; chạy `bash task3/scripts/run_master_patched.sh` và `bash task3/scripts/run_linux_agent_patched.sh`. Không xóa implementation backup. Windows Agent, physical Linux–Windows, NTP hai máy và Dynamic Chart aggregate/cumulative vẫn chưa được xác minh/đóng.

## Trạng thái P0 cuối — 16-10-2026

**READY_FOR_MANUAL_LINUX_TESTING** là kết luận lịch sử của build trong implementation backup. Backend source/patch/library hashes nêu trên vẫn được giữ; kết quả và build root hiện hành của repo chính được ghi ở mục “Source of truth sau tích hợp” phía trên. `/home/dev/dds-deps/install` vẫn là bản chưa vá.

Build bằng `bash task3/scripts/build_task3_patched.sh --jobs 4`; chạy bằng `bash task3/scripts/run_master_patched.sh` và `bash task3/scripts/run_linux_agent_patched.sh`. Xem `docs/task3/PRE_TEST_READINESS.md` cho ma trận kết quả và hướng dẫn người dùng. Windows/NTP liên máy/Linux–Windows vẫn PENDING_EXTERNAL_TEST.

Lịch sử checkpoint `3881967` và `52bbdea` thuộc bản implementation backup. Mã để phát triển, build và chạy hiện nằm trong `/home/dev/DDS-Monitor`; bản backup được giữ nguyên. Không dùng `git reset --hard` hoặc `git clean`.

## Đã thực hiện

RuntimeIdentity manifest theo PID/instance/GUID; nhiều process/Participant; bộ đếm write; FlowModel/ConnectionList freshness; một Main Window; raw Dynamic Chart snapshot; worker SQLite/retry/retention/migration; debounce/cooldown/hysteresis; session/sequence trong bộ nhớ; queue DDS giới hạn; shutdown Qt và build Windows cấu hình prefix.

Binary Linux trong lần kiểm chứng của implementation backup (lịch sử):
- `.task3-deps/task3-builds/20261016T054514Z-2/debug/dds_monitor`
- `.task3-deps/task3-builds/20261016T054514Z-2/release/dds_monitor`
- Agent và probe cùng nằm trong `debug/task3/` hoặc `release/task3/`.
- Build script đặt đường dẫn Release gần nhất trong `.task3-deps/task3-builds/LATEST`.

Đường dẫn dependency Linux: Qt `/home/dev/Qt/6.7.2/gcc_64`, Fast DDS/Fast CDR `/home/dev/dds-deps/install`, Statistics Backend đã vá `.task3-deps/fastdds-statistics-backend-2.5.0-patched`. Wrapper đặt Backend vá trước trong `LD_LIBRARY_PATH`, xác minh `ldd` và `/proc/<pid>/maps`. Master/Agent không cần shared key.

## Kiểm tra gần nhất

Final P0 verification trên prefix persistent/library SHA nêu trên và build của repo chính: Debug/Release build PASS, CTest 1/1 mỗi cấu hình, Local Integration 14/14, Continuation Integration 15/15, Subscriber restart 6/6, Publisher stop/restart 30/30. Master runtime map, SQLite `integrity_check=ok`, QML và shutdown đều PASS. Artifact mới dưới `test-artifacts/main-project-20261016/`. Lượt sandbox bị chặn UDP và lượt Continuation bị ngắt thủ công được giữ riêng, không tính PASS. Xem `docs/task3/TEST_RESULTS.md`.

Các lệnh đầy đủ nằm trong BUILD_AND_RUN.md. Test DDS cần socket; sandbox từ chối socket phải chạy lại bằng cơ chế cấp quyền, không tính là PASS.

## Công việc thực sự còn lại

1. Dynamic Chart aggregate/cumulative: Engine.cpp, DynamicDataChartBox và query window. Nhánh NONE đã sửa/test; giữ delay_time=30000. Cần tái hiện controlled trước khi thay aggregate semantics.
2. Windows: build/chạy task3/agent bằng MSVC/Qt/Fast DDS phù hợp; script build-windows.ps1 nhận prefix. windows-time-status.ps1/C++ chỉ cho evidence service/source/counter, chưa xác nhận synchronized native. Không khai Windows PASS.
3. Kiểm thử nhiều máy vật lý/NTP sau khi người dùng cho phép áp dụng OS config. Không sudo, không tự đổi clock/firewall/service.
4. Đo DDS mật độ cao: Backend get_data(raw) có thể cấp phát nhiều mẫu trước display cap. Đo migration DB rất lớn và chạy dài theo quy mô/thời gian do người dùng xác định.
5. Rà soát UX trên cửa sổ thật, service lifecycle thật trong môi trường test được cấp quyền và bookkeeping disposal upstream.

Không còn HMAC/TASK3_KEY hoặc checkpoint replay SQLite. Trong cùng session, sequence phải tăng; Master nhớ 256 session nghỉ gần nhất/máy và từ chối bản tin từ chúng. Session mới không phụ thuộc timestamp tăng nên Agent restart vẫn phục hồi khi đồng hồ lùi. Phạm vi lọc này chỉ trong bộ nhớ và mạng tin cậy, xem COMMUNICATION.md. Event History và trạng thái cảnh báo vẫn lưu SQLite; database hỏng không chặn telemetry nhưng cần hiển thị lỗi lưu trữ.
