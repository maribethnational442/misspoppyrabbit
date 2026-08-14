// ============================================================================
// background.js — service worker: sincroniza los feeds ICS con el dispositivo
// en segundo plano (chrome.alarms) o cuando el popup pide "Sync now".
// ============================================================================

importScripts("ics.js");

const DEFAULTS = {
  device: "http://prabbit.local",
  feeds: [],            // [{url:"https://...basic.ics", cal:0}, ...] max 4
  capturedCal: 0,       // calendario destino de lo capturado en pestañas
  auto: true,
  intervalMin: 60,
};

async function getCfg() {
  return await chrome.storage.sync.get(DEFAULTS);
}

async function syncAll() {
  const cfg = await getCfg();
  const results = [];

  for (const feed of cfg.feeds) {
    if (!feed.url) continue;
    try {
      const res = await fetch(feed.url, { cache: "no-store" });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const { events, skipped } = icsToImportable(await res.text());

      let queued = 0, dropped = 0, free;
      // En tandas de 40: el dispositivo tiene 512KB de RAM y cero paciencia
      for (let i = 0; i < events.length; i += 40) {
        const r = await fetch(`${cfg.device}/api/events/import`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ calendarId: feed.cal, events: events.slice(i, i + 40) }),
        });
        if (!r.ok) throw new Error(`device HTTP ${r.status}`);
        const j = await r.json();
        queued += j.queued || 0;
        dropped += j.dropped || 0;
        free = j.free;
      }
      results.push({ cal: feed.cal, ok: true, queued, dropped, skipped, free });
    } catch (err) {
      results.push({ cal: feed.cal, ok: false, error: String(err.message || err) });
    }
  }

  // Eventos capturados por el content script en las pestañas de calendario
  // (el camino para calendarios corporativos con el ICS secreto bloqueado)
  const { captured = {} } = await chrome.storage.local.get("captured");
  const now = Date.now() / 1000;
  const capEvents = Object.values(captured)
    .filter((e) => e.start >= now - 3600 && e.start <= now + 60 * 86400)
    .map((e) => ({ uid: e.uid, title: e.title, start: Math.round(e.start), end: Math.round(e.end) }));
  if (capEvents.length) {
    try {
      let queued = 0, dropped = 0, free;
      for (let i = 0; i < capEvents.length; i += 40) {
        const r = await fetch(`${cfg.device}/api/events/import`, {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ calendarId: cfg.capturedCal, events: capEvents.slice(i, i + 40) }),
        });
        if (!r.ok) throw new Error(`device HTTP ${r.status}`);
        const j = await r.json();
        queued += j.queued || 0;
        dropped += j.dropped || 0;
        free = j.free;
      }
      results.push({ cal: cfg.capturedCal, src: "capture", ok: true, queued, dropped, skipped: 0, free });
    } catch (err) {
      results.push({ cal: cfg.capturedCal, src: "capture", ok: false, error: String(err.message || err) });
    }
  }

  await chrome.storage.local.set({ lastSync: { at: Date.now(), results } });
  return results;
}

async function reschedule() {
  const cfg = await getCfg();
  await chrome.alarms.clear("prabbit-sync");
  const { captured = {} } = await chrome.storage.local.get("captured");
  if (cfg.auto && (cfg.feeds.some(f => f.url) || Object.keys(captured).length)) {
    chrome.alarms.create("prabbit-sync", { periodInMinutes: Math.max(15, cfg.intervalMin) });
  }
}

chrome.alarms.onAlarm.addListener((alarm) => {
  if (alarm.name === "prabbit-sync") syncAll();
});

chrome.runtime.onInstalled.addListener(reschedule);
chrome.runtime.onStartup.addListener(reschedule);

chrome.runtime.onMessage.addListener((msg, _sender, sendResponse) => {
  if (msg.cmd === "sync") {
    syncAll().then(sendResponse);
    return true;   // respuesta asíncrona
  }
  if (msg.cmd === "reschedule") {
    reschedule().then(() => sendResponse(true));
    return true;
  }
});
