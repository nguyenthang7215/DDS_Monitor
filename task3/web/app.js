// SPDX-License-Identifier: GPL-3.0-or-later
// Task 3 dashboard for Qt WebEngine. Mirrors Dashboard.qml, HostWorkspace.qml, FlowDetails.qml and
// EventHistoryView.qml: same texts, states and formatting. Data: `core` (task3::WebFeed, a trimmed
// view of MonitoringCore) and `ui` (selection state owned by WebDashboard.qml) are QObjects
// published through QWebChannel.
"use strict";

let core = null;
let ui = null;
const local = { hostSearch: "", eventSearch: "", eventFilter: "Tất cả", appScope: true, detailTab: -1, leftWidth: 260 };
const html = new Map();
const $ = id => document.getElementById(id);

// ---------- formatting (same rules as the QML helpers) ----------
const esc = v => String(v === undefined || v === null ? "" : v).replace(/[&<>"']/g,
    c => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", "\"": "&quot;", "'": "&#39;" }[c]));
const finite = v => v !== undefined && v !== null && isFinite(Number(v));
const fresh = h => h.online === true && h.metrics_state === "fresh";
const number = v => finite(v) ? Number(v).toFixed(1) : "N/A";
const mb = v => finite(v) ? (Number(v) / 1048576).toFixed(1) + " MiB" : "N/A";
const metric = (h, v, bytes) => fresh(h) ? (bytes ? mb(v) : number(v)) : "N/A";
const offset = v => finite(v) ? Number(v).toExponential(3) + " s" : "Unavailable";
const pad = n => String(n).padStart(2, "0");
const formatDate = d => pad(d.getDate()) + "/" + pad(d.getMonth() + 1) + "/" + d.getFullYear() + " " +
    pad(d.getHours()) + ":" + pad(d.getMinutes()) + ":" + pad(d.getSeconds());
const time = v => !finite(v) || Number(v) <= 0 ? "Unknown" : formatDate(new Date(Number(v)));
function lastSeen(v) {
    const date = new Date(v);
    return !v || v === "never" || isNaN(date.getTime()) ? "Chưa có telemetry" : formatDate(date);
}
const hostState = h => !h.metrics_state || h.metrics_state === "unknown" ? "Unknown" : h.online ? "Online" : "Offline";
const stateColor = h => hostState(h) === "Unknown" ? "#808080" : h.online ? "#237a45" : "#b33030";
const freshness = h => h.metrics_state === "fresh" ? "Fresh" : h.metrics_state === "stale" ? "Stale" : "Unknown";
const booleanText = v => typeof v === "boolean" ? (v ? "Có" : "Không") : "Unknown";
const flowKey = f => String(f.writer_id || "") + "/" + String(f.reader_id || "");

function setHtml(id, markup) {
    if (html.get(id) === markup)
        return;
    html.set(id, markup);
    $(id).innerHTML = markup;
}
const show = (id, visible) => $(id).classList.toggle("hidden", !visible);

// ---------- state derived from the channel objects ----------
function state() {
    const hosts = core.hosts || [];
    const host = hosts.find(h => h.host_id === ui.selectedHostId) || {};
    const application = (host.applications || []).find(a => a.application_id === ui.selectedApplicationId) || {};
    return { hosts, host, application };
}

// ---------- left panel ----------
function renderLeft(s) {
    $("allHosts").textContent = "Tất cả Slave  ·  " + s.hosts.length;
    const filtered = s.hosts.filter(h => !local.hostSearch || (h.host_id + " " + h.hostname).toLowerCase().includes(local.hostSearch));
    setHtml("hostList", filtered.map(h => {
        const selected = ui.selectedHostId === h.host_id;
        const apps = selected ? (h.applications || []).map(a =>
            `<div class="app-item ellipsis${ui.selectedApplicationId === a.application_id ? " selected" : ""}" data-action="app" data-host="${esc(h.host_id)}" data-app="${esc(a.application_id)}" title="${esc(a.application_id)}">${esc(a.application_id)}</div>`).join("") : "";
        return `<div class="host-node"><div class="host-item${selected ? " selected" : ""}" data-action="host" data-host="${esc(h.host_id)}" title="${esc(h.host_id)}">` +
            `<div class="bold ellipsis">${esc(h.hostname || h.host_id)}</div>` +
            `<div class="label12 ellipsis">${esc(h.host_id)}</div>` +
            `<div class="small13 ellipsis" style="color:${stateColor(h)}">● ${hostState(h)} · ${(h.applications || []).length} ứng dụng</div></div>${apps}</div>`;
    }).join(""));
}

