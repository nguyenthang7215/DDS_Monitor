// SPDX-License-Identifier: GPL-3.0-or-later
// Stand-in for qrc:///qtwebchannel/qwebchannel.js so task3/web runs in a plain browser.
// `core` mimics task3::WebFeed (one change signal per property, Participants trimmed) and `ui`
// mimics the uiState object of WebDashboard.qml. Like the real channel, a property written from JS
// updates the local cache at once while change signals and method calls arrive asynchronously.
"use strict";
(function () {
    // Headless --dump-dom produces no frames, so requestAnimationFrame (used by app.js to batch
    // renders) would never fire. Harness only: run the callback once, from a frame or a timer.
    const nativeFrame = window.requestAnimationFrame.bind(window);
    window.requestAnimationFrame = callback => {
        let done = false;
        const once = time => { if (!done) { done = true; callback(time); } };
        setTimeout(() => once(performance.now()), 20);
        return nativeFrame(once);
    };
    const later = fn => setTimeout(fn, 0);
    function signal() {
        const callbacks = [];
        return {
            connect: fn => callbacks.push(fn),
            disconnect: fn => { const i = callbacks.indexOf(fn); if (i >= 0) callbacks.splice(i, 1); },
            emit: () => callbacks.slice().forEach(fn => fn()),
            get count() { return callbacks.length; }
        };
    }
    function qobject(values) {
        const object = {}, store = Object.assign({}, values);
        for (const name of Object.keys(values)) {
            object[name + "Changed"] = signal();
            Object.defineProperty(object, name, {
                enumerable: true,
                get: () => store[name],
                set: value => { store[name] = value; later(() => object[name + "Changed"].emit()); }
            });
        }
        object.__store = store;
        // Change from the "C++/QML side": value and notification, like a property update message.
        object.__push = (name, value) => { store[name] = value; later(() => object[name + "Changed"].emit()); };
        return object;
    }
    const pick = (row, fields) => fields.reduce((out, f) => (f in row ? Object.assign(out, { [f]: row[f] }) : out), {});
    const participantsOf = entities => (entities || []).filter(e => e.kind === "Participant")
        .map(e => pick(e, ["id", "guid", "pid", "host_id", "application_id", "mapping_state"]));

    const calls = [];
    const core = qobject({ hosts: [], events: [], hasMoreEvents: false, flows: [], participants: [] });
    core.loadOlderEvents = () => { calls.push(["loadOlderEvents"]); };
    // hasMoreEvents shares eventsChanged with events in WebFeed.
    delete core.hasMoreEventsChanged;

    const ui = qobject({ eventsPage: false, selectedHostId: "", selectedApplicationId: "", selectedFlowKey: "",
        detailTab: 0, resourceTab: 0, fontFamily: "DejaVu Sans" }); // Qt's default Linux UI font
    ui.chooseHost = id => later(() => {
        calls.push(["chooseHost", id]);
        if (id !== ui.__store.selectedHostId) {
            ui.__push("selectedApplicationId", "");
            ui.__push("selectedFlowKey", "");
        }
        ui.__push("selectedHostId", id);
        ui.__push("detailTab", 0);
    });
    ui.chooseApplication = (hostId, appId) => later(() => {
        calls.push(["chooseApplication", hostId, appId]);
        if (hostId !== ui.__store.selectedHostId)
            ui.__push("selectedFlowKey", "");
        ui.__push("selectedHostId", hostId);
        ui.__push("selectedApplicationId", appId);
        ui.__push("detailTab", 1);
    });
    ui.openDds = entityId => later(() => calls.push(["openDds", entityId]));
    ui.reportNetwork = (width, height, count, contentHeight) =>
        later(() => calls.push(["reportNetwork", width, height, count, contentHeight]));

    // Loads a recorded MonitoringCore snapshot (Dashboard.cpp TASK3_SNAPSHOT_PATH format).
    function load(snapshot) {
        core.__push("hosts", snapshot.hosts || []);
        core.__push("flows", snapshot.flows || []);
        core.__push("participants", participantsOf(snapshot.entities));
        core.__store.hasMoreEvents = !!snapshot.hasMoreEvents;
        core.__push("events", snapshot.events || []);
    }
    window.task3Mock = { core, ui, calls, load, participantsOf };
    window.qt = { webChannelTransport: {} };
    window.QWebChannel = function (transport, ready) {
        later(() => ready({ objects: { core, ui } }));
    };
})();
