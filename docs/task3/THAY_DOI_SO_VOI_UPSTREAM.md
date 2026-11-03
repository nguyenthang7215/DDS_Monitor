# 🔍 THAY ĐỔI SO VỚI CODE GỐC eProsima DDS Monitor

| Thông tin | Giá trị |
| :--- | :--- |
| Code gốc (upstream) | https://github.com/eProsima/DDS-Monitor · nhánh `main` · commit `94bbd6d` (*Document event charts and plot mode dropdown #318*) |
| Code hiện tại | https://github.com/nguyenthang7215/DDS_Monitor · nhánh `feature/webengine-ui` · tính đến commit `3bb9990` + cập nhật tài liệu 03/11 |
| Ngày đối chiếu | 03/11/2026 |
| Upstream có commit mới từ lúc tích hợp? | **Không**: `origin/main` vẫn là `94bbd6d` |
| Tạo lại file này | `git diff 94bbd6d HEAD` (xem mục 6) |
| Nhánh | `feature/webengine-ui` (UI Task 3 chạy trên Qt WebEngine; **đã kiểm với Qt 6.8.3**; `-DTASK3_WEB_UI=OFF` build bản QML khi Qt không có WebEngine) |
| Lần cập nhật | 02/11/2026 lần 1 (`22e3109`) · lần 2 (`a19a240`): test GRAPH-01, QoS, stress P0 · lần 3 (`748c8d7`): **UI Task 3 chuyển sang Qt WebEngine** · lần 4 (03/11, `3bb9990`): `WebFeed`, CSP, chọn Qt tự động, test trang web trên trình duyệt |

---

## 1. Tóm tắt

| Nhóm | Số file | Ý nghĩa |
| :--- | :---: | :--- |
| ✏️ **File gốc bị sửa** | **11** (không đổi 03/11) | +232 / −14 dòng (trong đó `.gitignore` +41), chủ yếu ở điểm tích hợp. Chi tiết ở mục 2 và 3 |
| ➕ File mới: mã nguồn Task 3 (`task3/`) | 71 | Agent, Master, UI QML + Web, test, script, config |
| ➕ File mới: tài liệu (`docs/task3/`, `TASK3_README.md`) | 14 | Requirements, kiến trúc, hướng dẫn chạy/E2E, tiến độ |
| ➕ File mới: bằng chứng test (`test-artifacts/`) | 149 | Log, JSON, ảnh, patch P0 |
| ➕ File mới: checkpoint di chuyển (`.task3-migration-checkpoints/`) | 23 | Bản lưu tài liệu trước khi tích hợp vào repo chính |
| **Tổng** | **268** | `268 files changed, 40143 insertions(+), 14 deletions(-)` |

> **Lưu ý GRAPH-01:** bản sửa 1 dòng cho `qml/DomainGraphLayout.qml` **chưa áp** vào code; chỉ nằm ở `test-artifacts/original-20261102/graph01/graph01-hostslist-width.patch`. Khi áp, file này sẽ thành file gốc bị sửa thứ 12.

> **Nguyên tắc đã giữ:** không sửa các file lõi của Domain Graph (`DomainGraphLayout.qml`, `TabLayout.qml`, `GraphConnection.qml`), `Controller.cpp`, và không sửa source thư viện eProsima (Fast DDS, Statistics Backend). Bản vá Backend chỉ nằm trong `test-artifacts/` dưới dạng file `.patch`, không được áp vào source.

---

## 1b. Giao diện Task 3 trên Qt WebEngine (nhánh `feature/webengine-ui`)

| Phần | File | Ghi chú |
| :--- | :--- | :--- |
| Trang web (mới) | `task3/web/index.html`, `style.css`, `app.js` | HTML/CSS/JS thuần, không thư viện ngoài, nạp từ `qrc:/task3/web/` |
| Vỏ QML (mới) | `task3/WebDashboard.qml` | `WebEngineView` + `WebChannel`; giữ API của `Dashboard.qml` (`selectedHostId`, `chooseHost()`, `openDdsWorkspace`…) |
| Khởi tạo | `task3/Dashboard.cpp/.h` | `prepareWebEngine()` (bỏ qua `QTWEBENGINE_REMOTE_DEBUGGING` trừ khi `TASK3_WEB_DEVTOOLS=1`); tạo `task3WebFeed`; biến `TASK3_UI` chọn trang web/QML; `#ifdef TASK3_WEB_UI` |
| Dữ liệu cho trang (mới 03/11) | `task3/WebFeed.h/.cpp` | Lớp trung gian giữa `MonitoringCore` và `QWebChannel`: mỗi thuộc tính một signal, chỉ phát khi đổi, gộp cập nhật 100 ms, Participant chỉ giữ 6 trường. Payload mỗi cập nhật 133 KB → 19 KB. ⚠️ Chưa compile |
| Build | `task3/CMakeLists.txt` | Tùy chọn `TASK3_WEB_UI` (ON): link `Qt6::WebEngineQuick`, `Qt6::WebChannelQuick`, thiếu module → báo lỗi rõ; OFF → chỉ trang QML. `WebFeed.cpp` trong `task3_core` |
| An toàn / CON-04 | `task3/web/index.html` | Content-Security-Policy: `default-src 'none'`, `connect-src 'none'`, chỉ nạp `qrc:`; không `eval` |
| Script | `task3/scripts/qt_prefix.sh` (mới), `build*.sh`, `run_master*.sh` | Tự chọn Qt có WebEngine + Qt5Compat; kiểm module thiếu trước khi build/chạy; build ghi `qt-prefix`, `web-ui` |
| Test (mới) | `task3/tests/DashboardPreview.cpp` | Dựng trang QML hoặc Web từ snapshot để so sánh ảnh, bấm thử |
| Test trình duyệt (mới 03/11) | `task3/tests/web/` (`run_web_tests.py`, `mock_channel.js`, `selftest.js`, `perf.js`) | Chạy `task3/web` thật trong Chrome/Edge headless, không cần Qt: 17/17 PASS |
| Unit test (mới 03/11) | `task3/tests/CoreTests.cpp` → `webFeedPublishesTrimmedChangesOnly` | ⚠️ Chưa compile |
| Code gốc bị đụng thêm | `src/main.cpp` (+1 dòng), `qml/main.qml` (1 dòng `source`) | Không đổi màn DDS chuyên sâu |
| Bản QML cũ | `task3/Dashboard.qml`, `HostWorkspace.qml`, `FlowDetails.qml`, `EventHistoryView.qml` | Giữ lại để so sánh (`TASK3_UI=qml`) |

## 2. File gốc bị sửa: giải thích

