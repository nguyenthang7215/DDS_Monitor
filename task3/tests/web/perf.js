// SPDX-License-Identifier: GPL-3.0-or-later
// Wall-clock cost of one full render of task3/web (run_web_tests.py, ?mode=perf). Runs while the
// page loads, outside the browser's virtual time, and calls the app.js render functions directly.
// Kept outside selftest.js so the app.js globals core/ui are not shadowed (the page CSP forbids eval).
"use strict";
function task3Measure() {
    const fixtures = window.TASK3_FIXTURES;
    const clone = value => JSON.parse(JSON.stringify(value));
    core = window.task3Mock.core; // normally set by the QWebChannel callback in app.js
    ui = window.task3Mock.ui;
    const median = fn => {
        const samples = [];
        for (let i = 0; i < 7; ++i) {
            html.clear(); // setHtml skips identical markup; force the DOM write being measured
            const start = performance.now();
            fn();
            void document.body.offsetHeight; // include style and layout
            samples.push(performance.now() - start);
        }
        return Math.round(samples.sort((a, b) => a - b)[3] * 10) / 10;
    };
    core.__store.hosts = Array.from({ length: 512 }, (_, i) => Object.assign(clone(fixtures.mapped.hosts[0]), {
        host_id: "slave-" + String(i).padStart(3, "0"), hostname: "slave-" + i }));
    core.__store.events = Array.from({ length: 500 }, (_, i) =>
        Object.assign(clone(fixtures.recovered.events[i % 2]), { alert_id: "a" + i }));
    core.__store.flows = fixtures.mapped.flows;
    core.__store.participants = window.task3Mock.participantsOf(fixtures.mapped.entities);
    const result = { hosts: core.__store.hosts.length, interfaces_per_host: fixtures.mapped.hosts[0].metrics.interfaces.length,
        events: core.__store.events.length };
    show("overview", true);
    result.overview_ms = median(() => { renderLeft(state()); renderOverview(state()); });
    ui.__store.selectedHostId = core.__store.hosts[0].host_id;
    ui.__store.resourceTab = 1;
    show("overview", false);
    show("hostWorkspace", true);
    show("pageResources", true);
    result.host_resources_network_ms = median(() => { renderLeft(state()); renderHostHeader(state()); renderResources(state()); });
    show("systemView", false);
    show("eventsView", true);
    result.events_ms = median(() => renderEvents());
    // What QWebChannel carries for the recorded snapshot: MonitoringCore (all six properties share
    // one NOTIFY signal, so every update resends everything) versus WebFeed.
    const snapshot = fixtures.recovered;
    const size = value => new TextEncoder().encode(JSON.stringify(value)).length;
    result.payload_monitoring_core_every_update_bytes = size({ hosts: snapshot.hosts, events: snapshot.events,
        entities: snapshot.entities, flows: snapshot.flows, status: snapshot.status, hasMoreEvents: false });
    result.payload_web_feed_hosts_update_bytes = size({ hosts: snapshot.hosts });
    result.payload_web_feed_everything_bytes = size({ hosts: snapshot.hosts, events: snapshot.events, flows: snapshot.flows,
        participants: window.task3Mock.participantsOf(snapshot.entities), hasMoreEvents: false });
    return result;
}
