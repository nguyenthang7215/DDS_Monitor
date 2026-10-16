# Trạng thái triển khai và ma trận FR — 16-10-2026

## Tích hợp vào repository chính

Source of truth hiện tại: `/home/dev/DDS-Monitor` (upstream HEAD trước tích hợp `94bbd6d`). Toàn bộ module Task 3, cấu hình, tests, scripts và patch P0 đã được chuyển từ implementation; repo implementation cũ được giữ làm backup. Build mới tại `.task3-deps/task3-builds/20261016T060144Z-169555/`; Debug/Release, CTest, Local 14/14, Continuation 15/15, Subscriber 6/6, Publisher 30/30 đều PASS trên repo chính. Readiness: **MAIN_PROJECT_READY_FOR_MANUAL_LINUX_TESTING**; Windows/NTP/liên máy vật lý vẫn PENDING_EXTERNAL_TEST.

Các sửa đổi PoC đã có sẵn ở repo chính được giữ/hợp nhất: `FASTDDS_MONITOR_APP`, nhánh Dynamic Chart raw snapshot, và `delay_time=30000 ms` (vẫn là thử nghiệm). Không ghi đè `/home/dev/dds-deps/install`; Master wrapper bắt buộc xác minh Backend vá.

Phần dưới giữ lại ma trận và lịch sử trạng thái implementation trước khi chuyển. Source of truth hiện tại là `/home/dev/DDS-Monitor`; checkpoint và test mới được ghi trong `CONTINUATION_CHECKPOINT.md` và `TEST_RESULTS.md`.

Readiness trên repo chính: **MAIN_PROJECT_READY_FOR_MANUAL_LINUX_TESTING** khi build/chạy bằng các script dùng Backend persistent đã vá. Ma trận FR bên dưới mô tả mức triển khai và giới hạn tính năng; kết quả mới ở `docs/task3/PRE_TEST_READINESS.md` và `docs/task3/TEST_RESULTS.md`. Dependency mặc định `/home/dev/dds-deps/install` chưa được thay; không dùng prefix đó cho Publisher/Subscriber lifecycle testing.

Solutions docs/TASK3_REQUIREMENTS.md giữ nguyên SHA-256: 57e0de67e06239e22b558e441e010add14f8c79a7e324fa63835adc39e1eb47c.

VERIFIED_LOCAL chỉ xác nhận hành vi đã kiểm thử trên một máy Linux. Unit dữ liệu tổng hợp, build Linux và test adapter không được dùng để xác nhận Windows hoặc nghiệm thu Master–Slave vật lý.

## Các module thay đổi

- Collectors/RuntimeIdentity/Registry/Protocol: nhiều instance, manifest theo lần chạy, GUID động, bộ đếm Writer và checkpoint chống phát lại.
- DdsBridge/FlowModel/MonitoringCore: GUID ConnectionList, freshness, topology theo máy/ứng dụng, periodic/event và No Data qua restart.
- AlertStore: worker SQLite sở hữu kết nối riêng, queue/retry, migration/retention, debounce/cooldown, dedup và lưu trạng thái.
- qml/main.qml/Dashboard: một Main Window, trang Task 3 và DDS nâng cao, chi tiết ứng dụng/Participant, topology và trạng thái chưa xác định.
- Engine/DynamicDataChartBox/DataModel: sửa cửa sổ truy vấn/raw snapshot cho nhánh NONE; delay 30000 ms và API FASTDDS_MONITOR_APP giữ nguyên.
- DdsTransport/Shutdown: coalescing queue, dropped counter, đóng worker/callback/polling khi thoát bình thường.
- Agent standalone và script Windows có prefix cấu hình; nhánh Windows vẫn IMPLEMENTED_NOT_VERIFIED.

## Ma trận yêu cầu

Build cột dưới là build Linux Debug/Release. Windows và nghiệm thu vật lý đều PENDING trừ khi có bằng chứng riêng.

