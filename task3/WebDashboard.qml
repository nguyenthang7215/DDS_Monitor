// SPDX-License-Identifier: GPL-3.0-or-later
// Qt WebEngine host for the Task 3 dashboard (task3/web). Keeps the public API of Dashboard.qml so
// main.qml, Dashboard.cpp snapshots and UI tests keep driving the same selection state.
import QtQuick
import QtWebEngine
import QtWebChannel
import Theme 1.0

Item {
    id: dashboard
    anchors.fill: parent
    property bool eventsPage: false
    readonly property var hosts: task3Core.hosts
    property string selectedHostId: ""
    property string selectedApplicationId: ""
    property string selectedFlowKey: ""
    property int detailTab: 0
    property int resourceTab: 0
    readonly property var selectedHost: hosts.find(h => h.host_id === selectedHostId) || ({})
    readonly property var selectedApplication: (selectedHost.applications || []).find(a => a.application_id === selectedApplicationId) || ({})
    readonly property bool webReady: view.loadProgress === 100 && !view.loading
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
    function flowKey(f) {
        return String(f.writer_id || "") + "/" + String(f.reader_id || "")
    }

    // Only the selection state and actions are published to the page; data comes from task3WebFeed.
    QtObject {
        id: uiState
        property alias eventsPage: dashboard.eventsPage
        property alias selectedHostId: dashboard.selectedHostId
        property alias selectedApplicationId: dashboard.selectedApplicationId
        property alias selectedFlowKey: dashboard.selectedFlowKey
        property alias detailTab: dashboard.detailTab
        property alias resourceTab: dashboard.resourceTab
        readonly property string fontFamily: Qt.application.font.family
        function chooseHost(id) {
            dashboard.chooseHost(id)
        }
        function chooseApplication(hostId, appId) {
            dashboard.chooseApplication(hostId, appId)
        }
        function openDds(entityId) {
            dashboard.openDdsWorkspace(entityId)
        }
        function reportNetwork(width, height, count, contentHeight) {
            networkMetrics.width = width
            networkMetrics.height = height
            networkMetrics.count = count
            networkMetrics.contentHeight = contentHeight
        }
    }
    // Layout of the web network list, exported for the same UI snapshot checks as the QML ListView.
    QtObject {
        id: networkMetrics
        objectName: "hostNetworkMetrics"
        property real width: 0
        property real height: 0
        property int count: 0
        property real contentHeight: 0
    }
    // "core" is task3::WebFeed: only the fields the page renders, one change signal per property.
    WebChannel {
        id: channel
        Component.onCompleted: registerObjects({"core": task3WebFeed, "ui": uiState})
    }
    Rectangle {
        anchors.fill: parent
        color: Theme.whiteSmoke
    }
    WebEngineView {
        id: view
        objectName: "task3WebView"
        anchors.fill: parent
        backgroundColor: Theme.whiteSmoke
        webChannel: channel
        url: "qrc:/task3/web/index.html"
        settings.localContentCanAccessRemoteUrls: false
        settings.javascriptCanOpenWindows: false
        onContextMenuRequested: request => request.accepted = true
        onJavaScriptConsoleMessage: (level, message, lineNumber, sourceID) => {
            if (level !== WebEngineView.InfoMessageLevel)
                console.warn("task3 web:", message, sourceID + ":" + lineNumber)
        }
        onNavigationRequested: request => {
            // The dashboard is a single local page; never leave it.
            if (!request.url.toString().startsWith("qrc:/task3/web/"))
                request.action = WebEngineNavigationRequest.IgnoreRequest
        }
    }
}
