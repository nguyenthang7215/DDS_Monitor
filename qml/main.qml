// Copyright 2021 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// This file is part of eProsima DDS Monitor.
//
// eProsima DDS Monitor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// eProsima DDS Monitor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with eProsima DDS Monitor. If not, see <https://www.gnu.org/licenses/>.

import QtQuick 6.4
import QtQuick.Layouts 6.4
import QtQuick.Window 6.4
import QtQuick.Controls 6.4
import QtQuick.Dialogs 6.4

import Theme 1.0

ApplicationWindow {
    id: mainApplicationView
    objectName: "ddsMonitorWindow"
    visible: true
    width: 1600
    height: 900
    title: qsTr("DDS Monitor")

    signal startMonitoring

    // This property holds the current number of initialized monitors
    property int monitors: 0
    property bool task3Ready: false
    property int workspaceIndex: 0
    readonly property bool task3Page: workspaceIndex !== 1
    property bool legacyStarted: false
    property int task3UiSteps: 0
    property bool task3ResizeExercise: true
    readonly property bool task3DashboardLoaded: task3Loader.status === Loader.Ready
    readonly property string task3SelectedHost: task3Loader.item ? task3Loader.item.selectedHostId : ""
    readonly property string task3SelectedApplication: task3Loader.item ? task3Loader.item.selectedApplicationId : ""
    readonly property int task3DetailTab: task3Loader.item ? task3Loader.item.detailTab : 0
    readonly property int task3ResourceTab: task3Loader.item ? task3Loader.item.resourceTab : 0
    property string task3ExpectedHost: ""
    property string task3ExpectedApplication: ""
    property bool task3SelectionRetained: true
    function task3Exercise(step) {
        task3UiSteps = step
        const phase = step % 8
        workspaceIndex = phase === 4 ? 1 : phase === 5 ? 2 : 0
        const page = task3Loader.item
        if (page) {
            if (phase === 0 && page.hosts.length) page.chooseHost(page.hosts[0].host_id)
            if (phase === 1 && page.selectedHost.applications?.length) {
                page.chooseApplication(page.selectedHostId, page.selectedHost.applications[0].application_id)
                task3ExpectedHost = page.selectedHostId
                task3ExpectedApplication = page.selectedApplicationId
            }
            if (phase <= 3) page.detailTab = phase
            if (phase === 0) page.resourceTab = 0
            if (phase === 7) { page.detailTab = 0; page.resourceTab = 1 }
            if (phase === 2) {
                const flow = task3Core.flows.find(f => f.source_host === page.selectedHostId &&
                    (!page.selectedApplicationId || f.source_application === page.selectedApplicationId))
                if (flow) page.selectedFlowKey = page.flowKey(flow)
            }
            if (phase === 6 && task3ExpectedHost)
                task3SelectionRetained = task3SelectionRetained && page.selectedHostId === task3ExpectedHost &&
                    page.selectedApplicationId === task3ExpectedApplication
        }
        if (task3ResizeExercise) {
            width = step % 2 === 0 ? 1280 : 900
            height = step % 2 === 0 ? 820 : 650
        }
    }

    onStartMonitoring: {
        legacyStarted = true
        toolBar.isVisible = true
        dialogInitMonitor.open()
    }

    MainStartView {
        id: mainStartView
        visible: !mainApplicationView.task3Page && !mainApplicationView.legacyStarted
    }

    menuBar: MonitorMenuBar {
        id: monitorMenuBar
        visible: !mainApplicationView.task3Page && mainApplicationView.legacyStarted
        onToolBarHidden: toolBar.isVisible = !toolBar.isVisible
        onDispDataButtonHidden: toolBar.isVisibleDispData = !toolBar.isVisibleDispData
        onDispDynDataButtonHidden: toolBar.isVisibleDispDynData = !toolBar.isVisibleDispDynData
        onCreateAlertButtonHidden: toolBar.isVisibleCreateAlert = !toolBar.isVisibleCreateAlert
        onRefreshButtonHidden: toolBar.isVisibleRefresh = !toolBar.isVisibleRefresh
        onClearLogButtonHidden: toolBar.isVisibleClearLog = !toolBar.isVisibleClearLog
        onDashboardLayoutButtonHidden: toolBar.isVisibleDashboardLayout = !toolBar.isVisibleDashboardLayout
        onClearIssuesButtonHidden: toolBar.isVisibleClearIssues = !toolBar.isVisibleClearIssues
        onLeftSidebarHidden: panels.openCloseLeftSideBar()
        onChangeChartboxLayout: function(chartsPerRow) {
            panels.changeChartboxLayout(chartsPerRow)
            toolBar.changeChartboxLayoutIcon(chartsPerRow)
        }
        onSaveAllCSV: {
            panels.saveAllCSV()
        }
        onExplorerDDSEntitiesChanged: function(status) { panels.changeExplorerDDSEntities(status) }
        onExplorerPhysicalChanged: function(status) { panels.changeExplorerPhysical(status) }
        onExplorerLogicalChanged: function(status) { panels.changeExplorerLogical(status) }
        onExplorerEntityInfoChanged: function(status) { panels.changeExplorerEntityInfo(status) }
    }

    header: ColumnLayout {
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            spacing: 16
            Label { text: "DDS Monitor"; font.pixelSize: 23; font.bold: true; color: Theme.eProsimaDarkBlue; leftPadding: 20 }
            Label {
                objectName: "monitorStatusLabel"
                Layout.fillWidth: true
                // Keep the technical topic in diagnostics, not in the product heading.
                text: {
                    if (!mainApplicationView.task3Ready) return "Đang khởi tạo giám sát…"
                    const status = task3Core.status
                    if (!status.startsWith("DDS telemetry domain")) return status
                    const diagnostics = status.indexOf(" | ")
                    return "Giám sát tài nguyên & truyền thông DDS" + (diagnostics < 0 ? "" : status.slice(diagnostics))
                }
                color: Theme.midGrey
                elide: Text.ElideRight
                font.pixelSize: 13
                ToolTip.visible: statusHover.hovered
                ToolTip.text: text
                HoverHandler { id: statusHover }
            }
            Label { text: "Master"; color: Theme.eProsimaDarkBlue; rightPadding: 20 }
        }
        TabBar {
            id: primaryTabs
            font.pixelSize: 15
            implicitHeight: 42
            Layout.fillWidth: true
            currentIndex: mainApplicationView.workspaceIndex
            onCurrentIndexChanged: mainApplicationView.workspaceIndex = currentIndex
            Connections {
                target: mainApplicationView
                function onWorkspaceIndexChanged() { primaryTabs.currentIndex = mainApplicationView.workspaceIndex }
            }
            TabButton {
                id: systemTab
                text: "Giám sát hệ thống"
                contentItem: Label { text: systemTab.text; horizontalAlignment: Text.AlignHCenter; color: systemTab.checked ? Theme.eProsimaDarkBlue : Theme.midGrey; font.bold: systemTab.checked }
            }
            TabButton {
                id: ddsTab
                text: "DDS chuyên sâu"
                contentItem: Label { text: ddsTab.text; horizontalAlignment: Text.AlignHCenter; color: ddsTab.checked ? Theme.eProsimaDarkBlue : Theme.midGrey; font.bold: ddsTab.checked }
            }
            TabButton {
                id: historyTab
                text: "Cảnh báo & lịch sử"
                contentItem: Label { text: historyTab.text; horizontalAlignment: Text.AlignHCenter; color: historyTab.checked ? Theme.eProsimaDarkBlue : Theme.midGrey; font.bold: historyTab.checked }
            }
        }
        MonitorToolBar {
            id: toolBar
            Layout.fillWidth: true
            visible: mainApplicationView.workspaceIndex === 1 && isVisible
            onChangeChartboxLayout: function(chartsPerRow) { monitorMenuBar.changeChartboxLayoutViewMenu(chartsPerRow) }
        }
    }

    Panels {
        id: panels
        objectName: "ddsOriginalWorkspace"
        visible: !mainApplicationView.task3Page && mainApplicationView.legacyStarted
        onExplorerDDSEntitiesChanged: function(status) { monitorMenuBar.changeExplorerDDSEntities(status) }
        onExplorerPhysicalChanged: function(status) { monitorMenuBar.changeExplorerPhysical(status) }
        onExplorerLogicalChanged: function(status) { monitorMenuBar.changeExplorerLogical(status) }
        onExplorerEntityInfoChanged: function(status) { monitorMenuBar.changeExplorerEntityInfo(status) }
    }

    InitMonitorDialog {
        id: dialogInitMonitor
    }

    Loader {
        id: task3Loader
        anchors.fill: parent
        active: mainApplicationView.task3Ready
        visible: mainApplicationView.task3Page
        source: "qrc:/task3/Dashboard.qml"
    }
    Binding {
        target: task3Loader.item
        property: "eventsPage"
        value: mainApplicationView.workspaceIndex === 2
        when: task3Loader.item !== null
    }
    Connections {
        target: task3Loader.item
        function onOpenDdsWorkspace(entityId) {
            mainApplicationView.workspaceIndex = 1
            if (entityId) controller.endpoint_click(entityId)
        }
    }

    InitDSMonitorDialog {
        id: dialogDSInitMonitor
    }

    InitMonitorWithProfileDialog {
        id: initMonitorWithProfileDialog
    }

    DumpFileDialog {
        id: dumpDialog
        clear: false
    }

    DumpFileDialog {
        id: dumpDialogClear
        clear: true
    }

    HistoricDataKindDialog {
        id: dataKindDialog
        onCreateChart: function(dataKind) {
            panels.createHistoricChart(dataKind)
        }
    }

    DynamicDataKindDialog {
        id: dynamicDataKindDialog
        onCreateChart: function(dataKind, timeWindowSeconds, updatePeriod, maxPoints) {
            panels.createDynamicChart(dataKind, timeWindowSeconds, updatePeriod, maxPoints)
        }
    }

    AlertDialog {
        id: alertDialog
        onCreateAlert: function(alert_name, domain_name, host_name, user_name, topic_name, alert_type,
                                t_between_triggers, threshold, alert_timeout, script_path) {
            panels.createAlert(alert_name, domain_name, host_name, user_name, topic_name, alert_type,
                            t_between_triggers, threshold, alert_timeout, script_path)
        }
    }

    AlertsSettingsDialog {
        id: alertsSettingsDialog
        onSetAlertsPollingTime: function(pollingTime) {
            controller.set_alerts_polling_time(pollingTime)
        }
    }

    ScheduleClearDialog {
        id: scheduleClear
    }

    AboutDialog {
        id: aboutDialog
    }

    ErrorDialog {
        id: errorDialog
    }

    SeriesSetMaxPointsDialog {
        id: seriesSetMaxPointsDialog
    }

    LoadXmlProfilesDialog {
        id: loadXmlProfilesDialog
    }

    // Creates a connection between Controller::error(QString, int) signal and ErrorDialog
    Connections {
        target: controller
        function onError(errorMsg, errorType) {
            errorDialog.errorText = errorMsg
            errorDialog.errorType = errorType
            errorDialog.open()
        }
    }

    // Creates a connection between Controller::monitorInitialized() signal and qml
    Connections {
        target: controller
        function onMonitorInitialized() {
            monitors++
            mainApplicationView.legacyStarted = true
            toolBar.isVisible = true
        }
    }
}
