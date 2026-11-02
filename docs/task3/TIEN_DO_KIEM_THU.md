# 📋 TIẾN ĐỘ TASK 3 — DDS MONITOR (Linux Master ↔ Windows Slave)

| Thông tin | Giá trị |
| :--- | :--- |
| Cập nhật | 02/11/2026 |
| Source | `/home/dev/DDS-Monitor` · HEAD `44e1337` |
| Linux Master | `172.20.10.2/28` · Xubuntu 22.04 · Qt 6.7.2 |
| Windows Slave | `172.20.10.3/28` · `windows-slave-a` |
| Mục tiêu thiết kế | **Nhiều Slave** (Linux và Windows) |
| Phạm vi test thực tế | **2 thiết bị**: Linux chỉ làm Master · Windows làm Slave |
| Backend vận hành | Fast DDS Statistics Backend **2.5.0 Original** |

**Ký hiệu:** ✅ PASS · 🟡 PARTIAL · ⬜ NOT RUN · ⛔ BLOCKED · ❌ FAIL · 🔴 ưu tiên cao · 🟠 trung bình · ⚪ thấp

> Cách dùng: làm xong thì đổi `[ ]` → `[x]`, cập nhật biểu tượng trạng thái và ghi ngày + bằng chứng vào cột *Ghi chú*.

---

## MỤC LỤC

