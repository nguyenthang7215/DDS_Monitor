// SPDX-License-Identifier: GPL-3.0-or-later
// Browser checks of task3/web against recorded MonitoringCore snapshots (see run_web_tests.py).
// ?mode=test runs every check and prints JSON into #task3-web-result; ?mode=shot&scenario=<name>
// only arranges one page state for a screenshot (same states as compare_qml_web.sh).
"use strict";
(function () {
    const { core, ui, calls, load } = window.task3Mock;
    const fixtures = window.TASK3_FIXTURES;
    const params = new URLSearchParams(location.search);
    const $ = id => document.getElementById(id);
    const all = selector => Array.from(document.querySelectorAll(selector));
    const visible = id => !$(id).classList.contains("hidden") && $(id).offsetParent !== null;
    const clone = value => JSON.parse(JSON.stringify(value));
    const errors = [];
    window.addEventListener("error", e => errors.push(String(e.message)));
    window.addEventListener("unhandledrejection", e => errors.push("unhandled rejection: " + e.reason));

    const frame = () => new Promise(resolve => {
        let done = false;
        requestAnimationFrame(() => { done = true; resolve(); });
        setTimeout(() => { if (!done) resolve(); }, 50);
    });
    async function until(predicate, timeoutMs = 2000) {
        const start = performance.now();
        while (performance.now() - start < timeoutMs) {
            await frame();
            if (predicate()) return true;
        }
        return predicate();
    }
    const settle = async () => { for (let i = 0; i < 4; ++i) await frame(); };
    function assert(condition, message) {
        if (!condition) throw new Error(message);
    }
    function equal(actual, expected, message) {
        if (JSON.stringify(actual) !== JSON.stringify(expected))
            throw new Error(message + ": expected " + JSON.stringify(expected) + ", got " + JSON.stringify(actual));
    }
    const called = name => calls.filter(c => c[0] === name);
    const click = element => { assert(element, "element to click is missing"); element.click(); };

    async function reset(fixture) {
        ui.__push("eventsPage", false);
        ui.__push("selectedHostId", "");
        ui.__push("selectedApplicationId", "");
        ui.__push("selectedFlowKey", "");
        ui.__push("detailTab", 0);
        ui.__push("resourceTab", 0);
        load(clone(fixture));
        calls.length = 0;
        await settle();
    }

    const tests = [];
    const test = (name, fn) => tests.push({ name, fn });

    test("page connects to the channel and listens to every feed signal", async () => {
        await until(() => document.body.dataset.ready === "true");
        equal(document.body.dataset.ready, "true", "QWebChannel ready flag");
        assert(document.body.style.fontFamily.includes("DejaVu Sans") && getComputedStyle(document.body).fontFamily.includes("sans-serif"),
            "Qt application font with a sans-serif fallback: " + getComputedStyle(document.body).fontFamily);
        for (const s of ["hostsChanged", "eventsChanged", "flowsChanged", "participantsChanged"])
            assert(core[s].count >= 1, "app.js does not listen to core." + s);
        for (const s of ["eventsPage", "selectedHostId", "selectedApplicationId", "selectedFlowKey", "detailTab", "resourceTab"])
            assert(ui[s + "Changed"].count >= 1, "app.js does not listen to ui." + s + "Changed");
    });

    test("overview: counters, fleet table and host list", async () => {
        await reset(fixtures.mapped);
        assert(visible("overview") && !visible("hostWorkspace"), "overview must be the visible page");
        equal(all("#statCards .count").map(e => e.textContent), ["3", "2", "0", "1"], "TỔNG / ONLINE / OFFLINE / UNKNOWN");
        equal(all("#fleet .fleet-row").length, 3, "fleet rows");
        equal($("allHosts").textContent, "Tất cả Slave  ·  3", "all-hosts button");
        equal(all("#hostList .host-item").length, 3, "host list items");
        const unknown = all("#hostList .host-item").find(e => e.dataset.host === "zz-awaiting-agent");
        assert(unknown && unknown.textContent.includes("● Unknown · 0 ứng dụng"), "host never seen must read Unknown");
        assert($("fleet").textContent.includes("Chưa có telemetry"), "Last Seen of an unseen host");
    });

    test("selecting a host from the fleet table opens its workspace", async () => {
        await reset(fixtures.mapped);
        click(document.querySelector('#fleet .fleet-row[data-host="linux-agent-a"]'));
        assert(await until(() => ui.selectedHostId === "linux-agent-a" && visible("hostWorkspace")), "host workspace not shown");
        const header = $("hostHeader").textContent;
        assert(header.includes("dev") && header.includes("● Online") && header.includes("Fresh"), "header: " + header);
        equal(all("#hostTabs .tab").map(t => t.textContent), ["Tài nguyên", "Ứng dụng", "Luồng DDS", "Đồng bộ giờ"], "detail tabs");
        assert(all("#hostTabs .tab")[0].classList.contains("checked"), "Tài nguyên tab checked");
        equal(all('#hostList .app-item').length, 3, "selected host lists its applications");
    });

    test("resources: CPU, RAM, disks and CPU history chart", async () => {
        await reset(fixtures.mapped);
        ui.chooseHost("linux-agent-a");
        await until(() => visible("pageResources"));
        const host = fixtures.mapped.hosts.find(h => h.host_id === "linux-agent-a");
        assert($("cpuCard").textContent.includes(Number(host.metrics.cpu_percent).toFixed(1) + " %"), "CPU value");
        const mib = v => (Number(v) / 1048576).toFixed(1) + " MiB";
        assert($("ramCard").textContent.includes(mib(host.metrics.ram_used) + " / " + mib(host.metrics.ram_total)), "RAM used/total");
        equal(all("#disks .list-card").length, host.metrics.disks.length, "disk rows");
        const points = document.querySelector("#cpuChart polyline").getAttribute("points").trim().split(/\s+/);
        equal(points.length, host.cpu_history.length, "CPU chart points");
        assert($("cpuChart").textContent.includes("CPU (%) · tối đa 120 mẫu nhận gần nhất · Fresh"), "chart title");
    });

    test("network tab: rows, numeric order and layout report", async () => {
        await reset(fixtures.mapped);
        ui.chooseHost("linux-agent-a");
        await until(() => visible("pageResources"));
        click(document.querySelector('#resourceTabs .tab[data-index="1"]'));
        assert(await until(() => ui.resourceTab === 1 && visible("network")), "network list not shown");
        const names = all("#network .list-card .name").map(e => e.textContent);
        equal(names.length, 6, "interface rows");
        const sorted = names.slice().sort((a, b) => (/^\d+$/.test(a) && /^\d+$/.test(b)) ? Number(a) - Number(b) : (a < b ? -1 : a > b ? 1 : 0));
        equal(names, sorted, "interfaces sorted like the QML list");
        assert(await until(() => called("reportNetwork").some(c => c[3] === 6 && c[1] > 300 && c[2] > 80)),
            "hostNetworkMetrics report (count 6, width > 300, height > 80): " + JSON.stringify(called("reportNetwork").slice(-1)));
    });

    test("network list keeps its scroll position across telemetry updates", async () => {
        await reset(fixtures.mapped);
        const hosts = clone(fixtures.mapped.hosts);
        const host = hosts[0];
        host.metrics.interfaces = Array.from({ length: 40 }, (_, i) => ({ name: String(i + 1), rx_bytes: 1000 * i, tx_bytes: 10 * i, rx_bps: i, tx_bps: i, rate_valid: true }));
        core.__push("hosts", hosts);
        ui.chooseHost(host.host_id);
        ui.resourceTab = 1;
        await until(() => visible("network") && all("#network .list-card").length === 40);
        $("network").scrollTop = 300;
        const before = $("network").scrollTop;
        assert(before > 0, "network list is not scrollable");
        const next = clone(hosts);
        next[0].metrics.interfaces.forEach(n => { n.rx_bytes += 777; });
        core.__push("hosts", next);
        await until(() => $("network").textContent.includes(Number(777).toFixed(1) + " bytes"));
        assert($("network").textContent.includes("777.0 bytes"), "update not rendered");
        equal($("network").scrollTop, before, "scrollTop after a telemetry update");
    });

    test("applications: choose from the host tree, mapped participants", async () => {
        await reset(fixtures.mapped);
        ui.chooseHost("linux-agent-a");
        await until(() => all('#hostList .app-item[data-app="publisher"]').length === 1);
        click(document.querySelector('#hostList .app-item[data-app="publisher"]'));
        assert(await until(() => ui.selectedApplicationId === "publisher" && ui.detailTab === 1 && visible("pageApps")), "applications page");
        const detail = $("appDetail").textContent;
        assert(detail.includes("Participant đã ánh xạ: 2"), "mapped participants: " + detail);
        assert(!detail.includes("Unmapped — chưa có bằng chứng"), "must not claim Unmapped");
        assert(document.querySelector('#appList .app-card.selected[data-app="publisher"]'), "selected application card");
    });

    test("flows: list, selection, inspector and DDS chuyên sâu", async () => {
        await reset(fixtures.mapped);
        ui.chooseApplication("linux-agent-a", "publisher");
        await until(() => visible("pageApps"));
        click(document.querySelector('[data-action="viewFlows"]'));
        assert(await until(() => ui.detailTab === 2 && visible("pageFlows")), "flows page");
        equal(all("#flowList .flow-card").length, 4, "flow rows");
        equal($("appScopeText").textContent, "Ứng dụng: publisher", "scope label");
        assert($("appScope").checked && !$("appScope").disabled, "scope box checked and enabled");
        assert($("flowInspector").textContent.includes("Chọn một luồng để xem chi tiết"), "empty inspector");
        click(all("#flowList .flow-card")[0]);
        assert(await until(() => ui.selectedFlowKey === "52/24" && $("flowInspector").textContent.includes("Chi tiết luồng DDS")), "flow selection");
        assert($("flowInspector").textContent.includes("Matching: Unknown"), "matching state shown");
        assert(all("#flowInspector .arrow").every(a => a.textContent === "···"), "unconfirmed flow drawn without arrow");
        click(document.querySelector('[data-action="openDdsFlow"]'));
        assert(await until(() => called("openDds").some(c => c[1] === "52")), "openDdsWorkspace(\"52\")");
    });

    test("flow scope box starts checked on every visit, like the recreated QML page", async () => {
        await reset(fixtures.mapped);
        ui.chooseApplication("linux-agent-a", "publisher"); // asynchronous, and it sets detailTab to 1
        await until(() => ui.selectedApplicationId === "publisher" && visible("pageApps"));
        ui.detailTab = 2;
        await until(() => visible("pageFlows"));
        $("appScope").checked = false;
        $("appScope").dispatchEvent(new Event("change"));
        await settle();
        ui.detailTab = 0;
        await until(() => visible("pageResources"));
        ui.detailTab = 2;
        await until(() => visible("pageFlows"));
        assert($("appScope").checked, "scope box must be checked again after leaving the tab");
    });

    test("time sync tab never turns a missing offset into 0", async () => {
        await reset(fixtures.mapped);
        ui.chooseHost("linux-agent-a"); // asynchronous, and it resets detailTab to 0
        await until(() => ui.selectedHostId === "linux-agent-a" && visible("pageResources"));
        ui.detailTab = 3;
        await until(() => visible("pageTime"));
        const text = $("pageTime").textContent;
        assert(text.includes("Đồng bộ thời gian") && text.includes("Unavailable"), "time page: " + text);
        assert(!/0\.000e\+0 s/.test(text), "offset rendered as zero");
    });

    test("events page: count, filter, search and Load Older", async () => {
        await reset(fixtures.recovered);
        ui.eventsPage = true;
        assert(await until(() => visible("eventsView") && !visible("systemView")), "events page");
        equal($("eventCount").textContent, "Đã tải 2 bản ghi · đang hiển thị 2. Bộ lọc áp dụng trên các bản ghi đã tải.", "event count");
        equal(all("#eventList .badge").map(b => b.textContent).sort(), ["Active", "Recovered"], "badges");
        $("eventFilter").value = "Recovered";
        $("eventFilter").dispatchEvent(new Event("change"));
        await until(() => all("#eventList .event-row").length === 1);
        equal(all("#eventList .badge").map(b => b.textContent), ["Recovered"], "Recovered filter");
        $("eventFilter").value = "Tất cả";
        $("eventFilter").dispatchEvent(new Event("change"));
        $("eventSearch").value = "MISSING-PROCESS";
        $("eventSearch").dispatchEvent(new Event("input"));
        await until(() => all("#eventList .event-row").length === 1);
        assert($("eventList").textContent.includes("linux-agent-a/missing-process"), "case-insensitive search");
        $("eventSearch").value = "";
        $("eventSearch").dispatchEvent(new Event("input"));
        assert(!visible("loadOlder"), "Load Older hidden without more rows");
        core.__store.hasMoreEvents = true;
        core.__push("events", clone(fixtures.recovered.events));
        assert(await until(() => visible("loadOlder")), "Load Older shown when more rows exist");
        click($("loadOlder"));
        assert(called("loadOlderEvents").length === 1, "loadOlderEvents() called");
    });

    test("Agent Offline: stale values are N/A and marked unverified", async () => {
        await reset(fixtures.offline);
        ui.chooseApplication("linux-agent-a", "publisher");
        await until(() => visible("pageApps"));
        assert($("hostHeader").textContent.includes("● Offline") && $("hostHeader").textContent.includes("Stale"), "offline header");
        assert(document.querySelector("#hostHeader .warn"), "header warning for a stale host");
        assert($("appDetail").textContent.includes("Unknown — Agent không phản hồi"), "stale application warning");
        ui.detailTab = 0;
        await until(() => visible("pageResources"));
        assert($("cpuCard").textContent.includes("N/A %"), "stale CPU must read N/A");
        ui.eventsPage = true;
        await until(() => visible("eventsView"));
        assert($("eventList").textContent.includes("Not verified — Agent Offline"), "unverified alert text");
    });

    test("updates arrive without user action (realtime)", async () => {
        await reset(fixtures.mapped);
        const hosts = clone(fixtures.mapped.hosts);
        hosts[1].last_seen = "2031-02-03T04:05:06.000";
        hosts[1].metrics.cpu_percent = 77.7;
        core.__push("hosts", hosts);
        assert(await until(() => $("fleet").textContent.includes("03/02/2031 04:05:06") && $("fleet").textContent.includes("77.7 %")),
            "fleet table did not follow a hosts update");
    });

    test("telemetry text is escaped (no HTML injection from a Slave)", async () => {
        await reset(fixtures.mapped);
        const hosts = clone(fixtures.mapped.hosts);
        hosts[0].hostname = '<img src=x onerror="window.__task3Xss=1">';
        hosts[0].os = '<b id="task3-injected">os</b>';
        core.__push("hosts", hosts);
        await until(() => $("fleet").textContent.includes("<img src=x"));
        assert(!document.querySelector("#fleet img, #hostList img, #task3-injected"), "markup from telemetry was parsed");
        await settle();
        assert(window.__task3Xss === undefined, "injected handler ran");
    });

    test("Content-Security-Policy blocks network access (CON-04)", async () => {
        const meta = document.querySelector('meta[http-equiv="Content-Security-Policy"]');
        assert(meta && /connect-src 'none'/.test(meta.content) && /default-src 'none'/.test(meta.content), "CSP meta missing");
        const violations = [];
        document.addEventListener("securitypolicyviolation", e => violations.push(e.violatedDirective));
        const image = new Image();
        image.src = "http://127.0.0.1:9/task3-probe.png";
        let fetchBlocked = false;
        try { await fetch("http://127.0.0.1:9/task3-probe"); } catch (e) { fetchBlocked = true; }
        await until(() => violations.length >= 2, 1500);
        assert(fetchBlocked, "fetch to the network was not blocked");
        assert(violations.some(v => v.startsWith("img-src")) && violations.some(v => v.startsWith("connect-src")),
            "expected img-src and connect-src violations, got " + JSON.stringify(violations));
        // The blocked probes above still leave a (failed) timing entry; nothing else may be remote.
        const remote = performance.getEntriesByType("resource").map(r => r.name)
            .filter(n => !/^(file|qrc|data):/.test(n) && !n.includes("task3-probe"));
        equal(remote, [], "resources loaded from outside the package");
    });

    const largeFleet = () => Array.from({ length: 512 }, (_, i) => Object.assign(clone(fixtures.mapped.hosts[0]), {
        host_id: "slave-" + String(i).padStart(3, "0"), hostname: "slave-" + i }));
    const manyEvents = () => Array.from({ length: 500 }, (_, i) =>
        Object.assign(clone(fixtures.recovered.events[i % 2]), { alert_id: "a" + i }));

    // Functional only: timing is measured by ?mode=perf, outside the browser's virtual time.
    test("large fleet: 512 Slaves and 500 events render completely", async () => {
        await reset(fixtures.mapped);
        core.__push("hosts", largeFleet());
        assert(await until(() => all("#fleet .fleet-row").length === 512, 5000), "512 fleet rows");
        equal(all("#statCards .count")[0].textContent, "512", "TỔNG SLAVE");
        ui.eventsPage = true;
        core.__push("events", manyEvents());
        assert(await until(() => all("#eventList .event-row").length === 500, 5000), "500 event rows");
    });

    test("no JavaScript errors", async () => {
        equal(errors, [], "page errors");
    });

    async function runTests() {
        const results = [];
        for (const t of tests) {
            const started = performance.now();
            try {
                await t.fn();
                results.push({ name: t.name, ok: true, ms: Math.round(performance.now() - started) });
            } catch (e) {
                results.push({ name: t.name, ok: false, error: String(e && e.message || e) });
            }
        }
        return { passed: results.filter(r => r.ok).length, total: results.length, results, user_agent: navigator.userAgent };
    }

    async function arrange(scenario) {
        const fixture = scenario === "offline" || scenario === "events" ? fixtures.offline : fixtures.mapped;
        await reset(fixture);
        const host = "linux-agent-a";
        if (scenario === "resources" || scenario === "network" || scenario === "time") ui.__push("selectedHostId", host);
        if (scenario === "network") ui.__push("resourceTab", 1);
        if (scenario === "time") ui.__push("detailTab", 3);
        if (scenario === "apps" || scenario === "flows" || scenario === "offline") {
            ui.__push("selectedHostId", host);
            ui.__push("selectedApplicationId", "publisher");
            ui.__push("detailTab", scenario === "flows" ? 2 : 1);
        }
        if (scenario === "flows") ui.__push("selectedFlowKey", "52/24");
        if (scenario === "events") ui.__push("eventsPage", true);
        await settle();
        return { scenario };
    }

    function publish(output) {
        const pre = document.createElement("pre");
        pre.id = "task3-web-result";
        pre.style.display = "none";
        pre.textContent = JSON.stringify(output);
        document.body.appendChild(pre);
        document.title = "TASK3-WEB-DONE";
    }
    async function main() {
        await until(() => document.body.dataset.ready === "true");
        publish(params.get("mode") === "shot" ? await arrange(params.get("scenario") || "overview") : await runTests());
    }
    if (params.get("mode") === "perf") {
        try {
            publish(task3Measure()); // perf.js
        } catch (e) {
            publish({ error: String(e && e.stack || e) });
        }
    } else
        main();
})();