| # | File | +/− | Thay đổi | Lý do | Rủi ro / ghi chú |
| :---: | :--- | :---: | :--- | :--- | :--- |
| 1 | `src/main.cpp` | +5 | Gọi `task3::prepareWebEngine()` (trước `QApplication`), `task3::installShutdownHandler()` và `task3::installDashboard(engine)` | **Điểm cắm** module Task 3; Qt WebEngine phải khởi tạo trước `QApplication` | Thấp |
| 2 | `CMakeLists.txt` | +6 | `add_subdirectory(task3)` và tùy chọn `TASK3_BUILD_TESTS` | Build module Task 3 cùng DDS Monitor | Thấp |
| 3 | `qml/main.qml` | +132/−9 | Thêm thanh chọn 3 workspace (*Giám sát hệ thống · DDS chuyên sâu · Cảnh báo & lịch sử*), `Loader` nạp trang Task 3 (`WebDashboard.qml` mặc định, `Dashboard.qml` khi `TASK3_UI=qml`), ẩn/hiện màn gốc theo tab, hàm `task3Exercise()` phục vụ test UI tự động | Gộp UI Task 3 và UI gốc vào **một cửa sổ** | Trung bình: đây là file UI chính. Màn DDS gốc vẫn giữ nguyên, chỉ bọc thêm |
| 4 | `src/backend/SyncBackendConnection.cpp` | ±3 | `DDS_MONITOR_APP` → `FASTDDS_MONITOR_APP` (3 chỗ) | Header Statistics Backend 2.5.0 chỉ có tên `FASTDDS_MONITOR_APP`, không đổi thì **không build được** | Thấp: chỉ đổi tên hằng |
| 5 | `src/Engine.cpp` | +7 | Với chart kiểu `NONE` (raw), đọc lại cả cửa sổ hiển thị thay vì chỉ đọc từ lần trước | Sửa lỗi Dynamic Chart **bỏ sót mẫu đến muộn** | Trung bình: nhánh aggregate/cumulative chưa sửa |
| 6 | `src/statistics/dynamic/DynamicDataChartBox.cpp` | +15/−1 | Series raw dùng `handleSnapshot()` (thay cả dữ liệu) thay vì nối thêm điểm | Đi kèm #5, tránh điểm trùng | Trung bình |
| 7 | `include/.../dynamic/DynamicDataChartBox.h` | +1 | Thêm trường `window_size` vào `UpdateParameters` | Đi kèm #5 | Thấp |
| 8 | `src/statistics/DataModel.cpp` | +13 | Hàm `replaceData()`: lọc NaN, sắp xếp theo thời gian, giới hạn số điểm | Đi kèm #6 | Thấp |
| 9 | `include/.../statistics/DataModel.h` | +7 | Signal `snapshotReady`, hàm `handleSnapshot`, `replaceData` | Đi kèm #8 | Thấp |
| 10 | `qml/DynamicStatisticsChartView.qml` | +2/−1 | `delay_time`: 5000 → **30000 ms** | Thử nghiệm PoC cho chart hiển thị ổn định | ⚠️ **Thử nghiệm**, chưa có bằng chứng là giải pháp đúng |
| 11 | `.gitignore` | +41 | Bỏ qua `.task3-deps/`, `.task3-runtime/`, build, database | Không commit file sinh ra | Không ảnh hưởng chạy |

### Phân loại

| Loại | File |
| :--- | :--- |
| 🔌 Điểm tích hợp (bắt buộc) | `main.cpp`, `CMakeLists.txt`, `main.qml` |
| 🛠️ Sửa để build được | `SyncBackendConnection.cpp` |
| 🐞 Sửa lỗi chart gốc | `Engine.cpp`, `DynamicDataChartBox.*`, `DataModel.*` |
| 🧪 Thử nghiệm, cần xem lại | `DynamicStatisticsChartView.qml` (`delay_time`) |
| 📁 Cấu hình repo | `.gitignore` |

### File gốc **không** bị sửa (đã kiểm tra)

`qml/DomainGraphLayout.qml` · `qml/TabLayout.qml` · `qml/GraphConnection.qml` · `src/Controller.cpp`. Vì vậy lỗi **GRAPH-01 (Loading Graph)** nằm trong code gốc của eProsima, không phải do phần mở rộng gây ra.

---

## 3. Diff đầy đủ các file gốc bị sửa

> Trích nguyên văn từ `git diff 94bbd6d HEAD`. Dòng `+` là thêm, dòng `-` là xóa.

### `.gitignore`

<details><summary>Xem diff (+41 / −0)</summary>

```diff
diff --git a/.gitignore b/.gitignore
index b4dd883..d6e8c90 100644
--- a/.gitignore
+++ b/.gitignore
@@ -2,6 +2,7 @@
 ### C++ ###
 # Prerequisites
 *.d
+__pycache__/
 
 # Compiled Object files
 *.slo
@@ -61,6 +62,7 @@ qrc_*.cpp
 ui_*.h
 Makefile*
 *build-*
+!task3/scripts/build-windows.ps1
 
 # QtCreator
 
@@ -99,6 +101,45 @@ tags
 build/
 install/
 log/
+/.task3-deps/
+/.task3-runtime/
+
+# SQLite databases
+*.db
+*.sqlite
+*.sqlite3
+
+# Logs, PIDs and runtime artifacts
+*.log
+*.pid
+
+# Python bytecode
+*.py[cod]
+*$py.class
+
+# Test execution outputs and runtime state
+test-artifacts/ui-*/
+test-artifacts/**/state/
+test-artifacts/**/identities/
+test-artifacts/**/*.png
+test-artifacts/**/snapshot.json
+test-artifacts/**/flows.json
+test-artifacts/**/resource-usage.json
+test-artifacts/**/metrics_real.json
+test-artifacts/**/telemetry_real.json
+test-artifacts/**/process_stopped.json
+test-artifacts/**/stopped_run_unmapped.json
+test-artifacts/**/topology_application_routes.json
+test-artifacts/**/fresh_connection_list_matching.json
+test-artifacts/**/runtime_counter_recovery.json
+test-artifacts/**/restart_guid_remapping.json
+test-artifacts/**/runtime_mapping_multiple_participants.json
+test-artifacts/**/single_window_navigation_resize.json
+test-artifacts/**/agent*.json
+test-artifacts/**/alive_writer_no_data.json
+test-artifacts/**/dds_*.json
+test-artifacts/**/master_restart_live_agent.json
+test-artifacts/**/subscriber_*.json
 
 ### Visual Studio ###
 .vs
```

</details>

### `CMakeLists.txt`

<details><summary>Xem diff (+6 / −0)</summary>

