# Kế hoạch và phân loại kiểm thử

Giữ nguyên 15 kịch bản chính đã có. Bộ Qt Test hiện gồm các ca cũ và ca bổ sung trong task3/tests/CoreTests.cpp; setup/cleanup được Qt tính thành hai mục riêng. Không bỏ ca để đổi kết quả thành PASS.

## Kiểm thử cũ

| ID | Phạm vi | Nguồn |
| --- | --- | --- |
| TEST-01/02 | Build Master, chạy QML thật | Build Debug/Release và local_integration.py |
| TEST-03/04 | Collectors thật, telemetry DDS | realCollectors, localDdsTelemetryTimeoutAndRecovery |
| TEST-05/06/07 | Unknown/Online/Offline/Recovery | configuredUnknownToOnline, timeout/restart cục bộ |
| TEST-08/09 | Nguồn metric, process/service | Snapshot/ảnh UI; processStopped |
| TEST-10/11 | Discovery, matching không suy đoán | DDS thật; noDataIsNotStatisticsStale |
| TEST-12/13/14 | Throughput/Latency/units/mẫu muộn | Hello World, units, lateSamplesRollingWindow |
| TEST-15/16 | No Data, khôi phục/lưu lịch sử | Core và tích hợp Agent/Publisher restart |
| TEST-17/18 | Run identity, schema và kích thước bản tin không hợp lệ | explicitPerRunMapping, protocolSchemaAndSizeValidation |
| TEST-19/20 | Giao diện, nhiều host | Export/capture; 80 host tổng hợp |
| TEST-21 | Chrony | CSV fixture và nguồn không khả dụng |
| TEST-22/23 | Windows/Linux–Windows | PENDING_EXTERNAL_TEST |

## Các kiểm tra bổ sung

| ID | Ca | Loại bằng chứng |
| --- | --- | --- |
| TEST-24 | Manifest, GUID mới, stale/wrong instance, nhiều Participant | Unit dùng tiến trình hiện tại và tích hợp DDS thật |
| TEST-25 | Nhiều process, tên executable dài, process dừng | Hai tiến trình Linux do test tạo/dừng |
| TEST-26 | Matching có/không có bằng chứng, hết hạn, topology hai ứng dụng | Unit bằng dữ liệu adapter; ConnectionList thật trong continuation_integration.py |
| TEST-27 | Writer còn Alive nhưng im lặng, periodic/event idle, khôi phục sau GUID đổi | Unit counter tổng hợp và probe DDS thật có instrumentation |
| TEST-28 | Service running/stopped/failed/unknown và tách khỏi process | Unit adapter/transition; không stop/restart service thật |
| TEST-29 | Dynamic Chart raw gốc nhận mẫu muộn/không trùng/sắp thứ tự | Chính DynamicDataChartBox/DataModel của DDS Monitor |
| TEST-30 | Windows time JSON/đơn vị/thiếu counter | Unit parser trên Linux; script/Win32 runtime chưa kiểm thử Windows |
| TEST-31 | Worker SQLite bị khóa, đọc cache không chặn timer, flush 3000 sự kiện | SQLite thật; mở lại database và kiểm tra trạng thái |
| TEST-32 | Debounce, cooldown, Unknown và hysteresis | Unit với thời gian kiểm soát |
| TEST-33 | Master restart không cần checkpoint; 300 Agent restart, replay session đã quan sát, clock rollback | Database thật và Registry; timestamp fixture, không đổi đồng hồ OS |
| TEST-34 | 1024 snapshot JSON, 128 host × 32 ứng dụng, 1000 gói sai | Kiểm thử tải tổng hợp, không phải 128 máy vật lý |
| TEST-35 | Burst 2000 bản tin DDS, queue drop | DDS thật; kiểm tra droppedPackets tăng |
| TEST-36 | Một cửa sổ, ba tab Hybrid, chi tiết Slave/Application/Flow và resize | QML thật; ảnh từng trang và log lỗi |
| TEST-37 | Shutdown Qt, Master/Agent restart, SQLite integrity | Tiến trình cục bộ thật và integrity_check |
| TEST-38 | Retention tắt/bật, giữ Active, nâng cấp schema cũ | SQLite thật trong thư mục tạm |
| TEST-39 | Database hỏng vẫn nhận telemetry; lỗi hiện trên QML | Fixture database hỏng riêng, final_fix_ui_regression.py và corruptEventDatabaseDoesNotBlockTelemetry |
| TEST-40 | RSS/CPU/tiến trình và GUI tiếp tục cập nhật | continuation_integration.py với thời gian lấy mẫu được chỉ định |

Kịch bản local_integration.py kiểm tra 14 hành vi cũ. Kịch bản continuation_integration.py kiểm tra chức năng mới và ghi resource-usage.json, ảnh từng trang, snapshot trước/sau restart cùng kết quả SQLite/QML. Mọi tiến trình do test sở hữu được dừng trong finally; không chỉnh dịch vụ, NTP hoặc firewall.

Regression Final Fix bổ sung `agentRestartAfterClockRollbackRecovers`, `activeAlertsAreUnverifiedWithoutCurrentEvidence` (Unknown và thiếu ứng dụng), cùng kiểm tra thực tế Label QML trong `final_fix_ui_regression.py`. Phải giữ Active khi chưa có bằng chứng, không gắn Verified và chỉ Recovered sau Running. Snapshot UI xuất `status_text`/`status_visible` đọc từ Label thật, không phải suy ra từ core.status.

## Kiểm thử phải thực hiện bên ngoài

- Build MSVC bằng Qt/Fast DDS/Fast CDR cùng kiến trúc; chạy Agent Windows và từng API CPU/RAM/disk/network/process/SCM/CIM.
- Windows Time với nhiều ngôn ngữ, thiếu counter/quyền, service dừng và nguồn không khả dụng; chưa được báo synchronized chỉ vì Running.
- Master Linux/Slave Windows vật lý: discovery, NIC/firewall thực tế, mất mạng, tắt máy, reconnect và GUID restart.
- NTP/Chrony/Windows Time sau khi quản trị viên áp dụng cấu hình: nguồn đúng Master, startup convergence, mất nguồn và offset hai máy.
- Kiểm thử dài hạn theo thời gian/quy mô được thống nhất; không đổi bài 60/120 giây cục bộ thành nghiệm thu 24 giờ.
- Dynamic Chart thống kê gộp/cumulative và tình huống PoC gốc: tái hiện có kiểm soát, sửa và thêm hồi quy trước khi đóng lỗi.

Các ngưỡng nghiệm thu chính thức về số máy, độ trễ, độ chính xác đồng hồ và tải chưa được tự đặt trong bộ test này.
