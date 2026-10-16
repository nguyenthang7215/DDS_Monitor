// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

Page {
    id: dashboard
    anchors.fill: parent
    title: "Task 3 · DDS Master–Slave Monitor"
    property string selectedHostId: ""
    property var selectedHost: ({})
    property string search: ""
    property int testTab: 0
    Component.onCompleted: testTab = task3StartupTab
    property string selectedApplicationId: ""
    property var selectedParticipants: task3Core.entities.filter(e => e.kind === "Participant" && e.host_id === selectedHostId && e.application_id === selectedApplicationId)
    onTestTabChanged: tabs.currentIndex = testTab
    function number(v) { return v === undefined || v === null ? "—" : Number(v).toFixed(1) }
    function mb(v) { return v === undefined || v === null ? "—" : (Number(v) / 1048576).toFixed(1) + " MiB" }
    function offset(v) { return v === undefined || v === null ? "Unavailable" : Number(v).toExponential(3) + " s" }
    function refreshSelection() {
        selectedHost = ({})
        if (selectedHostId === "" && task3Core.hosts.length > 0) selectedHostId = task3Core.hosts[0].host_id
        for (let h of task3Core.hosts)
            if (h.host_id === selectedHostId) {
                selectedHost = h
                if (!h.applications.some(a => a.application_id === selectedApplicationId))
                    selectedApplicationId = h.applications.length ? h.applications[0].application_id : ""
                return
            }
    }
    Connections {
        target: task3Core
        function onChanged() { dashboard.refreshSelection() }
    }
    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            Label { text: "DDS MASTER–SLAVE"; font.bold: true; leftPadding: 16 }
            Label { text: task3Core.status; Layout.fillWidth: true; elide: Text.ElideRight }
            Label { text: "Hosts: " + task3Core.hosts.length + "   ·   DDS: " + task3Core.entities.length; rightPadding: 16 }
        }
    }
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        TabBar {
            id: tabs
            Layout.fillWidth: true
            TabButton { text: "Overview & host detail" }
            TabButton { text: "DDS entities & data flow" }
            TabButton { text: "Alerts & event history" }
            TabButton { text: "Host / Application Topology" }
        }
        StackLayout {
            currentIndex: tabs.currentIndex
            Layout.fillWidth: true
            Layout.fillHeight: true
            SplitView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                ListView {
                    SplitView.preferredWidth: 390
                    clip: true
                    model: task3Core.hosts
                    spacing: 6
                    ScrollBar.vertical: ScrollBar {}
                    delegate: ItemDelegate {
                        required property var modelData
                        width: ListView.view.width
                        height: 110
                        highlighted: dashboard.selectedHostId === modelData.host_id
                        onClicked: { dashboard.selectedHostId = modelData.host_id; dashboard.refreshSelection() }
                        contentItem: Column {
                            spacing: 4
                            Label { text: (modelData.online ? "● " : "○ ") + modelData.hostname + " · " + modelData.host_id; color: modelData.metrics_state === "unknown" ? "#697078" : (modelData.online ? "#22863a" : "#c0392b"); width: parent.width; elide: Text.ElideRight }
                            Label { text: modelData.os; width: parent.width; elide: Text.ElideRight }
                            Label { text: "CPU " + dashboard.number(modelData.metrics.cpu_percent) + "%  · RAM " + dashboard.mb(modelData.metrics.ram_used) }
                            Label { text: "Apps " + modelData.applications.length + " · Alerts " + modelData.alert_count + " · " + modelData.metrics_state; font.pixelSize: 11 }
                        }
                    }
                    Label { anchors.centerIn: parent; visible: task3Core.hosts.length === 0; text: "No telemetry received. Start a configured agent." }
                }
                ScrollView {
                    SplitView.fillWidth: true
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 12
                        Label { text: dashboard.selectedHost.hostname || "Select a Slave"; font.pixelSize: 24 }
                        Label { text: dashboard.selectedHost.host_state || "Host physical state is not inferred from DDS"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                        Label { text: "Address: " + (dashboard.selectedHost.address || "—") + "   OS: " + (dashboard.selectedHost.os || "—") }
                        Label { text: "Last seen: " + (dashboard.selectedHost.last_seen || "—") + " · Metrics: " + (dashboard.selectedHost.metrics_state || "unknown") }
                        ChartView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 170
                            title: "CPU usage (%) · last 120 samples"
                            legend.visible: false
                            ValueAxis { id: cpuX; min: 0; max: 120; labelsVisible: false; titleVisible: false; tickCount: 5 }
                            ValueAxis { id: cpuY; min: 0; max: 100; titleText: "%" }
                            LineSeries { id: cpuSeries; axisX: cpuX; axisY: cpuY }
                            Connections {
                                target: dashboard
                                function onSelectedHostChanged() {
                                    cpuSeries.clear()
                                    let points = dashboard.selectedHost.cpu_history || []
                                    for (let i = 0; i < points.length; ++i) cpuSeries.append(i, points[i].value)
                                }
                            }
                        }
                        GroupBox {
                            title: "Real host metrics"
                            Layout.fillWidth: true
                            ColumnLayout {
                                anchors.fill: parent
                                Label { text: "CPU: " + dashboard.number(dashboard.selectedHost.metrics?.cpu_percent) + "%" }
                                Label { text: "RAM used / total: " + dashboard.mb(dashboard.selectedHost.metrics?.ram_used) + " / " + dashboard.mb(dashboard.selectedHost.metrics?.ram_total) }
                                Label { text: "Disk used / total: " + dashboard.mb(dashboard.selectedHost.metrics?.disk_used) + " / " + dashboard.mb(dashboard.selectedHost.metrics?.disk_total) }
                                Label { text: "Network RX / TX: " + dashboard.number(dashboard.selectedHost.metrics?.network_rx_bps) + " / " + dashboard.number(dashboard.selectedHost.metrics?.network_tx_bps) + " B/s (aggregate)" }
                                Repeater {
                                    model: dashboard.selectedHost.metrics?.interfaces || []
                                    delegate: Label {
                                        required property var modelData
                                        text: modelData.name + " · RX " + dashboard.number(modelData.rx_bps) + " / TX " + dashboard.number(modelData.tx_bps) + " B/s" + (modelData.rate_valid ? "" : " · rate unavailable")
                                    }
                                }
                                Repeater {
                                    model: dashboard.selectedHost.metrics?.disks || []
                                    delegate: Label {
                                        required property var modelData
                                        text: modelData.path + " · used " + dashboard.mb(modelData.used) + " / " + dashboard.mb(modelData.total) + (modelData.valid ? "" : " · unavailable")
                                    }
                                }
                            }
                        }
                        GroupBox {
                            title: "Time synchronization"
                            Layout.fillWidth: true
                            ColumnLayout {
                                Label { text: "State: " + (dashboard.selectedHost.time_sync?.state || "unknown") + " · Source: " + (dashboard.selectedHost.time_sync?.source || "—") }
                                Label { text: "Offset: " + dashboard.offset(dashboard.selectedHost.time_sync?.offset_seconds) }
                                Label { text: "Service: " + (dashboard.selectedHost.time_sync?.service_running ?? "Unknown") + " · Clock synchronized: " + (dashboard.selectedHost.time_sync?.clock_synced ?? "Unknown"); wrapMode: Text.Wrap; Layout.fillWidth: true }
                                Label { text: dashboard.selectedHost.time_sync?.detail || ""; wrapMode: Text.Wrap; Layout.maximumWidth: 650 }
                            }
                        }
                        Label { text: "Applications / processes / services"; font.bold: true }
                        Repeater {
                            model: dashboard.selectedHost.applications || []
                            delegate: Button {
                                required property var modelData
                                text: modelData.application_id + " · process " + modelData.state + " · instances " + (modelData.instances?.length || (modelData.pid ? 1 : 0)) + " · service " + modelData.service_state
                                onClicked: dashboard.selectedApplicationId = modelData.application_id
                                Layout.fillWidth: true
                            }
                        }
                        Label { text: "Selected application: " + (dashboard.selectedApplicationId || "—"); font.bold: true }
                        Repeater {
                            model: (dashboard.selectedHost.applications || []).filter(a => a.application_id === dashboard.selectedApplicationId)
                            delegate: ColumnLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                Label { text: "Process: " + modelData.state + " · Service: " + (modelData.service || "not configured") + " / " + modelData.service_state; wrapMode: Text.Wrap; Layout.fillWidth: true }
                                Repeater {
                                    model: modelData.instances || []
                                    delegate: Label {
                                        required property var modelData
                                        text: "PID " + modelData.pid + " · instance " + modelData.instance_id + " · " + (modelData.binding_source || "Unmapped")
                                        wrapMode: Text.Wrap; Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                        Repeater {
                            model: dashboard.selectedParticipants
                            delegate: Label {
                                required property var modelData
                                text: "Participant " + modelData.guid + " · PID " + modelData.pid + " · instance " + (modelData.instance_id || "—")
                                wrapMode: Text.Wrap; Layout.fillWidth: true
                            }
                        }
                        Label { text: "Advanced DDS views, topology and historical/dynamic charts: choose DDS nâng cao in the bottom navigation."; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }
                }
            }
            ColumnLayout {
                TextField { Layout.fillWidth: true; placeholderText: "Filter entity name, topic, GUID or host"; onTextChanged: dashboard.search = text.toLowerCase() }
                Label { text: "Live Statistics Backend · raw rolling window 120 s · matching is not inferred from a shared Topic"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: task3Core.entities
                    clip: true
                    spacing: 6
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Frame {
                        id: ddsFrame
                        required property var modelData
                        property bool expanded: false
                        width: ListView.view.width
                        visible: dashboard.search === "" || JSON.stringify(modelData).toLowerCase().indexOf(dashboard.search) >= 0
                        height: visible ? ddsColumn.implicitHeight + 24 : 0
                        ColumnLayout {
                            id: ddsColumn
                            width: parent.width
                            Label { text: modelData.kind + " #" + modelData.id + " · " + (modelData.name || modelData.topic || "") + " · " + (modelData.alive ? "active" : "inactive"); font.bold: true }
                            Label { text: "GUID " + (modelData.guid || "—") + " · Host " + (modelData.hostname || "unknown") + " · PID " + (modelData.pid || "unknown"); wrapMode: Text.Wrap; Layout.fillWidth: true }
                            Label { text: "Mapping: " + (modelData.mapping_state || "unknown") + " " + (modelData.application_id || "") + " · Matching: " + (modelData.match_state || "unknown"); wrapMode: Text.Wrap; Layout.fillWidth: true }
                            Label { visible: modelData.kind === "Writer" || modelData.kind === "Reader"; text: "Throughput: " + dashboard.number(modelData.throughput_bps) + " B/s · " + (modelData.data_state || "no samples") + " · samples " + (modelData.samples?.length || 0) }
                            Repeater {
                                model: modelData.links || []
                                delegate: Label {
                                    required property var modelData
                                    text: "→ Reader #" + modelData.reader_id + " · " + modelData.relation + " · latency " + dashboard.number(modelData.latency_us) + " µs"
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }
                            }
                            Button { text: ddsFrame.expanded ? "Hide details / QoS" : "Details / QoS"; onClicked: ddsFrame.expanded = !ddsFrame.expanded }
                            Label { visible: ddsFrame.expanded; text: modelData.details || ""; font.family: "monospace"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                        }
                    }
                }
            }
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: task3Core.events
                clip: true
                spacing: 6
                ScrollBar.vertical: ScrollBar {}
                delegate: Frame {
                    required property var modelData
                    width: ListView.view.width
                    height: eventColumn.implicitHeight + 24
                    Column {
                        id: eventColumn
                        width: parent.width
                        Label { text: modelData.state + " · " + modelData.kind + " · " + modelData.source; color: modelData.state === "Active" ? "#b03a2e" : "#22863a"; font.bold: true }
                        Label { text: modelData.time + " · " + modelData.severity }
                        Label { text: modelData.message; wrapMode: Text.Wrap; width: parent.width }
                    }
                }
                Label { anchors.centerIn: parent; visible: task3Core.events.length === 0; text: "No alert transitions recorded." }
            }
            ListView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 10
                model: task3Core.flows
                ScrollBar.vertical: ScrollBar {}
                delegate: Frame {
                    required property var modelData
                    width: ListView.view.width
                    height: route.implicitHeight + 24
                    ColumnLayout {
                        id: route; width: parent.width
                        Label { text: modelData.match_state + " · " + (modelData.flow_state || "Unknown"); font.bold: true; wrapMode: Text.Wrap; Layout.fillWidth: true }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: (modelData.source_host || "Unmapped host") + " / " + (modelData.source_application || "Unmapped app") + "\nWriter #" + modelData.writer_id; wrapMode: Text.Wrap; Layout.preferredWidth: route.width * 0.35 }
                            Label { text: modelData.confirmed ? "→" : "|" }
                            Label { text: modelData.topic || "Unknown Topic"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                            Label { text: modelData.confirmed ? "→" : "|" }
                            Label { text: "Reader #" + (modelData.reader_id || "Unknown") + "\n" + (modelData.target_application || "Unmapped app") + " / " + (modelData.target_host || "Unmapped host"); wrapMode: Text.Wrap; Layout.preferredWidth: route.width * 0.35 }
                        }
                        Label { text: "Throughput " + dashboard.number(modelData.throughput_bps) + " B/s · recorded latency " + dashboard.number(modelData.latency_us) + " µs"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }
                }
                Label { anchors.centerIn: parent; visible: task3Core.flows.length === 0; text: "No Writer discovered yet. Unconfirmed relationships have no directional arrows."; width: parent.width; wrapMode: Text.Wrap }
            }
        }
    }
}