```diff
diff --git a/CMakeLists.txt b/CMakeLists.txt
index 6672def..e61d8a4 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -346,6 +346,12 @@ else()
 endif()
 
 # Install config.h header
+option(TASK3_BUILD_TESTS "Build Task 3 unit tests and owned DDS fixture" ON)
+if(TASK3_BUILD_TESTS)
+    enable_testing()
+endif()
+add_subdirectory(task3)
+
 install(FILES ${PROJECT_BINARY_DIR}/include/${PROJECT_NAME}/config.h
 DESTINATION ${INCLUDE_INSTALL_DIR}/${PROJECT_NAME}
 COMPONENT headers
```

</details>

### `include/dds_monitor/statistics/DataModel.h`

<details><summary>Xem diff (+7 / −0)</summary>

```diff
diff --git a/include/dds_monitor/statistics/DataModel.h b/include/dds_monitor/statistics/DataModel.h
index 7a2416a..18164ea 100644
--- a/include/dds_monitor/statistics/DataModel.h
+++ b/include/dds_monitor/statistics/DataModel.h
@@ -45,6 +45,8 @@ public:
     {
         QObject::connect(this, &DataModel::newPointAdded, this, &DataModel::addNewPoint,
                 Qt::QueuedConnection);
+        QObject::connect(this, &DataModel::snapshotReady, this, &DataModel::replaceData,
+                Qt::QueuedConnection);
     }
 
     DataModel(
@@ -55,6 +57,8 @@ public:
     {
         QObject::connect(this, &DataModel::newPointAdded, this, &DataModel::addNewPoint,
                 Qt::QueuedConnection);
+        QObject::connect(this, &DataModel::snapshotReady, this, &DataModel::replaceData,
+                Qt::QueuedConnection);
     }
 
     //! Number of rows
@@ -76,6 +80,7 @@ public:
     //! Function to add a new point by sending signal \c newPointAdded
     void handleNewPoint(
             const QPointF& point);
+    void handleSnapshot(const QVector<QPointF>& points) { emit snapshotReady(points); }
 
     const QVector<QPointF>& get_data() const;
 
@@ -90,12 +95,14 @@ public:
             quint64 max_points);
 
 signals:
+    void snapshotReady(QVector<QPointF> points);
 
     //! Signal to notify a new point must be added
     void newPointAdded(
             const QPointF& point);
 
 protected:
+    void replaceData(QVector<QPointF> points);
 
     //! Function to add a new point to the model
     void addNewPoint(
```

</details>

### `include/dds_monitor/statistics/dynamic/DynamicDataChartBox.h`

<details><summary>Xem diff (+1 / −0)</summary>

```diff
diff --git a/include/dds_monitor/statistics/dynamic/DynamicDataChartBox.h b/include/dds_monitor/statistics/dynamic/DynamicDataChartBox.h
index 5397b52..1666bf7 100644
--- a/include/dds_monitor/statistics/dynamic/DynamicDataChartBox.h
+++ b/include/dds_monitor/statistics/dynamic/DynamicDataChartBox.h
@@ -45,6 +45,7 @@ struct UpdateParameters
      */
     std::vector<bool> cumulative;
     std::vector<quint64> cumulative_interval;
+    quint64 window_size = 0;
 };
 
 /**
```

</details>

### `qml/DynamicStatisticsChartView.qml`

<details><summary>Xem diff (+2 / −1)</summary>

```diff
diff --git a/qml/DynamicStatisticsChartView.qml b/qml/DynamicStatisticsChartView.qml
index c5a70a2..a9c4afd 100644
--- a/qml/DynamicStatisticsChartView.qml
+++ b/qml/DynamicStatisticsChartView.qml
@@ -29,7 +29,8 @@ StatisticsChartView {
     // TODO if the backend changes and uses max time_frame as time in any point, this delay could be applied
     // to the time_to get_data call and not to the axis.
     // Meanwhile doing that would leave the chart too shifted to the left
-    property int delay_time: 5000
+    // Experimental PoC value; revisit when optimizing the Dynamic Chart.
+    property int delay_time: 30000
     property int axis_refresh_time: 100
 
     property bool running: false
```

</details>

### `qml/main.qml`

<details><summary>Xem diff (+132 / −9)</summary>