| FR / chức năng | Module / file | Implementation | Build | Test | Bằng chứng | Giới hạn |
| --- | --- | --- | --- | --- | --- | --- |
| FR-01 quản lý Slave | Registry.cpp, MonitoringCore.cpp, Dashboard.qml | Có danh sách/chọn máy, Unknown ban đầu | PASS | VERIFIED_LOCAL | CoreTests; regression/telemetry_real.json | 128 host tải là tổng hợp |
| FR-02 kết nối | Registry.cpp, MonitoringCore.cpp | Monotonic timeout, Offline, recovery | PASS | VERIFIED_LOCAL | regression/agent_timeout.json; runtime/agent_restart_identity_recovery.json | Không suy ra nguyên nhân mất phản hồi |
| FR-03 CPU | Collectors.cpp, Dashboard.qml | CPU interval, null mẫu đầu, biểu đồ | PASS | VERIFIED_LOCAL | regression/metrics_real.json; ảnh UI | Chưa hiệu chuẩn profiler độc lập |
| FR-04 RAM | Collectors.cpp | RAM nguồn hệ điều hành/hiển thị | PASS | VERIFIED_LOCAL Linux | regression/metrics_real.json | Windows runtime PENDING |
| FR-05 ổ đĩa | Collectors.cpp | QStorageInfo theo đường dẫn cấu hình | PASS | VERIFIED_LOCAL Linux | regression/metrics_real.json | Chưa bao phủ mọi loại thiết bị/phân vùng |
| FR-06 mạng | Collectors.cpp | Bộ đếm/rate theo interface, rate_valid | PASS | VERIFIED_LOCAL Linux | regression/metrics_real.json | Chưa test live interface churn/counter reset |
| FR-07 phần mềm | Collectors.cpp, RuntimeIdentity.cpp | Nhiều process; process/service riêng; error → Unknown | PASS | Process VERIFIED_LOCAL; service adapter UNIT | ownedMultipleProcesses, serviceTransitionsRemainIndependent | Chưa restart service hệ thống thật; SCM Windows PENDING |
| FR-08 thời gian | Collectors.cpp, windows-time-status.ps1, time-sync templates | Chrony parser hoàn thiện hơn; Windows evidence có cấu trúc | PASS Linux | UNIT/PARTIAL; PENDING_EXTERNAL_TEST | chronyParsing, serviceAndWindowsTimeEvidence | Windows sync native và NTP hai máy còn mở |
| FR-09 DDS entities | DdsBridge.cpp, Backend gốc | Domain/Participant/Topic/Writer/Reader | PASS | VERIFIED_LOCAL | regression/dds_discovery.json | Discovery vật lý liên máy PENDING |
| FR-10 ánh xạ | RuntimeIdentity.cpp, Collectors.cpp, Registry.cpp | Runtime instance/GUID, refresh qua restart, conflict → Unmapped | PASS | VERIFIED_LOCAL với instrumentation | runtime/runtime_mapping_multiple_participants.json; restart_guid_remapping.json | App không instrument cần cấu hình; không suy đoán GUID |
| FR-11 topology | FlowModel.cpp, Dashboard.qml | Host/Application/Writer/Topic/Reader, nhiều reader | PASS | VERIFIED_LOCAL; cross-host UNIT | runtime/topology_application_routes.json; topologyEvidence | Quan hệ liên máy chưa chạy vật lý |
| FR-12 trạng thái luồng | DdsBridge.cpp, FlowModel.cpp, MonitoringCore.cpp | Alive, Matched/Unmatched/Unknown, periodic/event/no-data/stale riêng | PASS | VERIFIED_LOCAL + UNIT | runtime/fresh_connection_list_matching.json; alive_writer_no_data.json | Matching hết hạn 15 giây; silent app không instrument vẫn Unknown |
| FR-13 thống kê | DdsBridge.cpp, Units.h; Engine/DataModel | Throughput/Latency; raw chart overlap/snapshot | PASS | VERIFIED_LOCAL metrics; UNIT raw model | regression/dds_throughput.json/dds_latency.json; legacyDynamicRawSnapshotRegression | Nhánh gộp/cumulative và hiệu chuẩn liên máy còn mở |
| FR-14 cảnh báo | MonitoringCore.cpp, AlertStore.cpp | Trigger/recovery/dedup, debounce/cooldown/hysteresis | PASS | VERIFIED_LOCAL + UNIT | runtime/runtime_counter_recovery.json; alertDebounceCooldown; throughputHysteresis | Cooldown trong phiên chưa được tái dựng đầy đủ qua restart; overload có giới hạn |
| FR-15 sự kiện | AlertStore.cpp | Worker transaction, cache, migration, retention, flush | PASS | VERIFIED_LOCAL + SQLite UNIT | sqliteWorkerLockAndShutdown, legacySqliteUpgradePreservesHistory; runtime/results.json | DB hỏng/khóa khi thoát có thể không flush được; không có spool thay thế |

Các đường dẫn regression/ và runtime/ trong bảng nằm dưới test-artifacts/continuation-final/. TestResults ghi số lượng và trạng thái cuối; không dùng việc viết test để thay cho việc chạy test.

## Tiếp tục khi có môi trường

1. Chạy Windows Agent/script trên Windows thật và hoàn thiện nguồn synchronized đáng tin cậy.
2. Cấu hình và kiểm thử NTP liên máy sau phê duyệt quản trị.
3. Tái hiện và sửa Dynamic Chart gộp/cumulative; giữ delay_time cho đến khi có bằng chứng.
4. Rà soát UX thật, tải DDS mật độ cao, database lớn và chạy 24 giờ theo tiêu chí thống nhất.

Không xóa test/artifact thất bại trước đó: runtime-01 tìm ra lỗi khóa No Data theo Writer ID; đã sửa sang luồng logic. Source không còn hai prefix debug PoC; POC_BASELINE.patch được giữ làm lịch sử.