// ---------- overview ----------
function renderOverview(s) {
    const cards = [
        ["TỔNG SLAVE", s.hosts.length, "#09487e"],
        ["ONLINE", s.hosts.filter(h => hostState(h) === "Online").length, "#237a45"],
        ["OFFLINE", s.hosts.filter(h => hostState(h) === "Offline").length, "#b33030"],
        ["UNKNOWN", s.hosts.filter(h => hostState(h) === "Unknown").length, "#808080"]];
    setHtml("statCards", cards.map(c => `<div class="frame stat"><div class="label12">${c[0]}</div><div class="count" style="color:${c[2]}">${c[1]}</div></div>`).join(""));
    const width = Math.max($("fleet").clientWidth, 880);
    const hostColumn = Math.max(200, (width - 80) * 0.25), ramColumn = Math.max(190, (width - 80) * 0.24);
    const col = w => `flex:0 0 ${w}px;width:${w}px`;
    const head = `<div class="table-head"><span style="${col(hostColumn)}">SLAVE / HOST ID</span><span style="${col(140)}">TRẠNG THÁI</span>` +
        `<span style="${col(72)}">CPU</span><span style="${col(ramColumn)}">RAM USED / TOTAL</span><span class="grow">LAST SEEN</span></div>`;
    const rows = s.hosts.map((h, i) => {
        const m = h.metrics || {};
        return `<div class="fleet-row${i % 2 ? " odd" : ""}" data-action="host" data-host="${esc(h.host_id)}" title="${esc(h.host_id + "\n" + h.os)}">` +
            `<div style="${col(hostColumn)}"><span class="bold ellipsis">${esc(h.hostname)}</span><span class="label12 ellipsis">${esc(h.host_id)}</span><span class="label12 ellipsis">${esc(h.os)}</span></div>` +
            `<div style="${col(140)}"><span class="bold" style="color:${stateColor(h)}">● ${hostState(h)}</span><span class="label12 ellipsis">${freshness(h)} · ${h.alert_count || 0} cảnh báo</span></div>` +
            `<div style="${col(72)}">${metric(h, m.cpu_percent, false)} %</div>` +
            `<div class="wrap" style="${col(ramColumn)};display:block">${metric(h, m.ram_used, true)} / ${metric(h, m.ram_total, true)}</div>` +
            `<div class="col-last wrap">${lastSeen(h.last_seen)}</div></div>`;
    }).join("");
    const empty = s.hosts.length ? "" : `<div class="empty">Chưa có Slave. Khởi chạy Agent với cấu hình giám sát để nhận dữ liệu.</div>`;
    setHtml("fleet", `<div class="fleet-table" style="width:${width}px">${head}${rows}</div>${empty}`);
}

// ---------- host workspace ----------
const tabs = (items, current, action, bold) => items.map((t, i) =>
    `<div class="tab${bold ? " emph" : ""}${i === current ? " checked" : ""}" data-action="${action}" data-index="${i}"><span>${t}</span></div>`).join("");

function renderHostHeader(s) {
    const h = s.host;
    setHtml("hostHeader",
        `<div class="row"><div class="h23 grow ellipsis">${esc(h.hostname || ui.selectedHostId)}</div>` +
        `<span class="bold" style="color:${stateColor(h)}">● ${hostState(h)}</span><span class="muted" style="margin-left:5px">${freshness(h)}</span></div>` +
        `<div class="muted wrap pre" style="margin-top:5px">Host ID: ${esc(ui.selectedHostId)}  ·  ${esc(h.os || "Unknown")}</div>` +
        `<div class="small13 wrap pre" style="margin-top:5px">Last Seen: ${lastSeen(h.last_seen)}  ·  Telemetry source: ${esc(h.address || "N/A")}</div>` +
        (fresh(h) ? "" : `<div class="warn wrap" style="margin-top:5px">${esc(h.host_state || "Unknown — chưa có telemetry")}</div>`));
    setHtml("hostTabs", tabs(["Tài nguyên", "Ứng dụng", "Luồng DDS", "Đồng bộ giờ"], ui.detailTab, "detailTab", true));
    ["pageResources", "pageApps", "pageFlows", "pageTime"].forEach((id, i) => show(id, i === ui.detailTab));
}