```diff
diff --git a/qml/main.qml b/qml/main.qml
index 9dd88b4..a541ccf 100644
--- a/qml/main.qml
+++ b/qml/main.qml
@@ -25,6 +25,7 @@ import Theme 1.0
 
 ApplicationWindow {
     id: mainApplicationView
+    objectName: "ddsMonitorWindow"
     visible: true
     width: 1600
     height: 900
@@ -34,23 +35,64 @@ ApplicationWindow {
 
     // This property holds the current number of initialized monitors
     property int monitors: 0
+    property bool task3Ready: false
+    property int workspaceIndex: 0
+    readonly property bool task3Page: workspaceIndex !== 1
+    property bool legacyStarted: false
+    property int task3UiSteps: 0
+    property bool task3ResizeExercise: true
+    readonly property bool task3DashboardLoaded: task3Loader.status === Loader.Ready
+    readonly property string task3SelectedHost: task3Loader.item ? task3Loader.item.selectedHostId : ""
+    readonly property string task3SelectedApplication: task3Loader.item ? task3Loader.item.selectedApplicationId : ""
+    readonly property int task3DetailTab: task3Loader.item ? task3Loader.item.detailTab : 0
+    readonly property int task3ResourceTab: task3Loader.item ? task3Loader.item.resourceTab : 0
+    property string task3ExpectedHost: ""
+    property string task3ExpectedApplication: ""
+    property bool task3SelectionRetained: true
+    function task3Exercise(step) {
+        task3UiSteps = step
+        const phase = step % 8
+        workspaceIndex = phase === 4 ? 1 : phase === 5 ? 2 : 0
+        const page = task3Loader.item
+        if (page) {
+            if (phase === 0 && page.hosts.length) page.chooseHost(page.hosts[0].host_id)
+            if (phase === 1 && page.selectedHost.applications?.length) {
+                page.chooseApplication(page.selectedHostId, page.selectedHost.applications[0].application_id)
+                task3ExpectedHost = page.selectedHostId
+                task3ExpectedApplication = page.selectedApplicationId
+            }
+            if (phase <= 3) page.detailTab = phase
+            if (phase === 0) page.resourceTab = 0
+            if (phase === 7) { page.detailTab = 0; page.resourceTab = 1 }
+            if (phase === 2) {
+                const flow = task3Core.flows.find(f => f.source_host === page.selectedHostId &&
+                    (!page.selectedApplicationId || f.source_application === page.selectedApplicationId))
+                if (flow) page.selectedFlowKey = page.flowKey(flow)
+            }
+            if (phase === 6 && task3ExpectedHost)
+                task3SelectionRetained = task3SelectionRetained && page.selectedHostId === task3ExpectedHost &&
+                    page.selectedApplicationId === task3ExpectedApplication
+        }
+        if (task3ResizeExercise) {
+            width = step % 2 === 0 ? 1280 : 900
+            height = step % 2 === 0 ? 820 : 650
+        }
+    }
 
     onStartMonitoring: {
-        mainStartView.visible = false
-        monitorMenuBar.visible = true
+        legacyStarted = true
         toolBar.isVisible = true
-        panels.visible = true
         dialogInitMonitor.open()
     }
 
     MainStartView {
         id: mainStartView
-        visible: true
+        visible: !mainApplicationView.task3Page && !mainApplicationView.legacyStarted
     }
 
     menuBar: MonitorMenuBar {
         id: monitorMenuBar
-        visible: false
+        visible: !mainApplicationView.task3Page && mainApplicationView.legacyStarted
         onToolBarHidden: toolBar.isVisible = !toolBar.isVisible
         onDispDataButtonHidden: toolBar.isVisibleDispData = !toolBar.isVisibleDispData
         onDispDynDataButtonHidden: toolBar.isVisibleDispDynData = !toolBar.isVisibleDispDynData
@@ -73,14 +115,72 @@ ApplicationWindow {
         onExplorerEntityInfoChanged: function(status) { panels.changeExplorerEntityInfo(status) }
     }
 
-    header: MonitorToolBar {
-        id: toolBar
-        onChangeChartboxLayout: function(chartsPerRow) { monitorMenuBar.changeChartboxLayoutViewMenu(chartsPerRow) }
+    header: ColumnLayout {
+        spacing: 0
+        RowLayout {
+            Layout.fillWidth: true
+            Layout.preferredHeight: 52
+            spacing: 16
+            Label { text: "DDS Monitor"; font.pixelSize: 23; font.bold: true; color: Theme.eProsimaDarkBlue; leftPadding: 20 }
+            Label {
+                objectName: "monitorStatusLabel"
+                Layout.fillWidth: true
+                // Keep the technical topic in diagnostics, not in the product heading.
+                text: {
+                    if (!mainApplicationView.task3Ready) return "Đang khởi tạo giám sát…"
+                    const status = task3Core.status
+                    if (!status.startsWith("DDS telemetry domain")) return status
+                    const diagnostics = status.indexOf(" | ")
+                    return "Giám sát tài nguyên & truyền thông DDS" + (diagnostics < 0 ? "" : status.slice(diagnostics))
+                }
+                color: Theme.midGrey
+                elide: Text.ElideRight
+                font.pixelSize: 13
+                ToolTip.visible: statusHover.hovered
+                ToolTip.text: text
+                HoverHandler { id: statusHover }
+            }
+            Label { text: "Master"; color: Theme.eProsimaDarkBlue; rightPadding: 20 }
+        }
+        TabBar {
+            id: primaryTabs
+            font.pixelSize: 15
+            implicitHeight: 42
+            Layout.fillWidth: true
+            currentIndex: mainApplicationView.workspaceIndex
+            onCurrentIndexChanged: mainApplicationView.workspaceIndex = currentIndex
+            Connections {
+                target: mainApplicationView
+                function onWorkspaceIndexChanged() { primaryTabs.currentIndex = mainApplicationView.workspaceIndex }
+            }
+            TabButton {
+                id: systemTab
+                text: "Giám sát hệ thống"
+                contentItem: Label { text: systemTab.text; horizontalAlignment: Text.AlignHCenter; color: systemTab.checked ? Theme.eProsimaDarkBlue : Theme.midGrey; font.bold: systemTab.checked }
+            }
+            TabButton {
+                id: ddsTab
+                text: "DDS chuyên sâu"
+                contentItem: Label { text: ddsTab.text; horizontalAlignment: Text.AlignHCenter; color: ddsTab.checked ? Theme.eProsimaDarkBlue : Theme.midGrey; font.bold: ddsTab.checked }
+            }
+            TabButton {
+                id: historyTab
+                text: "Cảnh báo & lịch sử"
+                contentItem: Label { text: historyTab.text; horizontalAlignment: Text.AlignHCenter; color: historyTab.checked ? Theme.eProsimaDarkBlue : Theme.midGrey; font.bold: historyTab.checked }
+            }
+        }
+        MonitorToolBar {
+            id: toolBar
+            Layout.fillWidth: true
+            visible: mainApplicationView.workspaceIndex === 1 && isVisible
+            onChangeChartboxLayout: function(chartsPerRow) { monitorMenuBar.changeChartboxLayoutViewMenu(chartsPerRow) }
+        }
     }
 
     Panels {
         id: panels
-        visible: false
+        objectName: "ddsOriginalWorkspace"
+        visible: !mainApplicationView.task3Page && mainApplicationView.legacyStarted
         onExplorerDDSEntitiesChanged: function(status) { monitorMenuBar.changeExplorerDDSEntities(status) }
         onExplorerPhysicalChanged: function(status) { monitorMenuBar.changeExplorerPhysical(status) }
         onExplorerLogicalChanged: function(status) { monitorMenuBar.changeExplorerLogical(status) }
@@ -91,6 +191,27 @@ ApplicationWindow {
         id: dialogInitMonitor
     }
 
+    Loader {
+        id: task3Loader
+        anchors.fill: parent
+        active: mainApplicationView.task3Ready
+        visible: mainApplicationView.task3Page
+        source: mainApplicationView.task3Ready ? task3DashboardSource : ""
+    }
+    Binding {
+        target: task3Loader.item
+        property: "eventsPage"
+        value: mainApplicationView.workspaceIndex === 2
+        when: task3Loader.item !== null
+    }
+    Connections {
+        target: task3Loader.item
+        function onOpenDdsWorkspace(entityId) {
+            mainApplicationView.workspaceIndex = 1
+            if (entityId) controller.endpoint_click(entityId)
+        }
+    }
+
     InitDSMonitorDialog {
         id: dialogDSInitMonitor
     }
@@ -174,6 +295,8 @@ ApplicationWindow {
         target: controller
         function onMonitorInitialized() {
             monitors++
+            mainApplicationView.legacyStarted = true
+            toolBar.isVisible = true
         }
     }
 }
```

</details>

### `src/Engine.cpp`

<details><summary>Xem diff (+7 / −0)</summary>

```diff
diff --git a/src/Engine.cpp b/src/Engine.cpp
index ce72171..a3919b9 100644
--- a/src/Engine.cpp
+++ b/src/Engine.cpp
@@ -2032,6 +2032,13 @@ void Engine::update_dynamic_chartbox(
 
         eprosima::statistics_backend::Timestamp time_from_ =
                 backend::Timestamp(std::chrono::milliseconds(parameters.time_from));
+        if (statistics_kind_ == backend::StatisticKind::NONE)
+        {
+            // Re-read the visible raw window: a sample can arrive after its timestamp was polled.
+            // DynamicDataChartBox replaces this snapshot instead of appending overlapping results.
+            const auto from = time_to > parameters.window_size ? time_to - parameters.window_size : 0;
+            time_from_ = backend::Timestamp(std::chrono::milliseconds(from));
+        }
 
         // If statistics_kind is NONE, then the number of bins is 0 to retrieve all the data available
         // Otherwise the bins is 1 so only one data is updated
```

