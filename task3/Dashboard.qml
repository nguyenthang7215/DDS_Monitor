// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme 1.0

Page {
    id: dashboard
    anchors.fill: parent
    padding: 16
    title: "DDS Monitor"
    font.pixelSize: 14
    background: Rectangle {
        color: Theme.whiteSmoke
    }
    property bool eventsPage: false
    readonly property var hosts: task3Core.hosts
    property string selectedHostId: ""
    property string selectedApplicationId: ""
    property string selectedFlowKey: ""
    property int detailTab: 0
    property int resourceTab: 0
    property string hostSearch: ""
    readonly property var selectedHost: hosts.find(h => h.host_id === selectedHostId) || ({})
    readonly property var selectedApplication: (selectedHost.applications || []).find(a => a.application_id === selectedApplicationId) || ({})
    signal openDdsWorkspace(string entityId)
    Component.onCompleted: detailTab = task3StartupTab

    function chooseHost(id) {
        if (id !== selectedHostId) {
            selectedApplicationId = "";
            selectedFlowKey = ""
        }
        selectedHostId = id
        detailTab = 0
    }
    function chooseApplication(hostId, appId) {
        if (hostId !== selectedHostId)
            selectedFlowKey = ""
        selectedHostId = hostId
        selectedApplicationId = appId
        detailTab = 1
    }
    function fresh(h) {
        return h.online === true && h.metrics_state === "fresh"
    }
    function number(v) {
        return v === undefined || v === null || !isFinite(Number(v)) ? "N/A" : Number(v).toFixed(1)
    }
    function mb(v) {
        return v === undefined || v === null || !isFinite(Number(v)) ? "N/A" : (Number(v) / 1048576).toFixed(1) + " MiB"
    }
    function metric(h, v, bytes) {
        return fresh(h) ? (bytes ? mb(v) : number(v)) : "N/A"
    }
    function offset(v) {
        return v === undefined || v === null || !isFinite(Number(v)) ? "Unavailable" : Number(v).toExponential(3) + " s"
    }
    function time(v) {
        return v === undefined || v === null || Number(v) <= 0 || !isFinite(Number(v)) ? "Unknown" : Qt.formatDateTime(new Date(Number(v)), "dd/MM/yyyy HH:mm:ss")
    }
    function lastSeen(v) {
        const date = new Date(v);
        return !v || v === "never" || isNaN(date.getTime()) ? "Chưa có telemetry" : Qt.formatDateTime(date, "dd/MM/yyyy HH:mm:ss")
    }
    function hostState(h) {
        return !h.metrics_state || h.metrics_state === "unknown" ? "Unknown" : h.online ? "Online" : "Offline"
    }
    function stateColor(h) {
        return hostState(h) === "Unknown" ? Theme.grey : h.online ? "#237a45" : "#b33030"
    }
    function freshness(h) {
        return h.metrics_state === "fresh" ? "Fresh" : h.metrics_state === "stale" ? "Stale" : "Unknown"
    }
    function booleanText(v) {
        return typeof v === "boolean" ? (v ? "Có" : "Không") : "Unknown"
    }
    function flowKey(f) {
        return String(f.writer_id || "") + "/" + String(f.reader_id || "")
    }

    SplitView {
        anchors.fill: parent
        visible: !dashboard.eventsPage
        handle: Rectangle {
            implicitWidth: 10;
            color: Theme.whiteSmoke
        }
        Frame {
            SplitView.preferredWidth: 260
            SplitView.minimumWidth: 210
            SplitView.maximumWidth: 380
            padding: 12
            background: Rectangle {
                color: "white";
                border.color: Theme.lightGrey;
                radius: 4
            }
            ColumnLayout {
                anchors.fill: parent
                spacing: 12
                Label {
                    text: "SLAVE & APPLICATION";
                    color: Theme.eProsimaDarkBlue;
                    font.bold: true;
                    font.pixelSize: 13
                }
                TextField {
                    Layout.fillWidth: true;
                    placeholderText: "Tìm tên máy / Host ID";
                    onTextChanged: dashboard.hostSearch = text.toLowerCase()
                }
                Button {
                    Layout.fillWidth: true
                    text: "Tất cả Slave  ·  " + dashboard.hosts.length
                    highlighted: dashboard.selectedHostId === ""
                    onClicked: dashboard.chooseHost("")
                }
                ListView {
                    id: hostList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 8
                    model: dashboard.hosts.filter(h => !dashboard.hostSearch || (h.host_id + " " + h.hostname).toLowerCase().includes(dashboard.hostSearch))
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Column {
                        id: hostNode
                        required property var modelData
                        width: hostList.width
                        spacing: 2
                        ItemDelegate {
                            id: hostButton
                            width: parent.width
                            height: 88
                            leftPadding: 12;
                            rightPadding: 10
                            highlighted: dashboard.selectedHostId === hostNode.modelData.host_id
                            background: Rectangle {
                                radius: 4
                                color: hostButton.highlighted ? "#eaf2f9" : hostButton.hovered ? "#f3f6f9" : "white"
                                border.color: hostButton.activeFocus ? Theme.eProsimaDarkBlue : "transparent"
                                Rectangle {
                                    width: 3;
                                    height: parent.height - 16;
                                    y: 8;
                                    color: Theme.eProsimaDarkBlue;
                                    visible: hostButton.highlighted
                                }
                            }
                            ToolTip.visible: hovered
                            ToolTip.text: hostNode.modelData.host_id
                            onClicked: dashboard.chooseHost(hostNode.modelData.host_id)
                            contentItem: Column {
                                spacing: 5
                                Label {
                                    width: parent.width;
                                    text: hostNode.modelData.hostname || hostNode.modelData.host_id;
                                    font.bold: true;
                                    elide: Text.ElideRight
                                }
                                Label {
                                    width: parent.width;
                                    text: hostNode.modelData.host_id;
                                    color: Theme.midGrey;
                                    elide: Text.ElideRight;
                                    font.pixelSize: 12
                                }
                                Label {
                                    width: parent.width;
                                    elide: Text.ElideRight;
                                    text: "● " + dashboard.hostState(hostNode.modelData) + " · " + (hostNode.modelData.applications || []).length + " ứng dụng";
                                    color: dashboard.stateColor(hostNode.modelData);
                                    font.pixelSize: 13
                                }
                            }
                        }
                        Repeater {
                            model: dashboard.selectedHostId === hostNode.modelData.host_id ? (hostNode.modelData.applications || []) : []
                            delegate: ItemDelegate {
                                id: appButton
                                required property var modelData
                                width: hostNode.width
                                leftPadding: 25
                                height: 36
                                text: modelData.application_id
                                highlighted: dashboard.selectedApplicationId === modelData.application_id
                                background: Rectangle {
                                    radius: 4;
                                    color: appButton.highlighted ? "#eaf2f9" : appButton.hovered ? "#f3f6f9" : "white";
                                    border.color: appButton.activeFocus ? Theme.eProsimaDarkBlue : "transparent"
                                }
                                contentItem: Label {
                                    text: appButton.text;
                                    elide: Text.ElideRight;
                                    verticalAlignment: Text.AlignVCenter;
                                    color: appButton.highlighted ? Theme.eProsimaDarkBlue : "#263747";
                                    font.bold: appButton.highlighted
                                }
                                ToolTip.visible: hovered
                                ToolTip.text: text
                                onClicked: dashboard.chooseApplication(hostNode.modelData.host_id, modelData.application_id)
                            }
                        }
                    }
                }
                Label {
                    text: "Trạng thái Slave dựa trên phản hồi Agent.";
                    wrapMode: Text.Wrap;
                    Layout.fillWidth: true;
                    font.pixelSize: 12;
                    color: Theme.midGrey
                }
            }
        }
        Item {
            SplitView.fillWidth: true
            StackLayout {
                anchors.fill: parent
                currentIndex: dashboard.selectedHostId ? 1 : 0
                ColumnLayout {
                    spacing: 12
                    RowLayout {
                        Layout.fillWidth: true
                        ColumnLayout {
                            Layout.fillWidth: true
                            Label {
                                text: "Tổng quan hệ thống";
                                font.pixelSize: 24;
                                font.bold: true;
                                color: Theme.eProsimaDarkBlue
                            }
                            Label {
                                text: "Chọn một Slave để xem tài nguyên, ứng dụng và các luồng DDS.";
                                color: Theme.midGrey;
                                wrapMode: Text.Wrap;
                                Layout.fillWidth: true
                            }
                        }
                        Button {
                            text: "Mở DDS chuyên sâu";
                            onClicked: dashboard.openDdsWorkspace("")
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Repeater {
                            model: [
                                {
                                    label: "TỔNG SLAVE",
                                    count: dashboard.hosts.length,
                                    color: Theme.eProsimaDarkBlue
                                },
                                {
                                    label: "ONLINE",
                                    count: dashboard.hosts.filter(h => dashboard.hostState(h) === "Online").length,
                                    color: "#237a45"
                                },
                                {
                                    label: "OFFLINE",
                                    count: dashboard.hosts.filter(h => dashboard.hostState(h) === "Offline").length,
                                    color: "#b33030"
                                },
                                {
                                    label: "UNKNOWN",
                                    count: dashboard.hosts.filter(h => dashboard.hostState(h) === "Unknown").length,
                                    color: Theme.grey
                                }
                            ]
                            delegate: Frame {
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                Layout.minimumWidth: 0
                                padding: 14
                                background: Rectangle {
                                    color: "white";
                                    radius: 4;
                                    border.color: Theme.lightGrey
                                }
                                ColumnLayout {
                                    Label {
                                        text: modelData.label;
                                        font.pixelSize: 12;
                                        color: Theme.midGrey
                                    }
                                    Label {
                                        text: modelData.count;
                                        font.pixelSize: 29;
                                        font.bold: true;
                                        color: modelData.color
                                    }
                                }
                            }
                        }
                    }
                    Frame {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        padding: 0
                        background: Rectangle {
                            color: "white";
                            radius: 4;
                            border.color: Theme.lightGrey
                        }
                        ListView {
                            id: fleet
                            anchors.fill: parent
                            anchors.margins: 1
                            clip: true
                            model: dashboard.hosts
                            contentWidth: Math.max(width, 880)
                            readonly property real hostColumn: Math.max(200, (contentWidth - 80) * 0.25)
                            readonly property int stateColumn: 140
                            readonly property int cpuColumn: 72
                            readonly property real ramColumn: Math.max(190, (contentWidth - 80) * 0.24)
                            flickableDirection: Flickable.AutoFlickDirection
                            ScrollBar.vertical: ScrollBar {}
                            ScrollBar.horizontal: ScrollBar {}
                            header: Rectangle {
                                width: fleet.contentWidth;
                                height: 40;
                                color: Theme.eProsimaDarkBlue
                                RowLayout {
                                    anchors.fill: parent;
                                    anchors.leftMargin: 16;
                                    anchors.rightMargin: 16;
                                    spacing: 12
                                    Label {
                                        text: "SLAVE / HOST ID";
                                        color: "white";
                                        Layout.minimumWidth: fleet.hostColumn;
                                        Layout.maximumWidth: fleet.hostColumn
                                    }
                                    Label {
                                        text: "TRẠNG THÁI";
                                        color: "white";
                                        Layout.minimumWidth: fleet.stateColumn;
                                        Layout.maximumWidth: fleet.stateColumn
                                    }
                                    Label {
                                        text: "CPU";
                                        color: "white";
                                        Layout.minimumWidth: fleet.cpuColumn;
                                        Layout.maximumWidth: fleet.cpuColumn
                                    }
                                    Label {
                                        text: "RAM USED / TOTAL";
                                        color: "white";
                                        Layout.minimumWidth: fleet.ramColumn;
                                        Layout.maximumWidth: fleet.ramColumn
                                    }
                                    Label {
                                        text: "LAST SEEN";
                                        color: "white";
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                            delegate: ItemDelegate {
                                id: fleetRow
                                required property int index
                                required property var modelData
                                width: fleet.contentWidth;
                                height: 92
                                leftPadding: 16;
                                rightPadding: 16
                                background: Rectangle {
                                    color: fleetRow.hovered ? "#eaf2f9" : fleetRow.index % 2 ? "#f7f9fb" : "white";
                                    border.color: fleetRow.activeFocus ? Theme.eProsimaDarkBlue : "transparent"
                                }
                                ToolTip.visible: hovered
                                ToolTip.text: modelData.host_id + "\n" + modelData.os
                                onClicked: dashboard.chooseHost(modelData.host_id)
                                contentItem: RowLayout {
                                    spacing: 12
                                    ColumnLayout {
                                        Layout.minimumWidth: fleet.hostColumn;
                                        Layout.maximumWidth: fleet.hostColumn
                                        Label {
                                            text: modelData.hostname;
                                            font.bold: true;
                                            Layout.fillWidth: true;
                                            elide: Text.ElideRight
                                        }
                                        Label {
                                            text: modelData.host_id;
                                            color: Theme.midGrey;
                                            Layout.fillWidth: true;
                                            elide: Text.ElideRight;
                                            font.pixelSize: 12
                                        }
                                        Label {
                                            text: modelData.os;
                                            Layout.fillWidth: true;
                                            elide: Text.ElideRight;
                                            font.pixelSize: 12;
                                            color: Theme.midGrey
                                        }
                                    }
                                    ColumnLayout {
                                        Layout.minimumWidth: fleet.stateColumn;
                                        Layout.maximumWidth: fleet.stateColumn
                                        Label {
                                            text: "● " + dashboard.hostState(modelData);
                                            color: dashboard.stateColor(modelData);
                                            font.bold: true
                                        }
                                        Label {
                                            text: dashboard.freshness(modelData) + " · " + (modelData.alert_count || 0) + " cảnh báo";
                                            font.pixelSize: 12;
                                            Layout.fillWidth: true;
                                            elide: Text.ElideRight;
                                            color: Theme.midGrey
                                        }
                                    }
                                    Label {
                                        text: dashboard.metric(modelData, modelData.metrics?.cpu_percent, false) + " %";
                                        Layout.minimumWidth: fleet.cpuColumn;
                                        Layout.maximumWidth: fleet.cpuColumn
                                    }
                                    Label {
                                        text: dashboard.metric(modelData, modelData.metrics?.ram_used, true) + " / " + dashboard.metric(modelData, modelData.metrics?.ram_total, true);
                                        Layout.minimumWidth: fleet.ramColumn;
                                        Layout.maximumWidth: fleet.ramColumn;
                                        wrapMode: Text.Wrap
                                    }
                                    Label {
                                        text: dashboard.lastSeen(modelData.last_seen);
                                        Layout.fillWidth: true;
                                        wrapMode: Text.Wrap;
                                        font.pixelSize: 13
                                    }
                                }
                            }
                            Label {
                                anchors.centerIn: parent;
                                width: parent.width - 40;
                                horizontalAlignment: Text.AlignHCenter;
                                wrapMode: Text.Wrap;
                                visible: fleet.count === 0;
                                text: "Chưa có Slave. Khởi chạy Agent với cấu hình giám sát để nhận dữ liệu.";
                                color: Theme.midGrey
                            }
                        }
                    }
                }
                HostWorkspace {
                    ui: dashboard;
                    Layout.fillWidth: true;
                    Layout.fillHeight: true
                }
            }
        }
    }
    EventHistoryView {
        anchors.fill: parent;
        visible: dashboard.eventsPage;
        ui: dashboard
    }
}
