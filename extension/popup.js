// popup.js — configuración de feeds y sync manual. Los nombres de los
// calendarios se leen del propio dispositivo para que el selector muestre
// "Trabajo 1" y no "Calendar 0".

const $ = (id) => document.getElementById(id);
const NUM_FEEDS = 4;

let calNames = ["Calendar 1", "Calendar 2", "Calendar 3", "Calendar 4"];

function renderFeeds(feeds) {
  $("feeds").innerHTML = "";
  for (let i = 0; i < NUM_FEEDS; i++) {
    const row = document.createElement("div");
    row.className = "feed";
    const input = document.createElement("input");
    input.type = "text";
    input.placeholder = "https://calendar.google.com/.../basic.ics";
    input.value = feeds[i]?.url || "";
    const sel = document.createElement("select");
    calNames.forEach((name, ci) => {
      const o = document.createElement("option");
      o.value = ci;
      o.textContent = name;
      sel.appendChild(o);
    });
    sel.value = String(feeds[i]?.cal ?? i);
    row.append(input, sel);
    $("feeds").appendChild(row);
  }
}

function readFeeds() {
  return [...$("feeds").children]
    .map((row) => ({
      url: row.children[0].value.trim(),
      cal: +row.children[1].value,
    }))
    .filter((f) => f.url);
}

async function loadCalendarNames(device) {
  try {
    const r = await fetch(`${device}/api/events`, { signal: AbortSignal.timeout(4000) });
    const data = await r.json();
    if (data.calendars?.length) calNames = data.calendars.map((c) => c.name);
  } catch { /* dispositivo fuera de linea: nombres genericos */ }
}

function showLastSync(last) {
  if (!last) return;
  const when = new Date(last.at).toLocaleTimeString();
  const lines = last.results.map((r) =>
    r.ok ? `[ok] ${calNames[r.cal] ?? r.cal}: ${r.queued} synced, ${r.skipped} skipped` +
           (r.free !== undefined && r.free < 20 ? ` (!device almost full: ${r.free} slots left)` : "")
         : `[x] ${calNames[r.cal] ?? r.cal}: ${r.error}`);
  $("status").innerHTML = `Last sync ${when}\n` + lines.join("\n");
  $("status").className = last.results.every((r) => r.ok) ? "ok" : "err";
}

async function init() {
  const cfg = await chrome.storage.sync.get({
    device: "http://prabbit.local", feeds: [], auto: true, intervalMin: 60,
  });
  $("device").value = cfg.device;
  $("auto").checked = cfg.auto;
  $("interval").value = String(cfg.intervalMin);

  await loadCalendarNames(cfg.device);
  renderFeeds(cfg.feeds);

  const { lastSync } = await chrome.storage.local.get("lastSync");
  showLastSync(lastSync);
}

async function save() {
  await chrome.storage.sync.set({
    device: $("device").value.trim().replace(/\/$/, "") || "http://prabbit.local",
    feeds: readFeeds(),
    auto: $("auto").checked,
    intervalMin: +$("interval").value,
  });
  await chrome.runtime.sendMessage({ cmd: "reschedule" });
  $("status").textContent = "Saved.";
  $("status").className = "ok";
}

$("save").onclick = save;

$("sync").onclick = async () => {
  await save();
  $("status").textContent = "Syncing...";
  $("status").className = "";
  const results = await chrome.runtime.sendMessage({ cmd: "sync" });
  const { lastSync } = await chrome.storage.local.get("lastSync");
  showLastSync(lastSync ?? { at: Date.now(), results });
};

init();