</details>

### `src/backend/SyncBackendConnection.cpp`

<details><summary>Xem diff (+3 / −3)</summary>

```diff
diff --git a/src/backend/SyncBackendConnection.cpp b/src/backend/SyncBackendConnection.cpp
index 8092fc8..c0ec648 100644
--- a/src/backend/SyncBackendConnection.cpp
+++ b/src/backend/SyncBackendConnection.cpp
@@ -536,7 +536,7 @@ EntityId SyncBackendConnection::init_monitor(
     try
     {
         return StatisticsBackend::init_monitor(domain, nullptr, CallbackMask::all(),
-                       DataKindMask::none(), DDS_MONITOR_APP, "", easy_mode_ip);
+                       DataKindMask::none(), FASTDDS_MONITOR_APP, "", easy_mode_ip);
     }
     catch (const Error& e)
     {
@@ -557,7 +557,7 @@ EntityId SyncBackendConnection::init_monitor(
     {
         return StatisticsBackend::init_monitor(
             discovery_server_locators,
-            nullptr, CallbackMask::all(), DataKindMask::none(), DDS_MONITOR_APP);
+            nullptr, CallbackMask::all(), DataKindMask::none(), FASTDDS_MONITOR_APP);
     }
     catch (const Error& e)
     {
@@ -578,7 +578,7 @@ EntityId SyncBackendConnection::init_monitor_with_profile(
     {
         return StatisticsBackend::init_monitor_with_profile(
             profile_name,
-            nullptr, CallbackMask::all(), DataKindMask::none(), DDS_MONITOR_APP);
+            nullptr, CallbackMask::all(), DataKindMask::none(), FASTDDS_MONITOR_APP);
     }
     catch (const Error& e)
     {
```

</details>

### `src/main.cpp`

<details><summary>Xem diff (+5 / −0)</summary>

```diff
diff --git a/src/main.cpp b/src/main.cpp
index a90aeef..e5f03f7 100644
--- a/src/main.cpp
+++ b/src/main.cpp
@@ -25,13 +25,17 @@
 #include <QQuickStyle>
 
 #include <dds_monitor/Engine.h>
+#include "../task3/Dashboard.h"
+#include "../task3/Shutdown.h"
 #include <dds_monitor/io/ClipboardHandler.h>
 
 int main(
         int argc,
         char* argv[])
 {
+    task3::prepareWebEngine();
     QApplication app(argc, argv);
+    task3::installShutdownHandler();
     app.setWindowIcon(QIcon(":/resources/images/eprosima_logo.ico"));
     app.setOrganizationName("eProsima");
     app.setOrganizationDomain("eprosima.com");
@@ -54,6 +58,7 @@ int main(
         return -1;
     }
     window->show();
+    task3::installDashboard(engine);
 
     return app.exec();
 }
```

</details>

### `src/statistics/DataModel.cpp`

<details><summary>Xem diff (+13 / −0)</summary>

```diff
diff --git a/src/statistics/DataModel.cpp b/src/statistics/DataModel.cpp
index 66668d1..310c0a0 100644
--- a/src/statistics/DataModel.cpp
+++ b/src/statistics/DataModel.cpp
@@ -17,6 +17,8 @@
 
 #include <iostream>
 #include <QDebug>
+#include <algorithm>
+#include <cmath>
 
 #include <dds_monitor/statistics/DataModel.h>
 
@@ -93,6 +95,17 @@ void DataModel::addNewPoint(
     }
 }
 
+void DataModel::replaceData(QVector<QPointF> points)
+{
+    points.erase(std::remove_if(points.begin(), points.end(), [](const QPointF& p) {
+        return !std::isfinite(p.x()) || !std::isfinite(p.y());
+    }), points.end());
+    std::stable_sort(points.begin(), points.end(), [](const QPointF& a, const QPointF& b) { return a.x() < b.x(); });
+    const auto limit = max_points_ ? max_points_ : 10000;
+    if (quint64(points.size()) > limit) points.remove(0, points.size() - qsizetype(limit));
+    beginResetModel(); m_data_ = std::move(points); endResetModel();
+}
+
 const QVector<QPointF>& DataModel::get_data() const
 {
     return m_data_;
```

</details>

### `src/statistics/dynamic/DynamicDataChartBox.cpp`

<details><summary>Xem diff (+15 / −1)</summary>

```diff
diff --git a/src/statistics/dynamic/DynamicDataChartBox.cpp b/src/statistics/dynamic/DynamicDataChartBox.cpp
index c60fc3c..aafa131 100644
--- a/src/statistics/dynamic/DynamicDataChartBox.cpp
+++ b/src/statistics/dynamic/DynamicDataChartBox.cpp
@@ -77,7 +77,20 @@ void DynamicDataChartBox::update(
     // Update internal timer so next call to get data use it as from
     time_to_ = time_to;
 
-    DataChartBox::update(new_data);
+    auto incremental = new_data;
+    for (size_t i = 0; i < current_update_parameters_.series_ids.size(); ++i)
+    {
+        if (current_update_parameters_.statistics_kinds[i] != "NONE") continue;
+        const auto id = current_update_parameters_.series_ids[i];
+        auto found = series_.find(id);
+        if (found != series_.end())
+        {
+            found->second->handleSnapshot(new_data[id]);
+            for (const auto& point : new_data[id]) newYValue(point.y());
+        }
+        incremental.erase(id);
+    }
+    DataChartBox::update(incremental);
 }
 
 UpdateParameters DynamicDataChartBox::get_update_parameters()
@@ -85,6 +98,7 @@ UpdateParameters DynamicDataChartBox::get_update_parameters()
     const std::lock_guard<std::recursive_mutex> lock(mutex_);
 
     current_update_parameters_.time_from = time_to_;
+    current_update_parameters_.window_size = window_size_;
 
     return current_update_parameters_;
 }
```

</details>

---

## 4. Mã nguồn mới: `task3/`

