# TIẾN ĐỘ KIỂM THỬ POC — TASK 3 DDS MONITOR (Linux Master ↔ Windows Slave)

| Thông tin | Giá trị |
| :--- | :--- |
| Cập nhật | 06/11/2026 (đã build và chạy Master với UI WebEngine) |
| Phạm vi | **PoC** trên 2 thiết bị: Linux chỉ làm Master, Windows làm Slave |
| Source | `main` `3c14b15` · UI WebEngine trên nhánh **`feature/webengine-ui`** (`a10ef23`, chưa merge) |
| Linux Master | `172.20.10.2/28` · Xubuntu 22.04 · **Qt 6.8.3** (WebEngine, WebChannel, Qt5Compat); Qt 6.7.2 không còn dùng |
| Windows Slave | `172.20.10.3/28` · `windows-slave-a` |
| Backend vận hành | Fast DDS Statistics Backend **2.5.0 Patched** (chốt 02/11/2026) |
| Mục tiêu thiết kế | Nhiều Slave (Linux và Windows); **kiểm thử trên hệ thống thật là hướng phát triển** (mục 6) |

**Ký hiệu:** ✅ PASS (chạy được trong kịch bản kiểm thử PoC) · ⬜ NOT RUN (chưa kiểm thử) · 🚧 chuyển sang hướng phát triển · 🔴 ưu tiên cao · 🟠 trung bình · ⚪ thấp

> Cách dùng: làm xong thì đổi `[ ]` → `[x]`, ghi ngày và bằng chứng vào cột *Ghi chú*. Cách đánh giá khớp Chương 5 và 6 của báo cáo (`A_TONGKET/BaoCao/`).

---

## MỤC LỤC