function renderResources(s) {
    const h = s.host, m = h.metrics || {}, health = h.collector_health || {};
    setHtml("cpuCard", `<div class="small13 muted">CPU</div><div class="metric-value">${metric(h, m.cpu_percent, false)} %</div>`);
    const ramUnavailable = health.ram === "unavailable";
    setHtml("ramCard", `<div class="small13 muted pre">RAM  ·  USED / TOTAL</div><div class="metric-value">${metric(h, m.ram_percent, false)} %</div>` +
        `<div class="small13 wrap">${metric(h, ramUnavailable ? null : m.ram_used, true)} / ${metric(h, ramUnavailable ? null : m.ram_total, true)}</div>`);
    renderCpuChart(h);
    setHtml("resourceTabs", tabs(["Ổ đĩa / phân vùng", "Mạng / interface"], ui.resourceTab, "resourceTab", false));
    show("disks", ui.resourceTab === 0);
    show("network", ui.resourceTab === 1);
    const disks = m.disks || [];
    setHtml("disks", disks.map(d => `<div class="list-card"><div class="name ellipsis" title="${esc(d.path)}">${esc(d.path)}</div>` +
        ["TOTAL", "USED", "FREE"].map((k, i) => `<div class="c140"><span class="label12">${k}</span><span>${metric(h, d.valid ? [d.total, d.used, d.free][i] : null, true)}</span></div>`).join("") +
        `</div>`).join("") + (disks.length ? "" : `<div class="empty">N/A — chưa có dữ liệu ổ đĩa</div>`));
    const rows = (m.interfaces || []).slice().sort((a, b) => {
        const left = String(a.name), right = String(b.name);
        if (/^\d+$/.test(left) && /^\d+$/.test(right) && Number(left) !== Number(right))
            return Number(left) - Number(right);
        return left < right ? -1 : left > right ? 1 : 0;
    });
    // The container is kept between updates, so the scroll position survives telemetry refreshes.
    setHtml("network", `<div class="network-head">RX/TX theo từng interface · tốc độ B/s · bộ đếm byte tích lũy</div>` +
        rows.map(n => `<div class="list-card"><div class="name ellipsis" title="${esc(n.name)}">${esc(n.name)}</div>` +
            `<div class="c230"><span class="pre" style="color:#09487e">RX  ${metric(h, n.rate_valid === true ? n.rx_bps : null, false)} B/s</span><span class="label12">${metric(h, n.rx_bytes, false)} bytes</span></div>` +
            `<div class="c230"><span class="pre" style="color:#09487e">TX  ${metric(h, n.rate_valid === true ? n.tx_bps : null, false)} B/s</span><span class="label12">${metric(h, n.tx_bytes, false)} bytes</span></div></div>`).join("") +
        (rows.length ? "" : `<div class="empty">N/A — chưa có dữ liệu interface</div>`));
    if (ui.resourceTab === 1) {
        const list = $("network");
        ui.reportNetwork(list.clientWidth, list.clientHeight, rows.length, list.scrollHeight);
    }
}