| File | Dòng | Vai trò |
| :--- | ---: | :--- |
| `task3/AlertStore.cpp` | 434 | SQLite Active/Recovered |
| `task3/AlertStore.h` | 52 | SQLite Active/Recovered |
| `task3/CMakeLists.txt` | 67 | Build |
| `task3/Collectors.cpp` | 521 | Thu CPU/RAM/Disk/Network/Process/Service/Time (Linux + Windows) |
| `task3/Collectors.h` | 38 | Thu CPU/RAM/Disk/Network/Process/Service/Time (Linux + Windows) |
| `task3/Dashboard.cpp` | 205 | Đăng ký model/UI Task 3 |
| `task3/Dashboard.h` | 9 | Đăng ký model/UI Task 3 |
| `task3/Dashboard.qml` | 462 | Giao diện QML Task 3 (trang chính) |
| `task3/DdsBridge.cpp` | 232 | Đọc Statistics Backend |
| `task3/DdsBridge.h` | 23 | Đọc Statistics Backend |
| `task3/DdsTransport.cpp` | 239 | DataWriter/DataReader telemetry Domain 42 |
| `task3/DdsTransport.h` | 30 | DataWriter/DataReader telemetry Domain 42 |
| `task3/EventHistoryView.qml` | 208 | Giao diện QML Task 3 |
| `task3/FlowDetails.qml` | 242 | Giao diện QML Task 3 |
| `task3/FlowModel.cpp` | 60 | Topology Host → App → Writer → Topic → Reader |
| `task3/FlowModel.h` | 7 | Topology Host → App → Writer → Topic → Reader |
| `task3/HostWorkspace.qml` | 727 | Giao diện QML Task 3 |
| `task3/MonitoringCore.cpp` | 404 | Đánh giá trạng thái, cảnh báo, luồng DDS |
| `task3/MonitoringCore.h` | 73 | Đánh giá trạng thái, cảnh báo, luồng DDS |
| `task3/Protocol.cpp` | 135 | Kiểm tra schema snapshot |
| `task3/Protocol.h` | 11 | Kiểm tra schema snapshot |
| `task3/Registry.cpp` | 123 | Định danh Host/Session, chống trùng sequence, ánh xạ GUID |
| `task3/Registry.h` | 36 | Định danh Host/Session, chống trùng sequence, ánh xạ GUID |
| `task3/RollingSeries.h` | 36 | Đơn vị, chuỗi thời gian |
| `task3/RuntimeIdentity.cpp` | 70 | Manifest ánh xạ process ↔ GUID |
| `task3/RuntimeIdentity.h` | 12 | Manifest ánh xạ process ↔ GUID |
| `task3/Shutdown.h` | 21 | Thoát an toàn |
| `task3/Telemetry.idl` | 8 | Kiểu DDS TelemetryEnvelopeV1 |
| `task3/Units.h` | 22 | Đơn vị, chuỗi thời gian |
| `task3/WebDashboard.qml` | 108 | Vỏ QML: WebEngineView + WebChannel |
| `task3/WebFeed.cpp` | 64 | Dữ liệu cho trang web qua QWebChannel (WebFeed) |
| `task3/WebFeed.h` | 57 | Dữ liệu cho trang web qua QWebChannel (WebFeed) |
| `task3/agent/CMakeLists.txt` | 15 | Build |
| `task3/agent_main.cpp` | 83 | Chương trình Agent (Slave) |
| `task3/config/flow-rules.json` | 4 | Cấu hình mẫu |
| `task3/config/linux-agent.json` | 10 | Cấu hình mẫu |
| `task3/config/runtime-agent.json` | 11 | Cấu hình mẫu |
| `task3/config/task3-agent.service.example` | 17 | Cấu hình mẫu |
| `task3/config/time-sync/master.conf.example` | 4 | Cấu hình mẫu |
| `task3/config/time-sync/slave.conf.example` | 3 | Cấu hình mẫu |
| `task3/config/windows-agent.json` | 10 | Cấu hình mẫu |
| `task3/scripts/build-windows.ps1` | 17 | Script build/chạy |
| `task3/scripts/build.sh` | 12 | Script build/chạy |
| `task3/scripts/build_statistics_backend_patched.sh` | 206 | Script build/chạy |
| `task3/scripts/build_task3_patched.sh` | 90 | Script build/chạy |
| `task3/scripts/patched_runtime_common.sh` | 136 | Script build/chạy |
| `task3/scripts/qt_prefix.sh` | 53 | Chọn bộ Qt, kiểm module thiếu (WebEngine, Qt5Compat) |
| `task3/scripts/run_linux_agent_patched.sh` | 23 | Script build/chạy |
| `task3/scripts/run_master_original.sh` | 80 | Chạy Master với Statistics Backend 2.5.0 Original |
| `task3/scripts/run_master_patched.sh` | 25 | Script build/chạy |
| `task3/scripts/windows-time-status.ps1` | 15 | Script build/chạy |
| `task3/tests/CoreTests.cpp` | 1294 | Kiểm thử |
| `task3/tests/DashboardPreview.cpp` | 145 | So sánh ảnh / bấm thử trang QML và Web |
| `task3/tests/RuntimeProbe.cpp` | 69 | Kiểm thử |
| `task3/tests/continuation_integration.py` | 145 | Kiểm thử |
| `task3/tests/final_fix_ui_regression.py` | 74 | Kiểm thử |
| `task3/tests/local_integration.py` | 150 | Kiểm thử |
| `task3/tests/qml/NetworkScrollRunner.cpp` | 13 | Test QML |
| `task3/tests/qml/graph/SpyHelper.qml` | 6 | Test tự động GRAPH-01 (Domain Graph gốc, offscreen) |
| `task3/tests/qml/graph/tst_DomainGraphLoading.qml` | 122 | Test tự động GRAPH-01 (Domain Graph gốc, offscreen) |
| `task3/tests/qml/tst_NetworkScroll.qml` | 114 | Test QML |
| `task3/tests/qos_match_local.py` | 71 | Test QoS match/no-match HelloWorld (E2E-12) |
| `task3/tests/runtime_library_verification.py` | 20 | Kiểm thử |
| `task3/tests/ui_integration.py` | 198 | Kiểm thử |
| `task3/tests/web/mock_channel.js` | 88 | Test trình duyệt: QWebChannel giả |
| `task3/tests/web/perf.js` | 51 | Test trình duyệt: đo thời gian render |
| `task3/tests/web/run_web_tests.py` | 129 | Test trình duyệt: chạy Chrome/Edge headless |
| `task3/tests/web/selftest.js` | 350 | Test trình duyệt: 17 ca chức năng |
| `task3/web/app.js` | 369 | Giao diện Task 3 trên WebEngine (HTML/CSS/JS) |
| `task3/web/index.html` | 97 | Giao diện Task 3 trên WebEngine (HTML/CSS/JS) |
| `task3/web/style.css` | 206 | Giao diện Task 3 trên WebEngine (HTML/CSS/JS) |

---

## 5. Tài liệu và bằng chứng mới

<details><summary>docs/task3/ (14 file)</summary>

