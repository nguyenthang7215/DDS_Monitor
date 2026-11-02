// SPDX-License-Identifier: GPL-3.0-or-later
// GRAPH-01 regression: the upstream Domain Graph must emit initialized() (which closes the
// "Loading Graph" dialog) when the domain holds endpoints on more than one host/participant,
// e.g. a Linux Publisher and a Windows Subscriber.
// Run from the repository root:
// QT_QPA_PLATFORM=offscreen QSG_RHI_BACKEND=software qmltestrunner \
//     -input task3/tests/qml/graph -import imports
import QtQuick 6.4
import QtTest 1.2
import "../../../../qml"

Item {
    id: root
    width: 1280
    height: 720

    // Stands in for the C++ Controller context property used by DomainGraphLayout.qml.
    QtObject {
        id: controller
        property string graph: ""
        function get_domain_view_graph(domainEntityId) { return graph }
        function metatraffic_visible() { return false }
        function proxy_visible() { return false }
    }
    QtObject {
        id: panels
        readonly property QtObject openMenuCaller: QtObject { readonly property int domainGraph: 0 }
    }

    function endpoint(kind, alias) {
        return {"kind": kind, "alias": alias, "status": "OK", "discovery_source": "discovery",
                "topic": "100", "metatraffic": false}
    }
    function host(alias, participantId, endpointId, kind) {
        var endpoints = {}
        endpoints[endpointId] = endpoint(kind, alias + " " + kind)
        var participants = {}
        participants[participantId] = {"alias": alias + "_participant", "status": "OK", "app_id": "UNKNOWN_APP",
                                       "app_metadata": "", "dds_vendor": "UNKNOWN", "discovery_source": "discovery",
                                       "metatraffic": false, "endpoints": endpoints}
        return {"alias": alias, "status": "OK", "discovery_source": "discovery", "metatraffic": false,
                "users": {"u": {"alias": "dev", "status": "OK", "discovery_source": "discovery", "metatraffic": false,
                                "processes": {"p": {"alias": alias + "_proc", "pid": "1", "status": "OK",
                                                    "discovery_source": "discovery", "metatraffic": false,
                                                    "participants": participants}}}}}
    }
    function graphJson(hostCount) {
        var hosts = {}
        hosts["10"] = host("linux-master", "1", "32", "datawriter")
        if (hostCount > 1)
            hosts["20"] = host("windows-slave-a", "9", "39", "datareader")
        if (hostCount > 2) {
            // Third host whose single process owns two participants with two endpoints each.
            var third = host("windows-slave-b", "41", "42", "datareader")
            var participants = third["users"]["u"]["processes"]["p"]["participants"]
            participants["41"]["endpoints"]["43"] = endpoint("datawriter", "b writer")
            participants["44"] = JSON.parse(JSON.stringify(participants["41"]))
            participants["44"]["endpoints"] = {"45": endpoint("datareader", "c reader"),
                                               "46": endpoint("datawriter", "c writer")}
            hosts["40"] = third
        }
        return JSON.stringify({"kind": "domain_view", "domain_info": {"domain_id": 0},
                               "topics": {"100": {"alias": "hello_world_topic", "metatraffic": false}},
                               "hosts": hosts})
    }

    Component {
        id: graphComponent
        DomainGraphLayout {
            anchors.fill: parent
            component_id: "graph01"
            domain_entity_id: 1
            domain_id: 0
        }
    }

    TestCase {
        name: "DomainGraphLoading"
        when: windowShown

        function loadAndWait(hostCount) {
            controller.graph = graphJson(hostCount)
            var graph = createTemporaryObject(graphComponent, root)
            verify(graph)
            var spy = createTemporaryObject(Qt.createComponent("SpyHelper.qml"), root, {"target": graph})
            graph.load_model()
            spy.wait(3000)
            return spy.count
        }

        function test_singleParticipantInitializes() {
            verify(loadAndWait(1) >= 1, "one participant: initialized() emitted")
        }

        function test_twoHostsInitialize() {
            verify(loadAndWait(2) >= 1, "two hosts/participants: initialized() must be emitted (GRAPH-01)")
        }

        function test_threeHostsSeveralParticipantsInitialize() {
            controller.graph = graphJson(3)
            var graph = createTemporaryObject(graphComponent, root)
            var spy = createTemporaryObject(Qt.createComponent("SpyHelper.qml"), root, {"target": graph})
            graph.load_model()
            spy.wait(3000)
            // Every endpoint (32, 39, 42, 43, 45, 46) must own a recorded connection to its topic.
            compare(Object.keys(graph.endpoint_topic_connections_).sort(), ["32", "39", "42", "43", "45", "46"])
            compare(graph.pending_connections_.length, 0)
            var image = grabImage(graph)
            image.save("graph-three-hosts.png")
        }

        function test_twoHostsRefreshRepeatedly() {
            controller.graph = graphJson(2)
            var graph = createTemporaryObject(graphComponent, root)
            var spy = createTemporaryObject(Qt.createComponent("SpyHelper.qml"), root, {"target": graph})
            for (var i = 1; i <= 10; ++i) {
                graph.load_model()
                tryVerify(function() { return spy.count >= i }, 3000, "Refresh " + i + " initialized")
            }
        }
    }
}