function renderCpuChart(h) {
    const box = $("cpuChart");
    const pageHeight = $("pageResources").clientHeight;
    const height = Math.round(Math.min(185, Math.max(85, pageHeight * 0.28)));
    const width = box.clientWidth;
    box.style.height = height + "px";
    // Matches the QtCharts ChartView geometry: inset white background, title, plot area.
    const left = 39, right = 10, top = 37, bottom = 12;
    const plotW = Math.max(10, width - left - right), plotH = Math.max(10, height - top - bottom);
    const x = i => left + plotW * i / 120, y = v => top + plotH * (1 - Math.min(100, Math.max(0, v)) / 100);
    const grid = [0, 30, 60, 90, 120].map(i => `<line x1="${x(i)}" y1="${top}" x2="${x(i)}" y2="${top + plotH}" stroke="#dcdcdc"/>`).join("") +
        [0, 50, 100].map(v => `<line x1="${left}" y1="${y(v)}" x2="${left + plotW}" y2="${y(v)}" stroke="#dcdcdc"/>` +
            `<text x="${left - 5}" y="${y(v) + 4}" text-anchor="end" font-size="12" fill="#000">${v}</text>`).join("");
    const points = (h.cpu_history || []).map((p, i) => `${x(i).toFixed(1)},${y(Number(p.value) || 0).toFixed(1)}`).join(" ");
    const title = "CPU (%) · tối đa 120 mẫu nhận gần nhất · " + freshness(h);
    setHtml("cpuChart", `<svg width="${width}" height="${height}"><rect x="9" y="12" width="${Math.max(0, width - 18)}" height="${Math.max(0, height - 16)}" fill="white"/>` +
        `<text x="${width / 2}" y="23" text-anchor="middle" font-size="12">${esc(title)}</text>` +
        `${grid}<path d="M${left} ${top}V${top + plotH}H${left + plotW}" fill="none" stroke="#bdbdbd"/>` +
        (points ? `<polyline points="${points}" fill="none" stroke="#0895cd" stroke-width="2"/>` : "") + `</svg>`);
}

function renderApplications(s) {
    const h = s.host, a = s.application;
    const vertical = $("pageApps").clientWidth <= 800;
    $("pageApps").classList.toggle("vertical", vertical);
    $("pageApps").classList.toggle("split", true);
    const apps = h.applications || [];
    setHtml("appList", apps.map(app => `<div class="select-card app-card${ui.selectedApplicationId === app.application_id ? " selected" : ""}" data-action="app" data-host="${esc(ui.selectedHostId)}" data-app="${esc(app.application_id)}" title="${esc(app.application_id)}">` +
        `<div class="bold ellipsis">${esc(app.application_id)}</div>` +
        `<div class="label12 ellipsis">${fresh(h) ? "Process: " + esc(app.state) + " · Service: " + esc(app.service_state) : "Unknown · Agent " + hostState(h)}</div></div>`).join("") +
        (apps.length ? "" : `<div class="empty">Chưa có ứng dụng được cấu hình</div>`));
    const selected = !!ui.selectedApplicationId;
    const participants = (core.participants || []).filter(e => e.host_id === ui.selectedHostId && e.application_id === ui.selectedApplicationId);
    let body = `<div class="h22 wrap">${esc(ui.selectedApplicationId || "Chọn một ứng dụng")}</div>`;
    if (selected && !fresh(h))
        body += `<div class="warn wrap">Unknown — Agent không phản hồi. Thông tin dưới đây là lần ghi nhận trước, chưa thể xác minh hiện tại.</div>`;
    if (selected) {
        body += `<div class="detail-grid"><span class="key">Process</span><span class="wrap">${esc(a.process_name || "N/A")} · ${fresh(h) ? esc(a.state || "Unknown") : "Unknown (last: " + esc(a.state || "Unknown") + ")"}</span>` +
            `<span class="key">Service</span><span class="wrap">${esc(a.service || "Không cấu hình")} · ${fresh(h) ? esc(a.service_state || "Unknown") : "Unknown"}</span>` +
            `<span class="key">PID / instances</span><span>${esc(a.pid || "N/A")} / ${(a.instances || []).length}</span></div>`;
        body += (a.instances || []).map(i => `<div class="instance"><div class="bold">PID ${esc(i.pid)} · ${esc(i.state || "Unknown")}</div>` +
            `<div class="small12 wrap">Instance: ${esc(i.instance_id)}</div><div class="muted">Binding: ${esc(i.binding_source || "Unmapped")}</div></div>`).join("");
        body += `<div class="bold">Participant đã ánh xạ: ${participants.length}</div>`;
        body += participants.map(p => `<div class="small12 wrap">${esc(p.guid)}<br>PID ${esc(p.pid)} · ${esc(p.mapping_state || "Unmapped")}</div>`).join("");
        if (!participants.length)
            body += `<div class="muted wrap">Unmapped — chưa có bằng chứng liên kết DDS với lần chạy này.</div>`;
    }
    body += `<div class="buttons"><button class="button" data-action="viewFlows"${selected ? "" : " disabled"}>Xem luồng ứng dụng</button>` +
        `<button class="button" data-action="openDds" data-entity="">Mở DDS chuyên sâu</button></div>`;
    setHtml("appDetail", body);
}