1. [Tổng quan tiến độ](#1-tổng-quan-tiến-độ)
2. [⚠️ Yêu cầu mới: chuyển giao diện sang Qt WebEngine](#2-️-yêu-cầu-mới-chuyển-giao-diện-sang-qt-webengine)
3. [Việc ưu tiên](#3-việc-ưu-tiên)
4. [Chuẩn bị môi trường](#4-chuẩn-bị-môi-trường)
5. [Sự cố đang xử lý](#5-sự-cố-đang-xử-lý)
6. [Checklist kiểm thử E2E](#6-checklist-kiểm-thử-e2e)
7. [Ma trận yêu cầu FR / NFR / CON + đối chiếu đề bài](#7-ma-trận-yêu-cầu-fr--nfr)
8. [Vấn đề còn mở](#8-vấn-đề-còn-mở)
9. [Tài liệu cần sửa](#9-tài-liệu-cần-sửa)
10. [Nhật ký kiểm thử](#10-nhật-ký-kiểm-thử)

---

## 1. Tổng quan tiến độ

### 1.1. Kiểm thử E2E (26 ca)

| Trạng thái | Số ca | Tỷ lệ | Các ca |
| :--- | :---: | :---: | :--- |
| ✅ PASS | 3 | 12 % | 03 · 06 · 10 |
| 🟡 PARTIAL | 12 | 46 % | 01 · 02 · 04 · 05 · 07 · 08 · 09 · 11 · 14 · 16 · 17 · 21 |
| ⬜ NOT RUN | 8 | 31 % | 12 · 13 · 15 · 18 · 19 · 20 · 22 · 25 |
| ⛔ BLOCKED | 3 | 11 % | 23 · 24 · 26 |

```
PASS     ███░░░░░░░░░░░░░░░░░░░░░░░   3/26
PARTIAL  ████████████░░░░░░░░░░░░░░  12/26
NOT RUN  ████████░░░░░░░░░░░░░░░░░░   8/26
BLOCKED  ███░░░░░░░░░░░░░░░░░░░░░░░   3/26
```

### 1.2. Yêu cầu chức năng (15 FR)

| Mức | Số FR | FR |
| :--- | :---: | :--- |
| 🟡 PARTIAL | 13 | 01–07 · 09 · 11–15 |
| ⬜ NOT VERIFIED | 2 | 08 (đồng bộ giờ) · 10 (ánh xạ GUID) |

### 1.3. Tóm tắt một dòng

> Nguyên mẫu đã chạy trên 2 máy thật (telemetry, Online/Offline, process, cảnh báo, DDS HelloWorld cross-OS). Đã tìm ra nguyên nhân lỗi Graph. **Có yêu cầu mới chuyển UI sang Qt WebEngine** — cần chốt phạm vi trước khi tiếp tục sửa UI QML.

---

## 2. ⚠️ Yêu cầu mới: chuyển giao diện sang Qt WebEngine

> **Ghi nhận 02/11/2026:** được yêu cầu chuyển giao diện từ **Qt/QML + C++** sang dạng **Qt WebEngine** (HTML/CSS/JS hiển thị trong cửa sổ Desktop, C++ làm backend).
>
> ✅ **Đã chốt 02/11/2026:** dùng **Qt WebEngine** cho **UI Task 3**, giao diện **giống hệt bản QML + C++ hiện tại**. Màn DDS chuyên sâu gốc (Explorer, QoS, Domain Graph, Charts) **giữ QML**. Module WebEngine sẽ được cài sau trên máy Master.

### 2.1. Hiểu đúng yêu cầu

| Hạng mục | Hiện tại (QML) | Sau khi chuyển (WebEngine) |
| :--- | :--- | :--- |
| Loại ứng dụng | Desktop Qt | **Vẫn là Desktop Qt** (`QWebEngineView`) |
| Trình bày | QML (`Dashboard.qml`, `HostWorkspace.qml`…) | HTML/CSS/JS (có thể dùng thư viện vẽ đồ thị JS) |
| Logic | C++ (`MonitoringCore`, `Registry`, `DdsBridge`, `AlertStore`) | **Giữ nguyên C++** |
| Cầu nối C++ ↔ UI | `Q_PROPERTY` / signal sang QML | `QWebChannel` (C++ object → JavaScript) |
| Web Server | Không | **Không cần** — trang nạp từ `qrc:/` hoặc file cục bộ |

> ✅ Không vi phạm CON-04 ("không xây Web Server") nếu nạp trang cục bộ qua `QWebChannel`, không mở cổng HTTP. Nên ghi rõ điều này trong báo cáo.

### 2.1b. 📌 Yêu cầu bắt buộc: giao diện giữ tương đương bản QML

> **UI sau khi chuyển sang WebEngine phải hiển thị tương tự bản Qt/QML + C++ hiện tại.** Chỉ đổi công nghệ hiển thị, không đổi bố cục, nội dung hay cách thao tác của người dùng.

| Xong | Mã | Tiêu chí tương đương | Cách kiểm |
| :---: | :--- | :--- | :--- |
| [ ] | UI-EQ-01 | Giữ **cùng bố cục**: một cửa sổ, ba tab *Giám sát hệ thống* · *DDS chuyên sâu* · *Cảnh báo & lịch sử* | Chụp ảnh QML vs Web cùng kích thước 1600×900, đặt cạnh nhau |
| [ ] | UI-EQ-02 | Giữ **cùng thông tin** trên từng màn: Host, đèn xanh/đỏ + nhãn chữ, Last Seen, CPU/RAM/Disk/Network, ứng dụng, luồng, đồng bộ giờ | Checklist từng trường so với `Dashboard.qml`, `HostWorkspace.qml` |
| [ ] | UI-EQ-03 | Giữ **cùng màu sắc, biểu tượng, nhãn trạng thái** (Online · Fresh, Offline · Stale, Unknown, Active, Recovered, Not verified) | So `Theme` QML với CSS |
| [ ] | UI-EQ-04 | Giữ **cùng thao tác**: chọn Host, chọn luồng, chuyển tab giữ lựa chọn, cuộn danh sách Network không nhảy, Load Older | Chạy lại E2E-20 và thao tác cuộn |
| [ ] | UI-EQ-05 | Giữ **cùng đơn vị và định dạng số** (MiB, %, B/s, µs, giờ ISO) | So số liệu cùng một snapshot |
| [ ] | UI-EQ-06 | Dữ liệu cập nhật **realtime như QML** (theo chu kỳ telemetry, không phải bấm reload) | Quan sát Last Seen tăng mỗi giây |
| [ ] | UI-EQ-07 | Màn **DDS chuyên sâu** (Explorer, QoS, Domain Graph, Charts) vẫn hoạt động như cũ khi chuyển tab giữa trang web Task 3 và màn QML gốc | Màn này **giữ QML**, không chuyển |
| [ ] | UI-EQ-08 | Chạy **offline hoàn toàn** (không CDN, không cổng HTTP) | `ss -ltnp` không có cổng mới của `dds_monitor` |
| [ ] | UI-EQ-09 | Hiệu năng không kém QML: mở app, chuyển tab, cập nhật không giật | Đo thời gian mở, CPU của Master |

> Cách nghiệm thu đề xuất: với mỗi màn hình, chụp **cặp ảnh QML / WebEngine** cùng thời điểm, cùng dữ liệu → lưu vào `test-artifacts/ui-webengine-parity/`.

### 2.2. Câu hỏi cần chốt với người giao việc

| Xong | Câu hỏi | Trả lời |
| :---: | :--- | :--- |
| [x] | Chuyển **toàn bộ** UI (cả màn DDS gốc của eProsima) hay chỉ **phần mở rộng Task 3**? | **Chỉ UI Task 3** (Giám sát hệ thống, Cảnh báo & lịch sử, luồng/topology Task 3). Chốt 02/11 |
| [x] | Domain Graph gốc (`DomainGraphLayout.qml`) giữ QML hay vẽ lại bằng web? | **Giữ QML gốc** (màn DDS chuyên sâu không chuyển) → vẫn cần sửa GRAPH-01 |
| [ ] | Có được dùng thư viện JS bên ngoài (vd. ECharts, Cytoscape.js, D3) không? Đóng gói offline? | |
| [ ] | Hạn hoàn thành? Có cần giữ song song bản QML để so sánh không? | |
| [x] | Có dùng Qt WebEngine không? Máy đã có module chưa? | **Dùng WebEngine.** Máy Master **chưa có** module → người thực hiện sẽ cài sau |
| [x] | Giao diện mục tiêu? | **Giống hệt UI QML + C++ Task 3 hiện tại** (xem 2.1b) |

### 2.3. Kế hoạch chuyển đổi (đề xuất)

| Xong | Bước | Mô tả | Ghi chú |
| :---: | :--- | :--- | :--- |
| [ ] | W-01 | Cài module Qt WebEngine + WebChannel cho Qt 6.7.2 | 02/11: **chưa có** trong `~/Qt/6.7.2/gcc_64/lib` → cài qua Qt Maintenance Tool |
| [ ] | W-02 | PoC: cửa sổ `QWebEngineView` nạp `qrc:/web/index.html`, gọi được 1 hàm C++ | |
| [ ] | W-03 | Expose `MonitoringCore` qua `QWebChannel` (hosts, events, flows, status) | Tái dùng Q_PROPERTY sẵn có |
| [ ] | W-04 | Trang **Giám sát hệ thống**: danh sách Slave, đèn xanh/đỏ, CPU/RAM/Disk/Network | Thay `Dashboard.qml`, `HostWorkspace.qml` |
| [ ] | W-05 | Trang **Luồng DDS / Topology** của Task 3: Host → App → Writer → Topic → Reader | Thay `FlowDetails.qml` (không phải Domain Graph gốc) |
| [ ] | W-06 | Trang **Cảnh báo & lịch sử**: Active/Recovered, Load Older | Thay `EventHistoryView.qml` |
| [ ] | W-07 | Biểu đồ CPU / Throughput / Latency (ghi đơn vị, cửa sổ thời gian) | |
| [ ] | W-08 | Tích hợp: tab Task 3 dùng `QWebEngineView`, tab **DDS chuyên sâu giữ QML gốc** trong cùng một cửa sổ | Đã chốt 02/11 |
| [ ] | W-09 | Đóng gói offline (không tải CDN), kiểm tra không mở cổng mạng | Chứng minh CON-04 |
| [ ] | W-10 | Chạy lại E2E-06 → E2E-11, E2E-20 trên UI mới | Kết quả QML cũ không tự chuyển sang |
| [ ] | W-11 | Cập nhật báo cáo / Chương 5 (kiến trúc UI, hình chụp mới) | |

### 2.4. Ảnh hưởng tới các việc khác

| Việc | Ảnh hưởng |
| :--- | :--- |
| GRAPH-01 (lỗi QML Graph) | Domain Graph **giữ QML** → **vẫn phải sửa** (bản sửa 1 dòng đã có, mục 5.1) |
| E2E-20 (ba tab, giữ lựa chọn) | Phải test lại trên UI web |
| NET-01, P0-BACKEND, NTP, QoS | **Không ảnh hưởng** — thuộc Agent / Backend / OS, vẫn làm tiếp |
| Kết quả PASS cũ (E2E-06, 10) | Phần dữ liệu vẫn đúng; phần hiển thị cần chụp lại trên UI mới |

---

## 3. Việc ưu tiên

| # | Việc | Ưu tiên | Lý do |
| :---: | :--- | :---: | :--- |
| 1 | Cài Qt WebEngine + WebChannel cho Qt 6.7.2 (W-01) | 🔴 | Đã chốt dùng WebEngine; chặn toàn bộ việc chuyển UI |
| 2 | Quyết định Backend Original / Patched sau kết quả P0 ngày 02/11 | 🔴 | Original đã crash lại |
| 3 | E2E-12 trên 2 máy (Linux Pub ↔ Windows Sub) | 🟠 | Cục bộ đã PASS |
| 4 | PoC WebEngine (W-01 → W-03) | 🟠 | Đánh giá khả thi sớm |
| 5 | E2E-13 GUID, E2E-14 Throughput/Latency | 🟠 | FR-10, FR-13 |
| 6 | NTP (E2E-22 → 24) | 🟠 | Cần quyền quản trị |

---

## 4. Chuẩn bị môi trường

| Xong | Hạng mục | Đường dẫn / lệnh | Ghi chú |
| :---: | :--- | :--- | :--- |
| [x] | Backend Original build từ source sạch v2.5.0 | `/home/dev/dds-e2e/backend-original-install/` | Commit `314fcd6e` · 26/10 |
| [x] | Script chạy Master + Backend Original | `bash task3/scripts/run_master_original.sh` | Tạo 02/11 · kiểm `/proc/<pid>/maps` |
| [ ] | Chạy thử script Original trên GUI | như trên | Chờ dòng `Verified process … backend-original-install` |
| [x] | Bản debug có log `[GRAPH01]` | `/home/dev/dds-e2e/graph01-build-debug-original/dds_monitor` | `TASK3_MASTER_BIN=<path> bash task3/scripts/run_master_original.sh` |
| [x] | Windows Agent (Release) | `D:\Thang_1910\DDS-Monitor\build-agent-windows\Release\task3_agent.exe` | E2E-03 |
| [x] | HelloWorld Linux Pub / Windows Sub | `/home/dev/dds-tests/hello_world/build/hello_world` | Bật `FASTDDS_STATISTICS` |
| [x] | File QoS thử nghiệm | `/home/dev/dds-e2e/qos/compatible.xml`, `incompatible-writer.xml` | Dùng cho E2E-12 |
| [ ] | Baseline: SHA binary, commit, `ldd`, IP, firewall | — | Cho E2E-01 |
| [ ] | Module Qt WebEngine | `~/Qt/6.7.2/gcc_64` | Cho W-01 |

---

## 5. Sự cố đang xử lý

### 5.1. 🔴 GRAPH-01 — Hộp thoại `Loading Graph` không đóng

**Nguyên nhân (xác nhận 02/11 bằng test tự động + stack trace):**

```
Graph có ≥ 2 host (Linux Publisher + Windows Subscriber)
  → hostsList (ListView ngoài cùng) không có width → width = 0
  → mỗi lần resize() đặt hostsList.currentIndex = c, ListView chỉ giữ currentItem,
    hủy delegate host còn lại (cả cây user/process/participant/endpoint bên trong)
  → record_connections_ chỉ tới listener của 1 participant
  → endpoint còn lại kẹt trong pending_connections_
  → không gọi stop_timer() / endpoints_updated_() → không initialized()
  → safety_timer gọi load_model() lặp lại cùng kết quả → Loading Graph không đóng
```

**Bản sửa đề xuất (1 dòng, chưa áp vào code gốc):** thêm `width: entity_box_width_` cho `hostsList` trong `qml/DomainGraphLayout.qml`
→ `test-artifacts/original-20261102/graph01/graph01-hostslist-width.patch`

| Kết quả test tự động (`task3/tests/qml/graph/tst_DomainGraphLoading.qml`) | Code gốc | Có bản sửa |
| :--- | :---: | :---: |
| 1 host / 1 participant | ✅ | ✅ |
| 2 host (Linux Pub + Windows Sub) | ❌ | ✅ |
| 3 host, 1 process có 2 participant, 6 endpoint (đủ 6 kết nối) | ❌ | ✅ |
| Refresh 10 lần liên tiếp với 2 host | ❌ | ✅ |
| **Tổng** | **3/6** | **6/6** |

| Xong | Bước | Ghi chú |
| :---: | :--- | :--- |
| [x] | Thêm log chẩn đoán vào bản diagnostic | `graph01-diagnostic/graph01-logs.diff` |
| [x] | Thu log lỗi thật | `/home/dev/dds-e2e/graph01-data.*/monitor.log` |
| [x] | Xác định nguyên nhân | Xem sơ đồ trên · 02/11 |
| [x] | Tái hiện tự động, không cần GUI | `qmltestrunner` offscreen · 02/11 |
| [x] | Thử và kiểm chứng bản sửa | `cacheBuffer`, `displayMargin`, `highlightFollowsCurrentItem` **không** sửa được; `width` sửa được · ảnh `graph01/graph-three-hosts-fixed.png` |
| [x] | Quyết định WebEngine (mục 2.2) | 02/11: Domain Graph giữ QML → cần áp bản sửa |
| [ ] | Phê duyệt sửa code gốc QML | Bản sửa chỉ 1 dòng, rủi ro thấp |
| [ ] | Áp patch, build lại Master | `git apply test-artifacts/original-20261102/graph01/graph01-hostslist-width.patch` |
| [ ] | Kiểm tra trên GUI thật: Refresh ≥ 10 lần (Linux Pub + Windows Sub) | |
| [ ] | Stop/restart Subscriber rồi Refresh | |
| [ ] | Writer/Reader hiện đúng trong viewport (GRAPH-02) | Test tự động đã thấy đủ 6 endpoint hiển thị; có thể GRAPH-02 cùng nguyên nhân |

Chạy lại test: `QT_QPA_PLATFORM=offscreen qmltestrunner -input task3/tests/qml/graph -import imports` (tại thư mục gốc repo).

### 5.2. 🔴 P0-BACKEND — Crash khi Publisher stop/restart

| Xong | Bước | Ghi chú |
| :---: | :--- | :--- |
**Kết quả stress 02/11 (Linux, offscreen, GDB, 30 chu kỳ Publisher stop/restart, dwell 2 s):**

| Lượt | Backend | Kết quả | Ghi chú |
| :--- | :--- | :---: | :--- |
| publisher-30 | Original | 🟡 30/30 | Master **không thoát** trong 25 s sau SIGTERM → bị kill (`-9`) |
| shutdown-check-1, 2 | Original | ✅ 3/3 · 3/3 | Thoát sạch |
| publisher-30-rerun | Original | ❌ **SIGSEGV chu kỳ 2** | `DatabaseDataQueue::process_sample()` → `DatabaseQueue::run()` · trùng chữ ký P0 cũ · chạy song song với test QML (máy có tải) |
| patched run1, run2 | Patched | ✅ 30/30 · 30/30 | Thoát sạch, không SIGSEGV |

> **Kết luận:** lỗi P0 **vẫn còn** ở Backend 2.5.0 Original (tái hiện 1/4 lượt Original, 0/2 lượt Patched). Nếu tiếp tục dùng Original theo quyết định chủ dự án thì phải ghi đây là **rủi ro chấp nhận**: Master có thể crash khi ứng dụng DDS khởi động lại nhiều lần.
> Bằng chứng: `test-artifacts/original-20261102/` (README, `publisher-30-rerun/crash-backtrace.txt`).

| Xong | Bước | Ghi chú |
| :---: | :--- | :--- |
| [x] | Tái hiện với bản chưa vá (lịch sử) | Crash chu kỳ 12/30 · patched 30/30 |
| [x] | Stress ≥ 30 chu kỳ Publisher với **Original** | 02/11 · crash 1/4 lượt · treo khi thoát 1/4 |
| [x] | So sánh cùng kịch bản với **Patched** | 02/11 · 2/2 PASS |
| [ ] | Stress Subscriber restart với Original | `subscriber_restart_regression.py` |
| [ ] | Chạy thêm ≥ 5 lượt Original khi máy rảnh (không chạy song song việc khác) để ước lượng tần suất | |
| [ ] | **Quyết định của chủ dự án:** giữ Original (chấp nhận rủi ro) hay chuyển Patched | Cần ghi vào báo cáo |

---

## 6. Checklist kiểm thử E2E

### 6.1. Nền tảng & build

| Xong | ID | Nội dung | TT | Đã làm | Còn thiếu |
| :---: | :--- | :--- | :---: | :--- | :--- |
| [ ] | E2E-01 | Baseline source, IP, topology | 🟡 | IP khớp, có mốc Git | Phiếu metadata |
| [ ] | E2E-02 | Multicast, firewall | 🟡 | Ping OK, DDS chạy | Biên bản UDP/locator |
| [x] | E2E-03 | Build Windows Agent | ✅ | Release build + chạy | — |
| [ ] | E2E-04 | `--once`, collector | 🟡 | JSON đúng | NET-01 |
| [ ] | E2E-05 | Master build, CTest | 🟡 | Linkage xác minh | CTest với Original |

### 6.2. Giám sát máy (Agent)

| Xong | ID | Nội dung | TT | Đã làm | Còn thiếu |
| :---: | :--- | :--- | :---: | :--- | :--- |
| [x] | E2E-06 | Host Online, Last Seen | ✅ | Online · Fresh | — |
| [ ] | E2E-07 | CPU | 🟡 | 1 % vs 1,6 % | Mẫu đồng thời |
| [ ] | E2E-08 | RAM, Disk, Network | 🟡 | RAM, C: khớp | NET-01 |
| [ ] | E2E-09 | Process / Service | 🟡 | Process Active→Recovered | Windows Service |
| [x] | E2E-10 | Agent Offline → Online | ✅ | `agent_timeout` Active→Recovered | — |

### 6.3. DDS

| Xong | ID | Nội dung | TT | Đã làm | Còn thiếu |
| :---: | :--- | :--- | :---: | :--- | :--- |
| [ ] | E2E-11 | Entities cross-OS | 🟡 | SENT/RECEIVED, thấy Domain 0 | Graph, thống kê |
| [ ] | E2E-12 | QoS MATCH / NO MATCH | 🟡 | 02/11 **cục bộ Linux PASS 3/3**: RELIABLE↔RELIABLE match 79/79 · Writer BE ↔ Reader RELIABLE **no match** 0 mẫu · BE↔BE match (chứng minh XML được nạp) · `test-artifacts/original-20261102/qos-local/` | Chạy Linux Pub ↔ Windows Sub; xem Monitor hiển thị QoS / incompatible |
| [ ] | E2E-13 | GUID mapping | ⬜ | Test Linux | Chạy 2 máy |
| [ ] | E2E-14 | Throughput / Latency | 🟡 | Có số liệu | Đo định lượng |

### 6.4. Trạng thái & cảnh báo

| Xong | ID | Nội dung | TT | Đã làm | Còn thiếu |
| :---: | :--- | :--- | :---: | :--- | :--- |
| [ ] | E2E-15 | Debounce | ⬜ | Unit test | Test thật |
| [ ] | E2E-16 | Unknown khi mất Agent | 🟡 | Thấy ở E2E-10 | Biến thể khác |
| [ ] | E2E-17 | Host Offline alert đầy đủ | 🟡 | Một phần | Kịch bản đủ |
| [ ] | E2E-18 | DDS No Data | ⬜ | — | Cấu hình `flow-rules.json` |
| [ ] | E2E-19 | SQLite > 500 sự kiện | ⬜ | Unit test | Máy thật |
| [ ] | E2E-20 | Ba tab, giữ lựa chọn | ⬜ | Tab hiển thị | **Test trên UI WebEngine** |
| [ ] | E2E-21 | Fresh / Stale / time Unknown | 🟡 | Fresh/Stale OK | Time Unknown |

### 6.5. Thời gian & khởi động lại

| Xong | ID | Nội dung | TT | Đã làm | Còn thiếu |
| :---: | :--- | :--- | :---: | :--- | :--- |
| [ ] | E2E-22 | Baseline NTP chỉ đọc | ⬜ | — | `chronyc tracking`, `w32tm /query /status` |
| [ ] | E2E-23 | Chrony → Windows Time | ⛔ | — | Quyền NTP |
| [ ] | E2E-24 | Mất NTP → đồng bộ lại | ⛔ | — | Phụ thuộc 23 |
| [ ] | E2E-25 | Master restart, SQLite | ⬜ | — | Đối chiếu lịch sử |
| [ ] | E2E-26 | Windows reboot | ⛔ | — | Quyền reboot |

---

## 7. Ma trận yêu cầu FR / NFR

### 7.1. Yêu cầu chức năng

| Xong | FR | Yêu cầu | Mức | E2E | Ghi chú |
| :---: | :--- | :--- | :---: | :--- | :--- |
| [ ] | FR-01 | Danh sách / chọn Slave | 🟡 | 06, 20 | 1 Slave thật |
| [ ] | FR-02 | Online / Offline / Unknown | 🟡 | 06, 10, 21 | Thiếu reboot |
| [ ] | FR-03 | CPU | 🟡 | 07 | |
| [ ] | FR-04 | RAM | 🟡 | 08 | |
| [ ] | FR-05 | Ổ đĩa | 🟡 | 08 | |
| [ ] | FR-06 | Mạng | 🟡 | 08 | NET-01 |
| [ ] | FR-07 | Process / Service | 🟡 | 09, 15 | Service chưa test |
| [ ] | FR-08 | Đồng bộ thời gian | ⬜ | 21–24 | |
| [ ] | FR-09 | Thực thể DDS | 🟡 | 11 | |
| [ ] | FR-10 | Ánh xạ Host / App / GUID | ⬜ | 13 | |
| [ ] | FR-11 | Quan hệ truyền nhận (Graph) | 🟡 | 11 | GRAPH-01 đã có bản sửa 1 dòng, test tự động 6/6 · chờ áp + test GUI |
| [ ] | FR-12 | Matching, QoS | 🟡 | 12 | QoS match/no-match PASS cục bộ |
| [ ] | FR-13 | Throughput, Latency | 🟡 | 14 | |
| [ ] | FR-14 | Cảnh báo | 🟡 | 09, 10, 15–18 | DDS rule chưa test |
| [ ] | FR-15 | Lịch sử sự kiện | 🟡 | 19, 25 | |

### 7.2. Yêu cầu phi chức năng

| Xong | NFR | Nội dung | Ghi chú |
| :---: | :--- | :--- | :--- |
| [ ] | NFR-01 | Mở rộng nhiều Slave | Chỉ có 2 máy → test 2–3 Slave (thật + mô phỏng); quy mô lớn dựa trên unit test tải tổng hợp (128 host), ghi rõ là mô phỏng |
| [ ] | NFR-02 | Ổn định, phục hồi | Agent recovery OK · P0 **tái hiện với Original** 02/11 |
| [ ] | NFR-03 | Kịp thời | Timeout 5000 ms OK · chưa đo độ trễ |
| [ ] | NFR-04 | Dễ quan sát | NET-01, GRAPH-02 · đánh giá lại với UI web |
| [ ] | NFR-05 | Tương thích Linux–Windows | HelloWorld + Agent OK |

### 7.3. Ràng buộc CON

| Xong | CON | Nội dung | Cách chứng minh | Ghi chú |
| :---: | :--- | :--- | :--- | :--- |
| [x] | CON-01 | Master là Desktop Linux, C++ + Qt, mở rộng từ DDS Monitor | Source `task3/` + `main.cpp` | Áp dụng cả khi chuyển WebEngine |
| [x] | CON-02 | Dùng Fast DDS + Statistics Backend | `ldd`, `/proc/<pid>/maps` | Backend 2.5.0 Original |
| [ ] | CON-03 | Slave **Linux và Windows** | Windows ✅ test thật · Linux Slave: code + test cục bộ, chưa có máy để test thật | Giới hạn 2 thiết bị |
| [ ] | CON-04 | Không Web Server · không Prometheus · không điều khiển Slave từ xa | `ss -ltnp` không có cổng HTTP; rà code không có lệnh start/stop từ xa | Cần lập biên bản, đặc biệt sau khi chuyển WebEngine |

### 7.4. Đối chiếu với đề bài gốc (Task3_Require · Task3_continue)

Những ý trong đề bài **chưa có ca test riêng** ở mục 6. Cần bổ sung:

> ⚠️ **Mục tiêu và phạm vi test:** hệ thống thiết kế cho **nhiều Slave** (Linux và Windows). Thực tế chỉ có **2 máy**: Linux **chỉ làm Master**, Windows làm Slave. Báo cáo ghi rõ giới hạn này; không ghi là đã test trên Linux Slave hay nhiều máy vật lý.

| Xong | Mã | Ý trong đề bài | Hiện trạng | Cách test với 2 máy |
| :---: | :--- | :--- | :--- | :--- |
| [ ] | REQ-01 | **Slave Linux** gửi telemetry về Master | Có code Linux collector + test cục bộ trên Linux | **Chưa test thật** (không có máy Linux Slave). Ghi là giới hạn; làm khi có thêm máy. |
| [ ] | REQ-02 | **Nhiều Slave cùng lúc** (máy A, B, …) | Mới 1 Slave thật | *Tùy chọn mở rộng:* chạy thêm 1 Agent trên Windows với `host_id` khác (vd. `windows-slave-b`) → kiểm tra 2 Slave trên một màn hình, đèn, chọn máy. Ghi rõ là **mô phỏng**, dùng chung phần cứng. |
| [ ] | REQ-03 | **Trao đổi trong cùng một máy** (pm A1 ↔ A2 trong Slave A) | Mới test Linux → Windows | Chạy HelloWorld **Publisher + Subscriber cùng trên Windows** → kiểm tra luồng bên trong `windows-slave-a`. Không cần máy thứ 3. |
| [ ] | REQ-04 | Hiển thị luồng **theo từng máy**: bên trong Slave thấy các phần mềm, thông lượng, có trao đổi được không, lỗi gì | Có topology Host → App → Writer → Topic → Reader (test Linux) | Kiểm tra trên 2 máy thật; áp dụng cho cả UI WebEngine |
| [ ] | REQ-05 | **Đồng bộ giờ khi Slave khởi động** (dịch vụ tự chạy khi boot, tự đồng bộ lại khi có mạng) | Có template `task3/config/time-sync/` | Gộp vào E2E-23, 24, 26: reboot Windows → kiểm tra w32time tự chạy và đồng bộ |
| [ ] | REQ-06 | Phân biệt: **dịch vụ đang chạy / có nguồn thời gian / đã đồng bộ**; offset không có thì hiện "chưa có dữ liệu", không hiện 0 | Code có `time_sync.state` + offset `null` | Kiểm tra trên UI khi NTP chưa cấu hình (E2E-21, 22) |
| [ ] | REQ-07 | Phát hiện **ngừng truyền dữ liệu** (Writer còn sống nhưng không gửi) | Có rule `dds_no_data` (test Linux) | E2E-18 với `hello_world_topic` dạng periodic |
| [ ] | REQ-08 | Phát hiện **mất kết nối DDS** (Reader/Writer hết matched) | Chưa có ca riêng | Tắt Subscriber / rút mạng → kiểm tra trạng thái Unmatched + cảnh báo |
| [ ] | REQ-09 | Phát hiện **hoạt động bất thường** (ngưỡng throughput, CPU…) | Có hysteresis throughput (unit test) | Chốt ngưỡng cụ thể rồi test thật |
| [ ] | REQ-10 | Hiển thị **QoS liên quan**: Reliability, History, Durability, Deadline, Liveliness | Màn DDS gốc có QoS | Chụp màn QoS của Writer/Reader HelloWorld; ghép vào E2E-12 |
| [ ] | REQ-11 | **Network Latency** và **Physical Data** (Host/User/Process) | Chưa ghi nhận riêng | Kiểm tra trong E2E-14; ghi đơn vị |
| [ ] | REQ-12 | Kiểm tra **tương thích phiên bản** Fast DDS / dependency giữa Linux và Windows (NFR-05) | Linux 3.6.1; Windows chưa ghi phiên bản | Ghi phiên bản Fast DDS/Fast CDR trên Windows vào E2E-01 |
| [ ] | REQ-13 | Hiển thị Slave **"Chưa xác định"** khi chưa nhận bản tin đầu tiên | Code có Unknown | Mở Master trước khi chạy Agent → chụp màn hình |
| [ ] | REQ-14 | Chạy **ổn định liên tục** trong thời gian dài | Mới test ngắn (~2 phút cục bộ) | Chạy 2 máy ≥ thời gian đã chốt ở 7.5 |

### 7.5. Thông số cần chốt trước khi nghiệm thu

> Theo Task3_continue mục 2.2: các thông số này phải thống nhất **trước** khi kiểm thử; hiện chưa chốt.

| Xong | Thông số | Giá trị mặc định trong code | Giá trị chốt |
| :---: | :--- | :--- | :--- |
| [ ] | Số Slave mục tiêu | Tối đa 512 host (giới hạn code) | |
| [ ] | Chu kỳ lấy mẫu Agent | `interval_ms` = 1000 ms | |
| [ ] | Timeout Online → Offline | `TASK3_TIMEOUT_MS` = 5000 ms | |
| [ ] | Độ trễ cập nhật UI chấp nhận được | — | |
| [ ] | Độ lệch thời gian cho phép (offset NTP) | — | |
| [ ] | Thời gian chạy liên tục để nghiệm thu | — | |
| [ ] | Ngưỡng cảnh báo (CPU, throughput, No Data deadline) | `flow-rules.json`: 1500 ms / 15000 ms | |
| [ ] | Thời hạn lưu sự kiện SQLite | Retention mặc định **tắt** | |

---

## 8. Vấn đề còn mở

| Xong | Mã | Vấn đề | Ưu tiên | Ghi chú |
| :---: | :--- | :--- | :---: | :--- |
| [ ] | UI-WEB | Chuyển UI sang Qt WebEngine | 🔴 | Yêu cầu mới 02/11 · mục 2 |
| [ ] | GRAPH-01 | `Loading Graph` không đóng | 🔴 | Đã sửa được trong test (1 dòng `width`) · chờ phê duyệt áp vào code gốc |
| [ ] | P0-BACKEND | Crash lifecycle với Original | 🔴 | 02/11 **tái hiện lại** SIGSEGV với Original; Patched 2/2 PASS · cần quyết định |
| [ ] | TIME-01 | NTP Master–Windows | 🔴 | Cần quyền |
| [ ] | STAT-01 | Độ chính xác Throughput / Latency | 🟠 | |
| [ ] | GRAPH-02 | Writer/Reader không rõ trong viewport | 🟠 | Có thể liên quan GRAPH-01 |
| [ ] | NET-01 | Interface Windows trùng bộ đếm (19, 25–29, 47–50) | 🟠 | Lỗi ở collector |
| [ ] | ID-01 | GUID mapping 2 máy | 🟠 | |
| [ ] | SERVICE-01 | Windows Service | 🟠 | |
| [ ] | HISTORY-01 | SQLite restart / > 500 sự kiện | 🟠 | |

---

## 9. Tài liệu cần sửa

| Xong | Việc | Ghi chú |
| :---: | :--- | :--- |
| [ ] | Bỏ mô tả **HMAC-SHA256 / `TASK3_KEY`** (báo cáo mục 8.4, 14.2; Chương 5 dòng 150) | Code không có HMAC |
| [ ] | Chương 5: `HostTelemetry/v1` → `Task3/HostTelemetry/v1` | |
| [ ] | `TASK3_README.md`: thêm `run_master_original.sh` | |
| [ ] | `KNOWN_ISSUES.md` mục 1: Windows Agent đã build/chạy | |
| [ ] | Báo cáo: thêm kết quả chẩn đoán GRAPH-01 | |
| [ ] | Báo cáo / Chương 5: kiến trúc UI WebEngine + `QWebChannel`, giải thích không vi phạm CON-04 | Sau khi có PoC |

---

## 10. Nhật ký kiểm thử

| Ngày | Ca / Việc | Kết quả | Bằng chứng | Người chạy |
| :--- | :--- | :---: | :--- | :--- |
| 23/10/2026 | E2E-06, 09, 10 | ✅ / 🟡 | Ảnh Host Online, lịch sử Active/Recovered | Thắng |
| 26/10/2026 | A/B Backend · Graph Refresh | 5/5 mỗi bản | Ảnh 20, 21 | Thắng |
| 26/10/2026 | Chẩn đoán GRAPH-01 | 8/9 kẹt · rõ nguyên nhân | `graph01-data.*/monitor.log` | Thắng |
| 02/11/2026 | Nhận yêu cầu chuyển UI sang WebEngine | Ghi nhận | — | Thắng |
| 02/11/2026 | GRAPH-01 test tự động + bản sửa | Gốc 3/6 · sửa 6/6 | `test-artifacts/original-20261102/graph01/` | Claude |
| 02/11/2026 | E2E-12 QoS cục bộ Linux | ✅ 3/3 | `test-artifacts/original-20261102/qos-local/result.json` | Claude |
| 02/11/2026 | P0 stress Original vs Patched | Original: crash 1/4, treo thoát 1/4 · Patched 2/2 PASS | `test-artifacts/original-20261102/` | Claude |
| | | | | |
