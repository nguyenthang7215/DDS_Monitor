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
    visible: true
    width: 1600
    height: 900
    title: qsTr("DDS Monitor")

    signal startMonitoring

    // This property holds the current number of initialized monitors
    property int monitors: 0
    property bool task3Ready: false
    property bool task3Page: true
    property bool legacyStarted: false
    property int task3UiSteps: 0
    function task3Exercise(step) {
        task3UiSteps = step
        task3Page = step % 5 !== 4
        if (task3Loader.item) task3Loader.item.testTab = step % 4
        width = step % 2 === 0 ? 1280 : 900
        height = step % 2 === 0 ? 820 : 650
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

    header: MonitorToolBar {
        id: toolBar
        visible: !mainApplicationView.task3Page && isVisible
        onChangeChartboxLayout: function(chartsPerRow) { monitorMenuBar.changeChartboxLayoutViewMenu(chartsPerRow) }
    }

    Panels {
        id: panels
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
    footer: TabBar {
        currentIndex: mainApplicationView.task3Page ? 0 : 1
        TabButton { text: "Task 3 · Máy / Ứng dụng / DDS"; onClicked: mainApplicationView.task3Page = true }
        TabButton { text: "DDS nâng cao · Topology / Statistics"; onClicked: mainApplicationView.task3Page = false }
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