function flowRows() {
    const host = ui.selectedHostId, app = ui.selectedApplicationId;
    const scoped = local.appScope && !!app;
    return (core.flows || []).filter(f => (f.source_host === host || f.target_host === host) &&
        (!scoped || (f.source_host === host && f.source_application === app) || (f.target_host === host && f.target_application === app)));
}

function renderFlows() {
    const app = ui.selectedApplicationId;
    $("appScope").disabled = !app;
    $("appScopeText").textContent = app ? "Ứng dụng: " + app : "Tất cả ứng dụng trên Slave";
    $("appScopeLabel").title = $("appScopeText").textContent;
    const rows = flowRows();
    setHtml("flowList", rows.map(f => `<div class="select-card flow-card${flowKey(f) === ui.selectedFlowKey ? " selected" : ""}" data-action="flow" data-key="${esc(flowKey(f))}" ` +
        `title="${esc((f.topic || "Unknown Topic") + "\n" + (f.source_application || "Unmapped app") + " → " + (f.target_application || "Unmapped app"))}">` +
        `<div class="left"><span class="bold ellipsis">${esc(f.topic || "Unknown Topic")}</span>` +
        `<span class="small12 ellipsis">${esc(f.source_application || "Unmapped app")}${f.confirmed ? " → " : " ··· "}${esc(f.target_application || "Unmapped app")}</span>` +
        `<span class="label12">Writer #${esc(f.writer_id)} / Reader #${esc(f.reader_id || "Unknown")}</span></div>` +
        `<div class="right"><span class="bold" style="color:${f.confirmed ? "#237a45" : "#585858"}">${esc(f.match_state || "Unknown")}</span>` +
        `<span class="small12 ellipsis">${esc(f.flow_state || "Unknown")}</span></div></div>`).join("") +
        (rows.length ? "" : `<div class="empty">Chưa có luồng được ánh xạ trong phạm vi đã chọn. Xem thực thể Unmapped tại DDS chuyên sâu.</div>`));
    const f = rows.find(r => flowKey(r) === ui.selectedFlowKey) || {};
    let body = `<div class="h17">${f.writer_id ? "Chi tiết luồng DDS" : "Chọn một luồng để xem chi tiết"}</div>`;
    if (f.writer_id) {
        const arrow = `<span class="arrow">${f.confirmed ? "→" : "···"}</span>`;
        body += `<div class="flow-path"><div class="end wrap">${esc(f.source_host || "Unmapped host")} / ${esc(f.source_application || "Unmapped app")}<br>Writer #${esc(f.writer_id || "—")}</div>${arrow}` +
            `<div class="end wrap bold" style="color:#09487e">${esc(f.topic || "Unknown Topic")}</div>${arrow}` +
            `<div class="end wrap">${esc(f.target_host || "Unmapped host")} / ${esc(f.target_application || "Unmapped app")}<br>Reader #${esc(f.reader_id || "Unknown")}</div></div>`;
        body += `<div class="wrap">Matching: ${esc(f.match_state || "Unknown")} · Data: ${esc(f.flow_state || "Unknown")}</div>`;
        body += `<div class="row" style="gap:24px;align-items:flex-start">` +
            `<div class="grow1 column" style="gap:4px"><span class="label12">THROUGHPUT · WRITER</span><span class="stat-value">${number(f.throughput_bps)} B/s</span><span class="small13 wrap">Cập nhật: ${time(f.throughput_timestamp_ms)}</span></div>` +
            `<div class="grow1 column" style="gap:4px"><span class="label12">LATENCY · WRITER / READER</span><span class="stat-value">${number(f.latency_us)} µs</span><span class="small13 wrap">Cập nhật: ${time(f.latency_timestamp_ms)}</span></div></div>`;
        body += `<div class="note wrap">Thống kê theo cửa sổ 120 s hiện có. Biểu đồ động/lịch sử và QoS nằm trong DDS chuyên sâu.</div>`;
    }
    setHtml("flowInspector", body);
}