1. [Tổng quan tiến độ PoC](#1-tổng-quan-tiến-độ-poc)
2. [Việc cần làm tiếp](#2-việc-cần-làm-tiếp)
3. [Chuyển giao diện sang Qt WebEngine](#3-chuyển-giao-diện-sang-qt-webengine)
4. [Kịch bản kiểm thử PoC](#4-kịch-bản-kiểm-thử-poc)
5. [Vấn đề kỹ thuật](#5-vấn-đề-kỹ-thuật)
6. [Kiểm thử trên hệ thống thật (hướng phát triển)](#6-kiểm-thử-trên-hệ-thống-thật-hướng-phát-triển)
7. [Chuẩn bị môi trường](#7-chuẩn-bị-môi-trường)
8. [Tài liệu cần sửa](#8-tài-liệu-cần-sửa)
9. [Nhật ký kiểm thử](#9-nhật-ký-kiểm-thử)

---

## 1. Tổng quan tiến độ PoC

### 1.1. Theo mảng việc

| Mảng việc | TT | Kết quả PoC | Còn thiếu |
| :--- | :---: | :--- | :--- |
| Agent Windows: CPU/RAM/Disk/Network, process, Online/Offline | ✅ | Build Release, chạy thật; telemetry, Online → Offline → Online, process Running → Stopped → Running | Lỗi NET-01 (bộ đếm interface trùng) |
| Telemetry DDS Domain 42, Registry, MonitoringCore | ✅ | Chạy 2 máy thật; session/sequence; timeout 5000 ms | — |
| Cảnh báo và lịch sử SQLite | ✅ | `agent_timeout`, `process_stopped` Active → Recovered, hiển thị lịch sử | — |
| DDS (Statistics Backend): thực thể, luồng, thống kê | ✅ | HelloWorld Linux → Windows matched; thấy Domain, Participant, Topic, Writer, Reader | Ánh xạ GUID (FR-10) |
| QoS | ✅ | Match / no-match cục bộ Linux 3/3 | — |
| Ổn định Backend (P0) | ✅ | Chốt **Patched**: 0/7 lượt crash (Original 5/9) | Crash `writer_not_alive` của Original (không chặn) |
| GRAPH-01 (Loading Graph) | ✅ | Không còn lỗi trên Qt 6.8.3 (test QML 6/6) | Xác nhận trên giao diện thật |
| **Giao diện Qt WebEngine** | 🟡 ~95 % | UI web xong; build + CTest PASS (Debug, Release); chạy Master thật với Agent Linux cục bộ: Online/Offline, tài nguyên, cảnh báo, lịch sử; không có cổng TCP | Agent Windows trên UI mới, Refresh Graph trên app thật, chụp ảnh, merge |
| Đồng bộ thời gian NTP (FR-08) | ✅ | 06/11: Chrony trên Master, Windows Stratum 4 (ref 172.20.10.2); mất nguồn → resync lỗi, Master vẫn Online; có lại → 5/5 mẫu, lệch tối đa 21,5 ms. Agent không admin: Master hiển thị Unknown, không báo Synced giả | `--once` luôn `unknown` (BUG-L03) |
| Tài liệu / báo cáo | 🟡 | Chương 5, 6 viết lại theo PoC (06/11) | Mục 8 |

### 1.2. Theo yêu cầu (khớp Bảng 5.9 của báo cáo)

| Nhóm | PASS | NOT RUN |
| :--- | :---: | :--- |
| Chức năng (15 FR) | **15** | — |
| Phi chức năng (5 NFR) | **5** | — (NFR-01 là mô phỏng: tải 128 host, UI 512 Slave) |

### 1.3. Tóm tắt một dòng

> PoC đã chạy trên 2 máy thật: telemetry, Online/Offline, process, cảnh báo, DDS HelloWorld giữa Linux và Windows. Backend chốt Patched, GRAPH-01 hết trên Qt 6.8.3 (test QML). UI Task 3 đã chuyển sang Qt WebEngine: 06/11 đã cài Qt5Compat (build từ mã nguồn chính thức v6.8.3), sửa CMake, build Debug + Release và CTest PASS, chạy Master thật; còn kiểm tra với Agent Windows và Graph trên app thật.

---

## 2. Việc cần làm tiếp

### 2.1. Hoàn thiện PoC (theo thứ tự)

| # | Việc | Ưu tiên | Lý do |
| :---: | :--- | :---: | :--- |
| 1 | ✅ Cài **Qt5Compat** cho Qt 6.8.3 | 🔴 | 06/11: MaintenanceTool đòi tài khoản gốc nên build từ mã nguồn `code.qt.io/qt/qt5compat` tag `v6.8.3` (cần `libxkbcommon-dev`), chép 123 file mới vào `~/Qt/6.8.3/gcc_64` |
| 2 | ✅ `build_task3_patched.sh --jobs 4` + CTest | 🔴 | 06/11: lỗi CMake (`find_package` WebEngine nằm trong `task3/`, target chỉ thấy trong thư mục con) → chuyển lên `CMakeLists.txt` gốc; build Debug + Release PASS, CTest PASS, `task3_tests` 48/48 |
| 3 | 🟡 Chạy Master thật (`run_master_patched.sh`) với UI web | 🔴 | 06/11 với Agent Linux cục bộ: Online · Fresh, CPU/RAM/ổ đĩa, tắt Agent → Offline · Stale (N/A), tab Cảnh báo hiển thị 52 bản ghi từ SQLite. **Còn:** Agent Windows trên UI mới (W-10) |
| 4 | 🟡 Trên app thật: Refresh Graph ≥ 10 lần, dừng/chạy lại Subscriber | 🔴 | `ss -ltnp`: **không có cổng TCP listen** (06/11, CON-04). **Còn:** Graph cần HelloWorld Pub/Sub trên Domain 0 với `FASTDDS_STATISTICS` (Agent chỉ dùng Domain 42 nên không tạo Graph) |
| 5 | 🟡 Chụp lại ảnh giao diện trên UI WebEngine | 🟠 | Hình 5.10 đã thay bằng ảnh Cảnh báo & lịch sử (06/11). **Còn:** Hình 5.9 (Host Windows Online) |
| 6 | 🟡 NET-01 (trùng bộ đếm interface Windows) | 🟠 | 06/11: code đã sửa (bỏ dòng `FilterInterface`, tên interface lấy từ `Alias`), `task3_tests` 49/49 PASS trên Linux. **Còn:** build Agent Windows, chạy `--once`, so với `Get-NetAdapterStatistics` |
| 7 | Merge `feature/webengine-ui` vào `main` | 🟠 | Sau khi 2–4 PASS |
| 8 | (Tùy chọn) Chạy HelloWorld Pub + Sub cùng trên Windows; chạy thêm Agent `windows-slave-b` | ⚪ | Minh họa trao đổi trong một máy và 2 Slave, ghi rõ là mô phỏng |
| 9 | (Tùy chọn) FR-10: ứng dụng mẫu có manifest định danh | ⚪ | Chuyển 1 FR từ NOT RUN sang PASS |

### 2.2. Hoàn thiện báo cáo

| # | Việc | Ghi chú |
| :---: | :--- | :--- |
| 1 | Dựng lại docx bằng `_build_docx/run_all.sh` trên Windows | md đã sửa 06/11 |
| 2 | Xuất lại PNG Hình 5.5 (đã bỏ bước HMAC, đổi `WebFeed`) | Cần PlantUML |
| 3 | Sửa Hình 5.4 (thêm `WebFeed`, `WebEngineView`) | `images/chuong_5/_gen_hinh_5_04.py` |
| 4 | Thay Hình 5.9, 5.10 bằng ảnh UI WebEngine (sau bước 5 ở trên) | Hình 5.11 vẫn là Domain Graph với Backend Original |
| 5 | Sau khi app thật chạy: cập nhật Bảng 5.10, 5.11 và dòng UI-WEB | Bỏ câu "chưa chạy trong ứng dụng đầy đủ" |
| 6 | Rà lại Chương 1 (mục tiêu Task 3 còn ghi Qt/QML) | Thêm WebEngine |

---

## 3. Chuyển giao diện sang Qt WebEngine

> **Yêu cầu (02/11):** chuyển UI **Task 3** từ Qt/QML + C++ sang Qt WebEngine (HTML/CSS/JS trong cửa sổ Desktop, C++ làm backend), **giao diện giống hệt bản QML**. Màn DDS chuyên sâu gốc (Explorer, QoS, Domain Graph, Charts) **giữ QML**. Không vi phạm CON-04 vì trang nạp từ `qrc:/`, không mở cổng HTTP.

### 3.1. Tiêu chí tương đương với bản QML

| Xong | Mã | Tiêu chí | Kết quả / Còn thiếu |
| :---: | :--- | :--- | :--- |
| [x] | UI-EQ-01 | Cùng bố cục: một cửa sổ, ba tab | 8 trang so sánh cùng dữ liệu · 06/11 xác nhận trên app thật |
| [x] | UI-EQ-02 | Cùng thông tin trên từng màn | Cùng trường, cùng chữ |
| [x] | UI-EQ-03 | Cùng màu, biểu tượng, nhãn trạng thái | Màu lấy từ `Theme.qml` |
| [x] | UI-EQ-04 | Cùng thao tác (chọn Host, luồng, đổi tab, cuộn không nhảy, Load Older) | Test bấm tự động PASS. **Còn:** thử tay trên app thật |
| [x] | UI-EQ-05 | Cùng đơn vị và định dạng số | Dùng lại hàm định dạng QML |
| [x] | UI-EQ-06 | Cập nhật realtime như QML | 06/11 app thật + Agent Linux: Last Seen, biểu đồ CPU, Online → Offline tự cập nhật không cần reload |
| [ ] | UI-EQ-07 | Màn DDS chuyên sâu (giữ QML) chạy như cũ khi chuyển tab | 06/11: tab mở được (Explorer, Domain 0 View, Refresh). **Còn:** Graph với HelloWorld |
| [x] | UI-EQ-08 | Chạy offline, không CDN, không cổng HTTP | CSP `default-src 'none'`; test chặn ảnh/fetch/eval PASS; 06/11 `ss -ltnp`: không có cổng TCP listen (chỉ UDP của Fast DDS, vd. 7400) |
| [ ] | UI-EQ-09 | Hiệu năng không kém QML | Chrome: 512 Slave 41 ms, 500 sự kiện 28 ms; WebFeed giảm payload 133 KB → 19 KB. **Còn:** đo CPU Master |

### 3.2. Các bước

| Xong | Bước | Mô tả | Ghi chú |
| :---: | :--- | :--- | :--- |
| [x] | W-01 | Cài Qt có WebEngine + WebChannel + **Qt5Compat** | 06/11: Qt5Compat đã build từ mã nguồn v6.8.3 và cài |
| [x] | W-02 | PoC `QWebEngineView` + `QWebChannel` | PASS trên Qt 6.8.3, kể cả sandbox |
| [x] | W-02b | Build DDS Monitor bằng Qt 6.8.3 và hồi quy | 06/11: build Debug + Release, CTest PASS, Master khởi động và nạp Patched Backend |
| [x] | W-03 | Expose dữ liệu qua `QWebChannel` | 03/11: `task3::WebFeed`; 06/11 đã compile, test `webFeedPublishesTrimmedChangesOnly` PASS |
| [x] | W-03b | Chọn Qt tự động | CMake `TASK3_WEB_UI`; `qt_prefix.sh` |
| [x] | W-04 → W-07 | Trang Giám sát hệ thống, Luồng DDS, Cảnh báo & lịch sử, biểu đồ CPU | `task3/web/` · 2,4–4,4 % pixel khác QML |
| [x] | W-08 | Tích hợp: tab Task 3 dùng WebEngine, DDS chuyên sâu giữ QML | `TASK3_UI=qml` quay lại bản QML |
| [x] | W-09 | Đóng gói offline, không mở cổng | 06/11: `ss -ltnp` không có cổng TCP |
| [x] | W-09b | Test trang web không cần Qt | `python3 task3/tests/web/run_web_tests.py --output test-artifacts/<thư-mục>` · 17/17 PASS |
| [ ] | W-10 | Chạy lại kịch bản K-03 → K-08 trên UI mới | Chờ Qt5Compat và máy Windows |
| [ ] | W-11 | Cập nhật báo cáo (kiến trúc, hình chụp mới) | Chương 5 đã có phần kiến trúc; còn ảnh mới |

Lệnh build và chạy (sau khi cài Qt5Compat):

```bash
git checkout feature/webengine-ui
bash task3/scripts/build_task3_patched.sh --jobs 4
bash task3/scripts/run_master_patched.sh        # UI web mặc định; TASK3_UI=qml để xem bản cũ
ss -ltnp | grep dds_monitor                     # không được có cổng HTTP
```

---

## 4. Kịch bản kiểm thử PoC

| Xong | Mã | Kịch bản | TT | Bằng chứng / Ghi chú |
| :---: | :--- | :--- | :---: | :--- |
| [x] | K-01 | Build Windows Agent (Release) và chạy | ✅ | `D:\Thang_1910\DDS-Monitor\build-agent-windows\Release\task3_agent.exe` |
| [x] | K-02 | Build Master, CTest, kiểm thử tích hợp cục bộ | ✅ | CTest PASS · local 14/14 · continuation 15/15 · `test-artifacts/patched-20261102/` |
| [x] | K-03 | Host Online, Last Seen, CPU/RAM/ổ đĩa/mạng hiển thị trên Master | ✅ | 23/10: Online · Fresh; CPU ≈ 1 % vs 1,6 %; RAM, ổ C: khớp Windows |
| [x] | K-04 | Process Running → Stopped → Running, cảnh báo `process_stopped` Active → Recovered | ✅ | 23/10 |
| [x] | K-05 | Dừng Agent → Offline · Stale, `agent_timeout` Active; chạy lại → Online, Recovered | ✅ | 23/10 |
| [x] | K-06 | HelloWorld Linux Pub → Windows Sub; Master thấy Domain, Participant, Topic, Writer, Reader | ✅ | Ảnh Domain Graph |
| [x] | K-07 | QoS ghép cặp / không ghép cặp (cục bộ Linux) | ✅ | 3/3 · RELIABLE↔RELIABLE 79/79 · BE↔RELIABLE 0 mẫu · `test-artifacts/original-20261102/qos-local/` |
| [x] | K-08 | Lịch sử sự kiện hiển thị (SQLite) | ✅ | Ảnh Cảnh báo & lịch sử |
| [x] | K-09 | Stress Publisher stop/restart 30 chu kỳ: Original vs Patched | ✅ | Original crash 5/9 · **Patched 0/7 (210 chu kỳ)** · `original-20261102*/`, `patched-20261102/` |
| [x] | K-10 | Subscriber restart | ✅ | 6/6 cả hai bản |
| [x] | K-11 | Domain Graph QML (GRAPH-01) | ✅ | Qt 6.7.2: 3/6 · **Qt 6.8.3: 6/6** · `task3/tests/qml/graph/` |
| [x] | K-12 | Trang web Task 3 trong Chrome headless | ✅ | 17/17 · `test-artifacts/webengine-20261103/browser/` |
| [x] | K-13 | UI WebEngine trong app đầy đủ với Agent thật (UI-EQ-06 → 09) | ✅ | 06/11 với Agent Linux cục bộ: Online/Offline, tài nguyên, cảnh báo, lịch sử 52 bản ghi. Còn Agent Windows (W-10) |
| [x] | K-14 | Refresh Graph ≥ 10 lần và Subscriber restart trên app thật (Qt 6.8.3) | ✅ | 06/11 (E2E-27): Refresh ×10 hai host không treo; Reader biến mất/xuất hiện lại khi dừng/chạy Subscriber |
| [x] | K-15 | `ss -ltnp`: không có cổng HTTP | ✅ | 06/11: không có cổng TCP listen |
| [ ] | K-16 | HelloWorld Pub + Sub cùng trên Windows (trao đổi trong một máy) | ⬜ | Tùy chọn (REQ-03) |
| [ ] | K-17 | Hai Agent `windows-slave-a`, `windows-slave-b` trên một màn hình | ⬜ | Tùy chọn, ghi rõ là mô phỏng |
| [ ] | K-18 | Master mở trước Agent: Slave hiển thị "Chưa xác định" | ⬜ | Tùy chọn (REQ-13) |

---

## 5. Vấn đề kỹ thuật

| Mã | Vấn đề | TT | Ghi chú |
| :--- | :--- | :---: | :--- |
| P0-BACKEND | Backend Original crash khi Publisher stop/restart | ✅ | Chốt dùng **Patched** (0/7). Crash `DataReaderHistory::writer_not_alive` của Original chưa phân tích, không chặn |
| GRAPH-01 | `Loading Graph` không đóng | ✅ (test QML) | Nguyên nhân: `hostsList` rộng 0 làm ListView hủy delegate trên Qt 6.7.2. **Hết trên Qt 6.8.3** (6/6). Bản sửa 1 dòng `width: entity_box_width_` (`test-artifacts/original-20261102/graph01/graph01-hostslist-width.patch`) giữ làm phòng ngừa, không bắt buộc. Xác nhận trên GUI thật ở K-14 (chưa chạy) |
| GRAPH-02 | Writer/Reader không rõ trong viewport | ⬜ | Chưa xác minh trên Qt 6.8.3 |
| UI-WEB | UI WebEngine | ✅ | 06/11 đã chạy trong app đầy đủ với Agent Linux cục bộ; còn Agent Windows |
| NET-01 | Interface Windows trùng bộ đếm RX/TX (19, 25–29, 47–50) | 🟡 | Nguyên nhân: `GetIfTable2` có cả dòng lớp driver lọc (WFP, QoS, Npcap...) lặp bộ đếm card thật. Đã sửa trong `Collectors.cpp`; tên interface dùng `Alias`, trùng thì thêm ` (#index)`. Chờ kiểm trên Windows |

Chạy lại test GRAPH: `QT_QPA_PLATFORM=offscreen qmltestrunner -input task3/tests/qml/graph -import imports` (tại thư mục gốc repo).

Kết quả stress P0 (30 chu kỳ Publisher stop/restart, dwell 2 s, offscreen, GDB):

| Backend | Số lượt | Crash SIGSEGV | Treo khi thoát |
| :--- | :---: | :---: | :---: |
| Original | 9 | 5 (4 trong `DatabaseQueue`, 1 trong Fast DDS) | 2 |
| Patched | 7 | 0 | 0 |

---

## 6. Kiểm thử trên hệ thống thật (hướng phát triển)

Các nội dung dưới đây **ngoài phạm vi PoC 2 máy**; giữ làm kế hoạch nghiệm thu khi có thêm thiết bị và quyền thao tác. Mã E2E giữ nguyên để đối chiếu với kế hoạch 26 ca cũ.

| Nhóm | Nội dung | Mã E2E | Điều kiện cần |
| :--- | :--- | :--- | :--- |
| Quy mô | Nhiều Slave vật lý, **Slave Linux** (REQ-01, 02, NFR-01) | E2E-01, 02, 04, 05 | Thêm máy |
| Đồng bộ thời gian | Baseline NTP, Chrony → Windows Time, mất NTP → đồng bộ lại (FR-08, REQ-05, 06) | E2E-21 → 24 | Quyền quản trị NTP |
| Khởi động lại | Windows reboot, Master restart, lịch sử SQLite, > 500 sự kiện (FR-15) | E2E-19, 25, 26 | Quyền reboot |
| DDS hai máy | QoS Linux Pub ↔ Windows Sub, GUID mapping, Throughput/Latency định lượng (FR-10, 12, 13) | E2E-11 → 14 | Ứng dụng có metadata; phép đo độc lập |
| Cảnh báo | Debounce thật, DDS No Data, mất kết nối DDS, ngưỡng bất thường (REQ-07 → 09) | E2E-15 → 18 | Chốt ngưỡng cụ thể |
| Dịch vụ | Windows Service (SERVICE-01) | E2E-09 | Cài Service |
| Ổn định | Chạy liên tục thời gian nghiệm thu, đo độ trễ cập nhật (REQ-14, NFR-03) | — | Chốt thông số |
| Giao diện | Ba tab giữ lựa chọn trên UI WebEngine (E2E-20) | E2E-20 | Sau K-13 |
| Phiên bản | Ghi phiên bản Fast DDS / Fast CDR trên Windows (NFR-05, REQ-12) | E2E-01 | — |

Thông số cần chốt trước khi nghiệm thu thật: số Slave mục tiêu (code giới hạn 512), chu kỳ lấy mẫu (`interval_ms` 1000 ms), timeout Online → Offline (5000 ms), độ trễ UI chấp nhận, độ lệch thời gian cho phép, thời gian chạy liên tục, ngưỡng cảnh báo (`flow-rules.json`: 1500 / 15000 ms), thời hạn lưu SQLite (retention mặc định tắt).

---

## 7. Chuẩn bị môi trường

| Xong | Hạng mục | Đường dẫn / lệnh | Ghi chú |
| :---: | :--- | :--- | :--- |
| [x] | Backend Patched | `.task3-deps/fastdds-statistics-backend-2.5.0-patched/` | Chạy bằng `run_master_patched.sh` |
| [x] | Backend Original (để đối chiếu) | `/home/dev/dds-e2e/backend-original-install/` | `run_master_original.sh` |
| [x] | Windows Agent (Release) | `D:\Thang_1910\DDS-Monitor\build-agent-windows\Release\task3_agent.exe` | K-01 |
| [x] | HelloWorld Linux Pub / Windows Sub | `/home/dev/dds-tests/hello_world/build/hello_world` | Bật `FASTDDS_STATISTICS` |
| [x] | File QoS thử nghiệm | `/home/dev/dds-e2e/qos/` | `compatible.xml`, `incompatible-writer.xml` |
| [x] | Qt 6.8.3 có WebEngine, WebChannel | `~/Qt/6.8.3/gcc_64` | Cài 02/11 |
| [x] | **Qt5Compat** cho Qt 6.8.3 | Build từ `code.qt.io/qt/qt5compat` tag `v6.8.3` | MaintenanceTool đòi tài khoản gốc của bản cài (`phuongvuong12a1@gmail.com`); cần gói `libxkbcommon-dev` |
| [x] | Chrome/Edge để test trang web | máy bất kỳ | 03/11 dùng Chrome trên Windows |
| [ ] | Qt trong VMware (máy nhà) | — | Để build nhánh WebEngine ngoài giờ |

---

## 8. Tài liệu cần sửa

| Xong | Việc | Ghi chú |
| :---: | :--- | :--- |
| [x] | Báo cáo Chương 5, 6: bỏ HMAC, đổi sang Patched, thêm WebEngine, trình bày theo PoC | 06/11 |
| [x] | Báo cáo: kết quả chẩn đoán GRAPH-01 | 06/11 |
| [ ] | Báo cáo: dựng lại docx, xuất lại Hình 5.5, sửa Hình 5.4, thay Hình 5.9/5.10 | Mục 2.2 |
| [x] | `TASK3_README.md`: `run_master_original.sh`, WebEngine, PoC | 06/11 |
| [x] | `docs/task3/KNOWN_ISSUES.md`: trạng thái PoC, mục 1, P0, GRAPH-01, NET-01 | 06/11 |
| [x] | `THAY_DOI_SO_VOI_UPSTREAM.md`: CMake gốc, WebFeed đã compile | 06/11. `TASK3_REQUIREMENTS.md` giữ nguyên (đề bài) |

---

## 9. Nhật ký kiểm thử

| Ngày | Việc | Kết quả | Bằng chứng | Người chạy |
| :--- | :--- | :---: | :--- | :--- |
| 23/10/2026 | K-03, K-04, K-05 trên Windows thật | ✅ | Ảnh Host Online, lịch sử Active/Recovered | Thắng |
| 26/10/2026 | A/B Backend · Graph Refresh | 5/5 mỗi bản | Ảnh 20, 21 | Thắng |
| 26/10/2026 | Chẩn đoán GRAPH-01 | Rõ nguyên nhân | `graph01-data.*/monitor.log` | Thắng |
| 02/11/2026 | GRAPH-01 test tự động + bản sửa | Gốc 3/6 · sửa 6/6 | `test-artifacts/original-20261102/graph01/` | Claude |
| 02/11/2026 | K-07 QoS cục bộ Linux | ✅ 3/3 | `.../qos-local/result.json` | Claude |
| 02/11/2026 | K-09 stress Original vs Patched | Original crash 5/9 · Patched 0/7 | `original-20261102*/`, `patched-20261102/` | Claude |
| 02/11/2026 | Chốt Backend Patched; cài Qt 6.8.3 | Quyết định | — | Thắng |
| 02/11/2026 | UI Task 3 sang WebEngine (`feature/webengine-ui`) | 2,4–4,4 % pixel khác · tương tác PASS · CTest PASS | `test-artifacts/webengine-20261102/` | Claude |
| 02/11/2026 | Build Qt 6.8.3 | Build, CTest, Graph QML 6/6 ✅ · Master ❌ thiếu Qt5Compat | `test-artifacts/qt683-20261102/` | Claude |
| 03/11/2026 | K-12 trang web (Chrome headless) | 17/17 · 512 Slave 41 ms · payload 133 → 19 KB · C++ chưa compile | `test-artifacts/webengine-20261103/browser/` | Claude |
| 06/11/2026 | Chuyển cách trình bày sang PoC; cập nhật Chương 5, 6 báo cáo | md đã sửa, docx chưa dựng lại | `A_TONGKET/BaoCao/` | Claude |
| 06/11/2026 | Cài Qt5Compat (build từ mã nguồn), sửa CMake WebEngine, build Debug + Release | ✅ CTest PASS · `task3_tests` 48/48 | `.task3-deps/task3-builds/20261106T020424Z-27772/` | Claude |
| 06/11/2026 | K-13, K-15: chạy Master thật với UI WebEngine + Agent Linux cục bộ | ✅ Online · Fresh, Offline · Stale, cảnh báo, 52 bản ghi lịch sử; không có cổng TCP | Ảnh chụp màn hình | Thắng, Claude |
| 06/11/2026 | NET-01: bỏ `FilterInterface`, đặt tên interface bằng `Alias`; thống nhất Qt 6.8.3 | ✅ Linux: `task3_tests` 49/49 · ⬜ Windows chưa kiểm | `task3/Collectors.cpp`, `task3/tests/CoreTests.cpp` | Codex, Claude |
| 06/11/2026 | E2E hai máy (Linux Master, Windows Slave): 29/29 ca PASS, gồm E2E-26 (reboot Windows) | ✅ PASS (E2E-09 dịch vụ Spooler đã kiểm) | `~/DDS-Monitor-Evidence/20261106-linux/02-acceptance/`, bảng 7.1 của `MASTER_LINUX_WINDOWS_E2E_TEST.md` | Thắng, Claude |
| | | | | |
