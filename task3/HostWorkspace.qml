// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts
import Theme 1.0

ColumnLayout {
    id: workspace
    required property var ui
    readonly property var host: ui.selectedHost
    readonly property var application: ui.selectedApplication
    readonly property var participants: task3Core.entities.filter(e => e.kind === "Participant" && e.host_id === ui.selectedHostId && e.application_id === ui.selectedApplicationId)
    spacing: 12

    Frame {
        Layout.fillWidth: true
        padding: 14
        background: Rectangle {
            color: "white";
            radius: 4;
            border.color: Theme.lightGrey
        }
        ColumnLayout {
            width: parent.width
            RowLayout {
                Layout.fillWidth: true
                Label {
                    text: workspace.host.hostname || ui.selectedHostId;
                    font.pixelSize: 23;
                    font.bold: true;
                    color: Theme.eProsimaDarkBlue;
                    Layout.fillWidth: true;
                    elide: Text.ElideRight
                }
                Label {
                    text: "● " + ui.hostState(workspace.host);
                    color: ui.stateColor(workspace.host);
                    font.bold: true
                }
                Label {
                    text: ui.freshness(workspace.host);
                    color: Theme.midGrey
                }
            }
            Label {
                text: "Host ID: " + ui.selectedHostId + "  ·  " + (workspace.host.os || "Unknown");
                wrapMode: Text.Wrap;
                Layout.fillWidth: true;
                color: Theme.midGrey
            }
            Label {
                text: "Last Seen: " + ui.lastSeen(workspace.host.last_seen) + "  ·  Telemetry source: " + (workspace.host.address || "N/A");
                font.pixelSize: 13;
                wrapMode: Text.Wrap;
                Layout.fillWidth: true
            }
            Label {
                visible: !ui.fresh(workspace.host);
                text: workspace.host.host_state || "Unknown — chưa có telemetry";
                color: "#9a6700";
                wrapMode: Text.Wrap;
                Layout.fillWidth: true
            }
        }
    }
    TabBar {
        id: hostTabs
        Layout.fillWidth: true
        currentIndex: ui.detailTab
        onCurrentIndexChanged: ui.detailTab = currentIndex
        Connections {
            target: ui;
            function onDetailTabChanged() {
                hostTabs.currentIndex = ui.detailTab
            }
        }
        Repeater {
            model: ["Tài nguyên", "Ứng dụng", "Luồng DDS", "Đồng bộ giờ"]
            TabButton {
                id: detailButton
                required property string modelData
                text: modelData
                contentItem: Label {
                    text: detailButton.text;
                    horizontalAlignment: Text.AlignHCenter;
                    font.bold: detailButton.checked;
                    color: detailButton.checked ? Theme.eProsimaDarkBlue : Theme.midGrey
                }
            }
        }
    }
    Loader {
        Layout.fillWidth: true
        Layout.fillHeight: true
        sourceComponent: [resourcesPage, applicationsPage, flowsPage, timePage][hostTabs.currentIndex]
    }
    Component {
        id: resourcesPage
        ColumnLayout {
            id: resourcePane
            spacing: 8
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 100
                Layout.minimumHeight: 104
                Layout.maximumHeight: 112
                spacing: 12
                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    Layout.minimumWidth: 0
                    background: Rectangle {
                        color: "white";
                        border.color: Theme.lightGrey;
                        radius: 4
                    }
                    ColumnLayout {
                        width: parent.width
                        Label {
                            text: "CPU";
                            color: Theme.midGrey;
                            font.pixelSize: 13
                        }
                        Label {
                            text: ui.metric(workspace.host, workspace.host.metrics?.cpu_percent, false) + " %";
                            font.pixelSize: 27;
                            font.bold: true;
                            color: Theme.eProsimaDarkBlue
                        }
                    }
                }
                Frame {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 1
                    Layout.minimumWidth: 0
                    background: Rectangle {
                        color: "white";
                        border.color: Theme.lightGrey;
                        radius: 4
                    }
                    ColumnLayout {
                        width: parent.width
                        Label {
                            text: "RAM  ·  USED / TOTAL";
                            color: Theme.midGrey;
                            font.pixelSize: 13
                        }
                        Label {
                            text: ui.metric(workspace.host, workspace.host.metrics?.ram_percent, false) + " %"
                            font.pixelSize: 27;
                            font.bold: true;
                            color: Theme.eProsimaDarkBlue
                        }
                        Label {
                            text: ui.metric(workspace.host, workspace.host.collector_health?.ram === "unavailable" ? null : workspace.host.metrics?.ram_used, true) + " / " + ui.metric(workspace.host, workspace.host.collector_health?.ram === "unavailable" ? null : workspace.host.metrics?.ram_total, true)
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true;
                            font.pixelSize: 13
                        }
                    }
                }
            }
            ChartView {
                id: cpuChart
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(185, Math.max(85, resourcePane.height * 0.28))
                Layout.minimumHeight: 70
                antialiasing: true
                legend.visible: false
                title: "CPU (%) · tối đa 120 mẫu nhận gần nhất · " + ui.freshness(workspace.host)
                margins {
                    left: 0;
                    right: 0;
                    top: 0;
                    bottom: 0
                }
                ValueAxis {
                    id: cpuX;
                    min: 0;
                    max: 120;
                    labelsVisible: false;
                    tickCount: 5
                }
                ValueAxis {
                    id: cpuY;
                    min: 0;
                    max: 100;
                    tickCount: 3;
                    labelFormat: "%.0f"
                }
                LineSeries {
                    id: cpuSeries;
                    axisX: cpuX;
                    axisY: cpuY;
                    color: Theme.eProsimaLightBlue;
                    width: 2
                }
                function updateSeries() {
                    cpuSeries.clear()
                    const points = workspace.host.cpu_history || []
                    for (let i = 0; i < points.length; ++i)
                        cpuSeries.append(i, points[i].value)
                }
                Component.onCompleted: updateSeries()
                Connections {
                    target: workspace;
                    function onHostChanged() {
                        cpuChart.updateSeries()
                    }
                }
            }
            TabBar {
                id: resourceTabs
                Layout.fillWidth: true
                currentIndex: ui.resourceTab
                onCurrentIndexChanged: ui.resourceTab = currentIndex
                Connections {
                    target: ui;
                    function onResourceTabChanged() {
                        resourceTabs.currentIndex = ui.resourceTab
                    }
                }
                Repeater {
                    model: ["Ổ đĩa / phân vùng", "Mạng / interface"]
                    TabButton {
                        id: resourceButton
                        required property string modelData
                        text: modelData
                        contentItem: Label {
                            text: resourceButton.text;
                            horizontalAlignment: Text.AlignHCenter;
                            color: resourceButton.checked ? Theme.eProsimaDarkBlue : Theme.midGrey
                        }
                    }
                }
            }
            StackLayout {
                Layout.fillWidth: true;
                Layout.fillHeight: true
                Layout.minimumHeight: 120
                currentIndex: resourceTabs.currentIndex
                ListView {
                    id: disks
                    clip: true
                    model: workspace.host.metrics?.disks || []
                    spacing: 6
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Frame {
                        required property var modelData
                        width: disks.width
                        background: Rectangle {
                            color: "white";
                            border.color: Theme.lightGrey;
                            radius: 4
                        }
                        contentItem: RowLayout {
                            spacing: 16
                            Label {
                                text: modelData.path;
                                font.bold: true;
                                Layout.fillWidth: true;
                                Layout.minimumWidth: 40;
                                elide: Text.ElideRight;
                                ToolTip.visible: diskHover.hovered;
                                ToolTip.text: text;
                                HoverHandler {
                                    id: diskHover
                                }
                            }
                            ColumnLayout {
                                Layout.preferredWidth: 140;
                                Label {
                                    text: "TOTAL";
                                    font.pixelSize: 12;
                                    color: Theme.midGrey
                                }
                                Label {
                                    text: ui.metric(workspace.host, modelData.valid ? modelData.total : null, true)
                                }
                            }
                            ColumnLayout {
                                Layout.preferredWidth: 140;
                                Label {
                                    text: "USED";
                                    font.pixelSize: 12;
                                    color: Theme.midGrey
                                }
                                Label {
                                    text: ui.metric(workspace.host, modelData.valid ? modelData.used : null, true)
                                }
                            }
                            ColumnLayout {
                                Layout.preferredWidth: 140;
                                Label {
                                    text: "FREE";
                                    font.pixelSize: 12;
                                    color: Theme.midGrey
                                }
                                Label {
                                    text: ui.metric(workspace.host, modelData.valid ? modelData.free : null, true)
                                }
                            }
                        }
                    }
                    Label {
                        anchors.centerIn: parent;
                        visible: disks.count === 0;
                        text: "N/A — chưa có dữ liệu ổ đĩa";
                        color: Theme.midGrey
                    }
                }
                ListView {
                    id: network
                    objectName: "hostNetworkMetrics"
                    header: Label {
                        width: network.width;
                        text: "RX/TX theo từng interface · tốc độ B/s · bộ đếm byte tích lũy";
                        color: Theme.midGrey;
                        font.pixelSize: 12;
                        wrapMode: Text.Wrap;
                        bottomPadding: 8
                    }
                    property var interfaces: workspace.host.metrics?.interfaces || []
                    property string selectedHostId: ui.selectedHostId
                    property string modelHostId: ""
                    model: ListModel {
                        id: networkModel
                        dynamicRoles: true
                    }

                    function syncInterfaces() {
                        const hostChanged = modelHostId !== selectedHostId
                        const rows = interfaces.slice().sort(function(a, b) {
                            const left = String(a.name)
                            const right = String(b.name)
                            if (/^\d+$/.test(left) && /^\d+$/.test(right)) {
                                const difference = Number(left) - Number(right)
                                if (difference !== 0)
                                    return difference
                            }
                            return left < right ? -1 : left > right ? 1 : 0
                        })
                        const oldY = contentY
                        const anchorIndex = indexAt(1, contentY + 1)
                        const anchor = anchorIndex >= 0 ? networkModel.get(anchorIndex).interfaceKey : ""
                        const anchorItem = anchorIndex >= 0 ? itemAtIndex(anchorIndex) : null
                        const anchorOffset = anchorItem ? contentY - anchorItem.y : 0
                        let structureChanged = hostChanged
                        if (hostChanged) {
                            networkModel.clear()
                            modelHostId = selectedHostId
                        }
                        for (let i = 0; i < rows.length; ++i) {
                            const key = String(rows[i].name)
                            let existing = i
                            while (existing < networkModel.count && networkModel.get(existing).interfaceKey !== key)
                                ++existing
                            if (existing === networkModel.count) {
                                networkModel.insert(i, {interfaceKey: key, modelData: rows[i]})
                                structureChanged = true
                            } else {
                                if (existing !== i) {
                                    networkModel.move(existing, i, 1)
                                    structureChanged = true
                                }
                                networkModel.setProperty(i, "modelData", rows[i])
                            }
                        }
                        if (networkModel.count > rows.length) {
                            networkModel.remove(rows.length, networkModel.count - rows.length)
                            structureChanged = true
                        }
                        if (structureChanged) {
                            forceLayout()
                            let restoredY = oldY
                            for (let i = 0; !hostChanged && anchor && i < networkModel.count; ++i) {
                                if (networkModel.get(i).interfaceKey === anchor) {
                                    const item = itemAtIndex(i)
                                    if (item)
                                        restoredY = item.y + anchorOffset
                                    break
                                }
                            }
                            contentY = hostChanged ? originY : Math.max(originY, Math.min(restoredY, originY + Math.max(0, contentHeight - height)))
                        }
                    }
                    onInterfacesChanged: syncInterfaces()
                    onSelectedHostIdChanged: syncInterfaces()
                    Component.onCompleted: syncInterfaces()
                    clip: true;
                    spacing: 6
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Frame {
                        required property var modelData
                        width: network.width
                        background: Rectangle {
                            color: "white";
                            border.color: Theme.lightGrey;
                            radius: 4
                        }
                        contentItem: RowLayout {
                            Label {
                                text: modelData.name;
                                font.bold: true;
                                Layout.fillWidth: true;
                                elide: Text.ElideRight;
                                ToolTip.visible: networkHover.hovered;
                                ToolTip.text: text;
                                HoverHandler {
                                    id: networkHover
                                }
                            }
                            ColumnLayout {
                                Layout.preferredWidth: 230
                                Label {
                                    text: "RX  " + ui.metric(workspace.host, modelData.rate_valid === true ? modelData.rx_bps : null, false) + " B/s";
                                    color: Theme.eProsimaDarkBlue
                                }
                                Label {
                                    text: ui.metric(workspace.host, modelData.rx_bytes, false) + " bytes";
                                    font.pixelSize: 12;
                                    color: Theme.midGrey
                                }
                            }
                            ColumnLayout {
                                Layout.preferredWidth: 230
                                Label {
                                    text: "TX  " + ui.metric(workspace.host, modelData.rate_valid === true ? modelData.tx_bps : null, false) + " B/s";
                                    color: Theme.eProsimaDarkBlue
                                }
                                Label {
                                    text: ui.metric(workspace.host, modelData.tx_bytes, false) + " bytes";
                                    font.pixelSize: 12;
                                    color: Theme.midGrey
                                }
                            }
                        }
                    }
                    Label {
                        anchors.centerIn: parent;
                        visible: network.count === 0;
                        text: "N/A — chưa có dữ liệu interface";
                        color: Theme.midGrey
                    }
                }
            }
        }
    }
    Component {
        id: applicationsPage
        SplitView {
            orientation: width > 800 ? Qt.Horizontal : Qt.Vertical
            ListView {
                id: apps
                SplitView.preferredWidth: 260
                SplitView.preferredHeight: 160
                SplitView.minimumHeight: 85
                clip: true;
                spacing: 6
                model: workspace.host.applications || []
                ScrollBar.vertical: ScrollBar {}
                delegate: ItemDelegate {
                    id: applicationButton
                    required property var modelData
                    width: apps.width;
                    height: 76
                    highlighted: ui.selectedApplicationId === modelData.application_id
                    background: Rectangle {
                        radius: 4;
                        color: applicationButton.highlighted ? "#eaf2f9" : applicationButton.hovered ? "#f3f6f9" : "white";
                        border.color: applicationButton.highlighted || applicationButton.activeFocus ? Theme.eProsimaDarkBlue : Theme.lightGrey
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: modelData.application_id
                    onClicked: ui.chooseApplication(ui.selectedHostId, modelData.application_id)
                    contentItem: Column {
                        spacing: 6
                        Label {
                            width: parent.width;
                            text: modelData.application_id;
                            font.bold: true;
                            elide: Text.ElideRight
                        }
                        Label {
                            width: parent.width;
                            text: ui.fresh(workspace.host) ? "Process: " + modelData.state + " · Service: " + modelData.service_state : "Unknown · Agent " + ui.hostState(workspace.host);
                            elide: Text.ElideRight;
                            font.pixelSize: 12;
                            color: Theme.midGrey
                        }
                    }
                }
                Label {
                    anchors.centerIn: parent;
                    visible: apps.count === 0;
                    text: "Chưa có ứng dụng được cấu hình";
                    wrapMode: Text.Wrap;
                    width: parent.width;
                    color: Theme.midGrey
                }
            }
            ScrollView {
                id: applicationScroll
                SplitView.fillWidth: true;
                SplitView.fillHeight: true
                clip: true
                contentWidth: availableWidth
                ColumnLayout {
                    width: applicationScroll.availableWidth - 16
                    x: 8
                    spacing: 12
                    Label {
                        text: ui.selectedApplicationId || "Chọn một ứng dụng";
                        font.pixelSize: 22;
                        color: Theme.eProsimaDarkBlue;
                        font.bold: true;
                        wrapMode: Text.Wrap;
                        Layout.fillWidth: true
                    }
                    Label {
                        visible: !!ui.selectedApplicationId && !ui.fresh(workspace.host);
                        text: "Unknown — Agent không phản hồi. Thông tin dưới đây là lần ghi nhận trước, chưa thể xác minh hiện tại.";
                        color: "#9a6700";
                        wrapMode: Text.Wrap;
                        Layout.fillWidth: true
                    }
                    GridLayout {
                        visible: !!ui.selectedApplicationId
                        columns: 2;
                        columnSpacing: 18;
                        rowSpacing: 12
                        Layout.fillWidth: true
                        Label {
                            text: "Process";
                            color: Theme.midGrey
                        }
                        Label {
                            text: (workspace.application.process_name || "N/A") + " · " + (ui.fresh(workspace.host) ? workspace.application.state || "Unknown" : "Unknown (last: " + (workspace.application.state || "Unknown") + ")");
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "Service";
                            color: Theme.midGrey
                        }
                        Label {
                            text: (workspace.application.service || "Không cấu hình") + " · " + (ui.fresh(workspace.host) ? workspace.application.service_state || "Unknown" : "Unknown");
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "PID / instances";
                            color: Theme.midGrey
                        }
                        Label {
                            text: (workspace.application.pid || "N/A") + " / " + (workspace.application.instances || []).length
                        }
                    }
                    Repeater {
                        model: workspace.application.instances || []
                        delegate: Frame {
                            required property var modelData
                            Layout.fillWidth: true
                            ColumnLayout {
                                width: parent.width
                                Label {
                                    text: "PID " + modelData.pid + " · " + (modelData.state || "Unknown");
                                    font.bold: true
                                }
                                Label {
                                    text: "Instance: " + modelData.instance_id;
                                    wrapMode: Text.Wrap;
                                    Layout.fillWidth: true;
                                    font.pixelSize: 12
                                }
                                Label {
                                    text: "Binding: " + (modelData.binding_source || "Unmapped");
                                    color: Theme.midGrey
                                }
                            }
                        }
                    }
                    Label {
                        visible: !!ui.selectedApplicationId;
                        text: "Participant đã ánh xạ: " + workspace.participants.length;
                        font.bold: true
                    }
                    Repeater {
                        model: workspace.participants
                        delegate: Label {
                            required property var modelData
                            text: modelData.guid + "\nPID " + modelData.pid + " · " + (modelData.mapping_state || "Unmapped")
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true;
                            font.pixelSize: 12
                        }
                    }
                    Label {
                        visible: !!ui.selectedApplicationId && workspace.participants.length === 0;
                        text: "Unmapped — chưa có bằng chứng liên kết DDS với lần chạy này.";
                        color: Theme.midGrey;
                        wrapMode: Text.Wrap;
                        Layout.fillWidth: true
                    }
                    Flow {
                        Layout.fillWidth: true;
                        spacing: 8
                        Button {
                            text: "Xem luồng ứng dụng";
                            enabled: !!ui.selectedApplicationId;
                            onClicked: ui.detailTab = 2
                        }
                        Button {
                            text: "Mở DDS chuyên sâu";
                            onClicked: ui.openDdsWorkspace("")
                        }
                    }
                }
            }
        }
    }
    Component {
        id: flowsPage;
        FlowDetails {
            ui: workspace.ui
        }
    }
    Component {
        id: timePage
        ScrollView {
            id: timeScroll
            clip: true
            contentWidth: availableWidth
            ColumnLayout {
                width: timeScroll.availableWidth
                spacing: 18
                Label {
                    text: "Đồng bộ thời gian";
                    font.pixelSize: 23;
                    color: Theme.eProsimaDarkBlue;
                    font.bold: true
                }
                Label {
                    text: "Nguồn dữ liệu: dịch vụ hệ điều hành trên Slave · cập nhật: " + ui.time(workspace.host.time_sync?.checked_ms);
                    wrapMode: Text.Wrap;
                    Layout.fillWidth: true;
                    color: Theme.midGrey
                }
                Frame {
                    Layout.fillWidth: true
                    background: Rectangle {
                        color: "white";
                        radius: 4;
                        border.color: Theme.lightGrey
                    }
                    GridLayout {
                        width: parent.width;
                        columns: 2;
                        rowSpacing: 18;
                        columnSpacing: 20
                        Label {
                            text: "Trạng thái";
                            color: Theme.midGrey
                        }
                        Label {
                            text: ui.fresh(workspace.host) ? workspace.host.time_sync?.state || "Unknown" : "Unknown · " + ui.freshness(workspace.host);
                            font.bold: true
                        }
                        Label {
                            text: "Dịch vụ đang chạy";
                            color: Theme.midGrey
                        }
                        Label {
                            text: ui.fresh(workspace.host) ? ui.booleanText(workspace.host.time_sync?.service_running) : "Unknown"
                        }
                        Label {
                            text: "Source / Reference ID";
                            color: Theme.midGrey
                        }
                        Label {
                            text: workspace.host.time_sync?.source || "Unavailable";
                            wrapMode: Text.Wrap;
                            Layout.fillWidth: true
                        }
                        Label {
                            text: "Đồng bộ với nguồn đã chọn";
                            color: Theme.midGrey
                        }
                        Label {
                            text: ui.fresh(workspace.host) ? ui.booleanText(workspace.host.time_sync?.clock_synced) : "Unknown"
                        }
                        Label {
                            text: "Nguồn Master đã xác minh";
                            color: Theme.midGrey
                        }
                        Label {
                            text: ui.fresh(workspace.host) ? ui.booleanText(workspace.host.time_sync?.master_source_verified) : "Unknown"
                        }
                        Label {
                            text: workspace.host.time_sync?.offset_kind === "absolute_w32time_estimate" ? "Windows absolute offset estimate" : "Chrony Last offset";
                            color: Theme.midGrey
                        }
                        Label {
                            text: ui.offset(ui.fresh(workspace.host) ? workspace.host.time_sync?.offset_seconds : null)
                        }
                        Label {
                            text: "Chrony System time correction";
                            color: Theme.midGrey
                        }
                        Label {
                            text: ui.offset(ui.fresh(workspace.host) ? workspace.host.time_sync?.system_time_seconds : null)
                        }
                    }
                }
                Label {
                    text: workspace.host.time_sync?.detail || "Unknown — chưa có kết quả truy vấn dịch vụ đồng bộ.";
                    wrapMode: Text.Wrap;
                    Layout.fillWidth: true;
                    color: Theme.midGrey
                }
            }
        }
    }
}
