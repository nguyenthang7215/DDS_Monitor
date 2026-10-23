import QtQuick
import QtTest
import "../.." as Task3

TestCase {
    name: "NetworkScroll"
    width: 1280
    height: 800
    when: windowShown
    QtObject {
        id: task3Core
        property var entities: []
    }
    QtObject {
        id: ui
        property var selectedHost: ({})
        property var selectedApplication: ({})
        property string selectedHostId: "host-a"
        property string selectedApplicationId: ""
        property int detailTab: 0
        property int resourceTab: 1
        function fresh(host) { return true }
        function freshness(host) { return "Fresh" }
        function hostState(host) { return "Online" }
        function stateColor(host) { return "green" }
        function lastSeen(value) { return "now" }
        function metric(host, value, bytes) { return value == null ? "N/A" : String(value) }
    }
    Task3.HostWorkspace {
        id: workspace
        anchors.fill: parent
        ui: ui
    }
    function rows(names, value, valid) {
        return names.map(function(name) {
            return {name: String(name), rx_bps: value, tx_bps: value * 2,
                rx_bytes: value * 100, tx_bytes: value * 200, rate_valid: valid}
        })
    }
    function publish(values) {
        ui.selectedHost = {hostname: ui.selectedHostId, metrics: {interfaces: values}}
    }
    function init() {
        failOnWarning(/.*/)
        ui.selectedHostId = "host-a"
        const names = []
        for (let i = 40; i >= 1; --i)
            names.push(i)
        publish(rows(names, 10, true))
        wait(100)
    }
    function test_refresh_preserves_scroll_and_delegates() {
        const view = findChild(workspace, "hostNetworkMetrics")
        verify(view !== null)
        compare(view.count, 40)
        compare(view.model.get(1).modelData.name, "2")
        view.positionViewAtIndex(20, ListView.Beginning)
        wait(50)
        const oldY = view.contentY
        verify(oldY > 0)
        const delegate = view.itemAtIndex(20)
        const names = []
        for (let i = 1; i <= 40; ++i)
            names.push(i)
        for (let update = 1; update <= 12; ++update) {
            publish(rows(names, update * 20, update !== 12))
            wait(20)
            compare(view.contentY, oldY)
            compare(view.itemAtIndex(20), delegate)
        }
        compare(view.model.get(20).modelData.rx_bps, 240)
        compare(view.model.get(20).modelData.tx_bytes, 48000)
        compare(view.model.get(20).modelData.rate_valid, false)
    }
    function test_membership_and_host_change() {
        const view = findChild(workspace, "hostNetworkMetrics")
        view.positionViewAtIndex(20, ListView.Beginning)
        wait(50)
        const oldY = view.contentY
        const anchor = view.model.get(view.indexAt(1, oldY + 1)).interfaceKey
        const names = []
        for (let i = 0; i < 40; ++i)
            names.push(i)
        // Same count, different membership: add 0, remove 40.
        publish(rows(names, 30, true))
        wait(50)
        compare(view.count, 40)
        compare(view.model.get(0).modelData.name, "0")
        compare(view.model.get(39).modelData.name, "39")
        verify(view.contentY > 0)
        verify(Math.abs(view.contentY - oldY) < 200)
        compare(view.model.get(view.indexAt(1, view.contentY + 1)).interfaceKey, anchor)
        publish(rows([10, 2, 1], 40, true))
        wait(50)
        compare(view.count, 3)
        compare(view.model.get(1).modelData.name, "2")
        ui.selectedHostId = "host-b"
        publish(rows([88, 7], 50, false))
        wait(50)
        compare(view.count, 2)
        compare(view.model.get(0).modelData.name, "7")
        compare(view.model.get(1).modelData.name, "88")
        compare(view.contentY, view.originY)
        ui.selectedHostId = "host-c"
        publish(rows([88, 7], 60, true))
        wait(50)
        compare(view.count, 2)
        compare(view.model.get(0).modelData.rx_bps, 60)
        compare(view.model.get(0).modelData.rate_valid, true)
        publish([])
        wait(50)
        compare(view.count, 0)
    }
}