function renderTime(s) {
    const h = s.host, t = h.time_sync || {}, ok = fresh(h);
    const rows = [
        ["Trạng thái", `<b>${esc(ok ? t.state || "Unknown" : "Unknown · " + freshness(h))}</b>`],
        ["Dịch vụ đang chạy", ok ? booleanText(t.service_running) : "Unknown"],
        ["Source / Reference ID", `<span class="wrap">${esc(t.source || "Unavailable")}</span>`],
        ["Đồng bộ với nguồn đã chọn", ok ? booleanText(t.clock_synced) : "Unknown"],
        ["Nguồn Master đã xác minh", ok ? booleanText(t.master_source_verified) : "Unknown"],
        [t.offset_kind === "absolute_w32time_estimate" ? "Windows absolute offset estimate" : "Chrony Last offset", offset(ok ? t.offset_seconds : null)],
        ["Chrony System time correction", offset(ok ? t.system_time_seconds : null)]];
    setHtml("pageTime", `<div class="column"><div class="h23">Đồng bộ thời gian</div>` +
        `<div class="muted wrap">Nguồn dữ liệu: dịch vụ hệ điều hành trên Slave · cập nhật: ${time(t.checked_ms)}</div>` +
        `<div class="frame"><div class="time-grid">${rows.map(r => `<span class="key">${r[0]}</span><span>${r[1]}</span>`).join("")}</div></div>` +
        `<div class="muted wrap">${esc(t.detail || "Unknown — chưa có kết quả truy vấn dịch vụ đồng bộ.")}</div></div>`);
}

// ---------- events ----------
function renderEvents() {
    const all = core.events || [];
    const term = local.eventSearch.toLowerCase();
    const rows = all.filter(e => (local.eventFilter === "Tất cả" || e.state === local.eventFilter) &&
        (!term || (e.source + " " + e.message).toLowerCase().includes(term)));
    $("eventCount").textContent = "Đã tải " + all.length + " bản ghi · đang hiển thị " + rows.length + ". Bộ lọc áp dụng trên các bản ghi đã tải.";
    setHtml("eventList", rows.map((e, i) => {
        const severity = e.severity === "error" || e.severity === "critical" ? "#b33030" : e.severity === "warning" ? "#9a6700" : "#09487e";
        const active = e.state === "Active";
        const accent = active ? severity : "#237a45";
        const verification = e.verification ? `<div class="small12 wrap" style="color:${e.verification.startsWith("Not verified") ? "#9a6700" : "#585858"}">${esc(e.verification)}</div>` : "";
        const recovered = e.recovered_ms === undefined || e.recovered_ms === null ? "Chưa khôi phục" : time(e.recovered_ms);
        return `<div class="event-row${i % 2 ? " odd" : ""}" style="--accent:${accent}"><div class="main">` +
            `<div class="row" style="gap:10px"><span class="badge" style="color:${accent};background:${active ? "#fff4e8" : "#eaf5ef"}">${esc(e.state)}</span>` +
            `<span class="bold" style="color:${severity}">${esc(e.severity)}</span><span class="small13 muted ellipsis grow">${esc(e.kind)}</span></div>` +
            `<div class="bold wrap" style="color:#09487e">${esc(e.source || "Unknown object")}</div>` +
            `<div class="wrap">${esc(e.message || "Unknown condition")}</div>${verification}</div>` +
            `<div class="times"><span class="wrap">Phát sinh: ${time(e.triggered_ms)}</span><span class="wrap muted">Khôi phục: ${recovered}</span></div></div>`;
    }).join("") + (rows.length ? "" : `<div class="empty">Chưa có sự kiện phù hợp trong dữ liệu đã tải.</div>`));
    show("loadOlder", core.hasMoreEvents === true);
}