- `TASK3_README.md`
- `docs/task3/ARCHITECTURE.md`
- `docs/task3/BUILD_AND_RUN.md`
- `docs/task3/COMMUNICATION.md`
- `docs/task3/CONFIGURATION.md`
- `docs/task3/CRASH_ROOT_CAUSE_ANALYSIS.md`
- `docs/task3/DATA_MODEL.md`
- `docs/task3/KNOWN_ISSUES.md`
- `docs/task3/MASTER_LINUX_WINDOWS_E2E_TEST.md`
- `docs/task3/STATISTICS_UNITS.md`
- `docs/task3/TASK3_REQUIREMENTS.md`
- `docs/task3/THAY_DOI_SO_VOI_UPSTREAM.md`
- `docs/task3/TIEN_DO_KIEM_THU.md`
- `docs/task3/TIME_SYNC_SETUP.md`

</details>

<details><summary>test-artifacts/ (149 file)</summary>

- `test-artifacts/final-fix-20261019/continuation/qml-errors.json`
- `test-artifacts/final-fix-20261019/continuation/results.json`
- `test-artifacts/final-fix-20261019/continuation/runtime-library-maps.json`
- `test-artifacts/final-fix-20261019/local/results.json`
- `test-artifacts/final-fix-20261019/local/runtime-library-maps.json`
- `test-artifacts/final-fix-20261019/p1-ui-debug/results.json`
- `test-artifacts/final-fix-20261019/p1-ui-debug/runtime-library-maps.json`
- `test-artifacts/final-fix-20261019/p1-ui-red/results.json`
- `test-artifacts/final-fix-20261019/p1-ui-red/runtime-library-maps.json`
- `test-artifacts/final-fix-20261019/p1-ui-release/results.json`
- `test-artifacts/final-fix-20261019/p1-ui-release/runtime-library-maps.json`
- `test-artifacts/final-fix-20261019/protected.sha256`
- `test-artifacts/final-fix-20261019/standalone-telemetry.json`
- `test-artifacts/final-fix-20261019/ui-1600/default_fleet_overview.json`
- `test-artifacts/final-fix-20261019/ui-1600/mapped_application_and_flow.json`
- `test-artifacts/final-fix-20261019/ui-1600/offline_event_page.json`
- `test-artifacts/final-fix-20261019/ui-1600/offline_stale.json`
- `test-artifacts/final-fix-20261019/ui-1600/offline_ui_selected.json`
- `test-artifacts/final-fix-20261019/ui-1600/offline_unverified_alert.json`
- `test-artifacts/final-fix-20261019/ui-1600/qml-errors.json`
- `test-artifacts/final-fix-20261019/ui-1600/real_agents_and_unknown_host.json`
- `test-artifacts/final-fix-20261019/ui-1600/recovered_event_page.json`
- `test-artifacts/final-fix-20261019/ui-1600/results.json`
- `test-artifacts/final-fix-20261019/ui-1600/runtime-library-maps.json`
- `test-artifacts/final-fix-20261019/ui-1600/ui-observations.json`
- `test-artifacts/main-project-20261016/agent-wrapper-smoke.log`
- `test-artifacts/main-project-20261016/binary-library-sha256.txt`
- `test-artifacts/main-project-20261016/continuation-15/results.json`
- `test-artifacts/main-project-20261016/continuation-15/runtime-library-maps.json`
- `test-artifacts/main-project-20261016/continuation-rerun/qml-errors.json`
- `test-artifacts/main-project-20261016/continuation-rerun/results.json`
- `test-artifacts/main-project-20261016/continuation-rerun/runtime-library-maps.json`
- `test-artifacts/main-project-20261016/ctest-debug.log`
- `test-artifacts/main-project-20261016/ctest-release.log`
- `test-artifacts/main-project-20261016/local-14/results.json`
- `test-artifacts/main-project-20261016/local-14/runtime-library-maps.json`
- `test-artifacts/main-project-20261016/master-wrapper-smoke/master-wrapper.log`
- `test-artifacts/main-project-20261016/patched-backend-manifest.txt`
- `test-artifacts/main-project-20261016/publisher-30/result.json`
- `test-artifacts/main-project-20261016/qt-test-debug.log`
- `test-artifacts/main-project-20261016/qt-test-release.log`
- `test-artifacts/main-project-20261016/runtime-linkage.txt`
- `test-artifacts/main-project-20261016/subscriber-restart/results.json`
- `test-artifacts/main-project-20261016/subscriber-restart/runtime-library-maps.json`
- `test-artifacts/original-20261102/README.md`
- `test-artifacts/original-20261102/graph01/graph-three-hosts-fixed.png`
- `test-artifacts/original-20261102/graph01/graph01-hostslist-width.patch`
- `test-artifacts/original-20261102/graph01/qmltest-fixed.txt`
- `test-artifacts/original-20261102/graph01/qmltest-original.txt`
- `test-artifacts/original-20261102/patched-publisher-30-run1.stdout`
- `test-artifacts/original-20261102/patched-publisher-30-run1/result.json`
- `test-artifacts/original-20261102/patched-publisher-30-run2.stdout`
- `test-artifacts/original-20261102/patched-publisher-30-run2/result.json`
- `test-artifacts/original-20261102/publisher-30-rerun.stdout`
- `test-artifacts/original-20261102/publisher-30-rerun/crash-backtrace.txt`
- `test-artifacts/original-20261102/publisher-30-rerun/result.json`
- `test-artifacts/original-20261102/publisher-30.stdout`
- `test-artifacts/original-20261102/publisher-30/result.json`
- `test-artifacts/original-20261102/qos-local/result.json`
- `test-artifacts/original-20261102/qos-profiles/both-best-effort.xml`
- `test-artifacts/original-20261102/qos-profiles/compatible.xml`
- `test-artifacts/original-20261102/qos-profiles/incompatible-writer.xml`
- `test-artifacts/original-20261102/shutdown-check-1/result.json`
- `test-artifacts/original-20261102/shutdown-check-2/result.json`
- `test-artifacts/original-20261102b/README.md`
- `test-artifacts/original-20261102b/continuation-rerun.stdout`
- `test-artifacts/original-20261102b/continuation.stdout`
- `test-artifacts/original-20261102b/continuation/qml-errors.json`
- `test-artifacts/original-20261102b/continuation/results.json`
- `test-artifacts/original-20261102b/continuation/runtime-library-maps.json`
- `test-artifacts/original-20261102b/ctest-release.stdout`
- `test-artifacts/original-20261102b/local-rerun.stdout`
- `test-artifacts/original-20261102b/local.stdout`
- `test-artifacts/original-20261102b/local/results.json`
- `test-artifacts/original-20261102b/local/runtime-library-maps.json`
- `test-artifacts/original-20261102b/publisher-30-run1.stdout`
- `test-artifacts/original-20261102b/publisher-30-run1/crash-backtrace.txt`
- `test-artifacts/original-20261102b/publisher-30-run1/result.json`
- `test-artifacts/original-20261102b/publisher-30-run2.stdout`
- `test-artifacts/original-20261102b/publisher-30-run2/crash-backtrace.txt`
- `test-artifacts/original-20261102b/publisher-30-run2/result.json`
- `test-artifacts/original-20261102b/publisher-30-run3.stdout`
- `test-artifacts/original-20261102b/publisher-30-run3/result.json`
- `test-artifacts/original-20261102b/publisher-30-run4.stdout`
- `test-artifacts/original-20261102b/publisher-30-run4/crash-backtrace.txt`
- `test-artifacts/original-20261102b/publisher-30-run4/result.json`
- `test-artifacts/original-20261102b/publisher-30-run5.stdout`
- `test-artifacts/original-20261102b/publisher-30-run5/crash-backtrace.txt`
- `test-artifacts/original-20261102b/publisher-30-run5/result.json`
- `test-artifacts/original-20261102b/run_all.sh`
- `test-artifacts/original-20261102b/run_integration.sh`
- `test-artifacts/original-20261102b/subscriber-restart.stdout`
- `test-artifacts/original-20261102b/subscriber-restart/results.json`
- `test-artifacts/original-20261102b/subscriber-restart/runtime-library-maps.json`
- `test-artifacts/original-20261102b/summary.txt`
- `test-artifacts/p0-crash-20261016/Fast-DDS-Statistics-Backend-2.5.0-database-queue-lifecycle.patch`
- `test-artifacts/p0-crash-20261016/dds-monitor-gdb`
- `test-artifacts/p0-crash-20261016/publisher_stop_restart_reproducer.py`
- `test-artifacts/p0-crash-20261016/subscriber_restart_regression.py`
- `test-artifacts/patched-20261102/README.md`
- `test-artifacts/patched-20261102/continuation.stdout`
- `test-artifacts/patched-20261102/continuation/qml-errors.json`
- `test-artifacts/patched-20261102/continuation/results.json`
- `test-artifacts/patched-20261102/continuation/runtime-library-maps.json`
- `test-artifacts/patched-20261102/ctest-release.stdout`
- `test-artifacts/patched-20261102/local.stdout`
- `test-artifacts/patched-20261102/local/results.json`
- `test-artifacts/patched-20261102/local/runtime-library-maps.json`
- `test-artifacts/patched-20261102/publisher-30-run1.stdout`
- `test-artifacts/patched-20261102/publisher-30-run1/result.json`
- `test-artifacts/patched-20261102/publisher-30-run2.stdout`
- `test-artifacts/patched-20261102/publisher-30-run2/result.json`
- `test-artifacts/patched-20261102/publisher-30-run3.stdout`
- `test-artifacts/patched-20261102/publisher-30-run3/result.json`
- `test-artifacts/patched-20261102/publisher-30-run4.stdout`
- `test-artifacts/patched-20261102/publisher-30-run4/result.json`
- `test-artifacts/patched-20261102/publisher-30-run5.stdout`
- `test-artifacts/patched-20261102/publisher-30-run5/result.json`
- `test-artifacts/patched-20261102/run_all.sh`
- `test-artifacts/patched-20261102/subscriber-restart.stdout`
- `test-artifacts/patched-20261102/subscriber-restart/results.json`
- `test-artifacts/patched-20261102/subscriber-restart/runtime-library-maps.json`
- `test-artifacts/patched-20261102/summary.txt`
- `test-artifacts/qt683-20261102/README.md`
- `test-artifacts/qt683-20261102/ctest-release.stdout`
- `test-artifacts/qt683-20261102/graph-qml.stdout`
- `test-artifacts/qt683-20261102/local.stdout`
- `test-artifacts/qt683-20261102/run_all.sh`
- `test-artifacts/qt683-20261102/summary.txt`
- `test-artifacts/webengine-20261102/README.md`
- `test-artifacts/webengine-20261102/compare_qml_web.sh`
- `test-artifacts/webengine-20261102/pixel-diff.txt`
- `test-artifacts/webengine-20261102/shots/apps-pair.png`
- `test-artifacts/webengine-20261102/shots/events-pair.png`
- `test-artifacts/webengine-20261102/shots/flows-pair.png`
- `test-artifacts/webengine-20261102/shots/network-pair.png`
- `test-artifacts/webengine-20261102/shots/offline-pair.png`
- `test-artifacts/webengine-20261102/shots/overview-pair.png`
- `test-artifacts/webengine-20261102/shots/resources-pair.png`
- `test-artifacts/webengine-20261102/shots/time-pair.png`
- `test-artifacts/webengine-20261103/browser/apps-web.png`
- `test-artifacts/webengine-20261103/browser/events-web.png`
- `test-artifacts/webengine-20261103/browser/flows-web.png`
- `test-artifacts/webengine-20261103/browser/network-web.png`
- `test-artifacts/webengine-20261103/browser/offline-web.png`
- `test-artifacts/webengine-20261103/browser/overview-web.png`
- `test-artifacts/webengine-20261103/browser/resources-web.png`
- `test-artifacts/webengine-20261103/browser/results.json`
- `test-artifacts/webengine-20261103/browser/time-web.png`