// ---------- render loop ----------
let pending = false;
function render() {
    if (pending || !core || !ui)
        return;
    pending = true;
    requestAnimationFrame(() => {
        pending = false;
        const s = state();
        show("systemView", !ui.eventsPage);
        show("eventsView", ui.eventsPage);
        $("leftPanel").style.flexBasis = local.leftWidth + "px";
        if (ui.eventsPage) {
            renderEvents();
            return;
        }
        renderLeft(s);
        show("overview", !ui.selectedHostId);
        show("hostWorkspace", !!ui.selectedHostId);
        if (!ui.selectedHostId) {
            renderOverview(s);
            return;
        }
        renderHostHeader(s);
        // FlowDetails.qml is recreated on every visit of the tab, so its scope box starts checked.
        if (ui.detailTab === 2 && local.detailTab !== 2) {
            local.appScope = true;
            $("appScope").checked = true;
        }
        local.detailTab = ui.detailTab;
        if (ui.detailTab === 0) renderResources(s);
        if (ui.detailTab === 1) renderApplications(s);
        if (ui.detailTab === 2) renderFlows();
        if (ui.detailTab === 3) renderTime(s);
    });
}

// ---------- interaction ----------
document.addEventListener("click", event => {
    const target = event.target.closest("[data-action]");
    if (!target || !ui || target.disabled)
        return;
    const d = target.dataset;
    switch (d.action) {
    case "allHosts": ui.chooseHost(""); break;
    case "host": ui.chooseHost(d.host); break;
    case "app": ui.chooseApplication(d.host, d.app); break;
    case "detailTab": ui.detailTab = Number(d.index); break;
    case "resourceTab": ui.resourceTab = Number(d.index); break;
    case "flow": ui.selectedFlowKey = d.key; break;
    case "viewFlows": ui.detailTab = 2; break;
    case "openDds": ui.openDds(d.entity || ""); break;
    case "openDdsFlow": {
        const f = flowRows().find(r => flowKey(r) === ui.selectedFlowKey) || {};
        ui.openDds(f.writer_id || "");
        break;
    }
    case "loadOlder": core.loadOlderEvents(); break;
    }
    render();
});
$("hostSearch").addEventListener("input", e => { local.hostSearch = e.target.value.toLowerCase(); render(); });
$("eventSearch").addEventListener("input", e => { local.eventSearch = e.target.value; render(); });
$("eventFilter").addEventListener("change", e => { local.eventFilter = e.target.value; render(); });
$("appScope").addEventListener("change", e => { local.appScope = e.target.checked; render(); });
document.addEventListener("contextmenu", e => e.preventDefault());
window.addEventListener("resize", () => { html.clear(); render(); });

function dragHandle(id, onMove) {
    $(id).addEventListener("mousedown", start => {
        start.preventDefault();
        const move = e => { onMove(e, start); html.clear(); render(); };
        const up = () => { window.removeEventListener("mousemove", move); window.removeEventListener("mouseup", up); };
        window.addEventListener("mousemove", move);
        window.addEventListener("mouseup", up);
    });
}
dragHandle("leftHandle", e => { local.leftWidth = Math.min(380, Math.max(210, e.clientX - 16)); });
dragHandle("appHandle", e => {
    const box = $("pageApps").getBoundingClientRect();
    const vertical = $("pageApps").classList.contains("vertical");
    const size = vertical ? e.clientY - box.top : e.clientX - box.left;
    $("appList").style.flexBasis = Math.max(85, size) + "px";
});
dragHandle("flowHandle", e => {
    const box = $("flowList").parentElement.getBoundingClientRect();
    $("flowInspector").style.flexBasis = Math.min(box.height - 110, Math.max(140, box.bottom - e.clientY)) + "px";
});

new QWebChannel(qt.webChannelTransport, channel => {
    core = channel.objects.core;
    ui = channel.objects.ui;
    if (ui.fontFamily)
        document.body.style.fontFamily = "\"" + ui.fontFamily + "\", sans-serif";
    ["hostsChanged", "eventsChanged", "flowsChanged", "participantsChanged"].forEach(s => core[s].connect(render));
    ["eventsPage", "selectedHostId", "selectedApplicationId", "selectedFlowKey", "detailTab", "resourceTab"]
        .forEach(p => ui[p + "Changed"].connect(render));
    document.body.dataset.ready = "true";
    render();
});