</details>

<details><summary>.task3-migration-checkpoints/ (23 file)</summary>

- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/ARCHITECTURE.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/BUILD_AND_RUN.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/COMMUNICATION.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/CONFIGURATION.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/CONTINUATION_CHECKPOINT.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/CRASH_ROOT_CAUSE_ANALYSIS.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/DATA_MODEL.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/DELIVERY_REPORT.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/IMPLEMENTATION_PLAN.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/IMPLEMENTATION_STATUS.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/KNOWN_ISSUES.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/POC_BASELINE.patch`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/PRE_TEST_READINESS.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/STATISTICS_UNITS.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/TASK3_REQUIREMENTS.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/TEST_PLAN.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/TEST_RESULTS.md`
- `.task3-migration-checkpoints/20261016-pre-integration/docs-task3-before/TIME_SYNC_SETUP.md`
- `.task3-migration-checkpoints/20261016-pre-integration/git-log.txt`
- `.task3-migration-checkpoints/20261016-pre-integration/git-status.txt`
- `.task3-migration-checkpoints/20261016-pre-integration/head.txt`
- `.task3-migration-checkpoints/20261016-pre-integration/index.diff`
- `.task3-migration-checkpoints/20261016-pre-integration/worktree.diff`

</details>

---

## 6. Lệnh tự kiểm tra

```bash
git fetch origin main                                # origin = eProsima/DDS-Monitor
git diff --stat 94bbd6d HEAD                         # tổng quan
git diff --name-status --diff-filter=M 94bbd6d HEAD  # chỉ file gốc bị sửa
git diff 94bbd6d HEAD -- src/ qml/ include/ CMakeLists.txt   # diff code gốc
```
