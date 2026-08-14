// ============================================================================
// ics.js — parser de ICS + expansión de recurrencias. Es el MISMO algoritmo
// que usa la WebUI del dispositivo (web/index.html); si arreglas un bug aquí,
// arréglalo allá también.
// ============================================================================

const DAY_MS = 86400000;

function parseICS(text) {
  // "Unfold": las lineas largas del ICS continuan con espacio inicial
  const lines = text.replace(/\r?\n[ \t]/g, "").split(/\r?\n/);
  const events = [];
  let cur = null;
  for (const ln of lines) {
    if (ln === "BEGIN:VEVENT") { cur = { exdates: [] }; continue; }
    if (ln === "END:VEVENT") { if (cur) events.push(cur); cur = null; continue; }
    if (!cur) continue;
    const ci = ln.indexOf(":");
    if (ci < 0) continue;
    const [prop, ...params] = ln.slice(0, ci).split(";");
    const val = ln.slice(ci + 1);
    const tzid = params.find(p => p.startsWith("TZID="))?.slice(5);
    if (prop === "UID") cur.uid = val;
    else if (prop === "SUMMARY")
      cur.title = val.replace(/\\,/g, ",").replace(/\\;/g, ";").replace(/\\n/g, " ").replace(/\\\\/g, "\\");
    else if (prop === "DTSTART") { cur.start = icsDate(val, tzid); cur.allDay = params.some(p => p === "VALUE=DATE"); }
    else if (prop === "DTEND") cur.end = icsDate(val, tzid);
    else if (prop === "RRULE") cur.rrule = val;
    else if (prop === "EXDATE") cur.exdates.push(icsDate(val, tzid));
    else if (prop === "STATUS") cur.status = val;
  }
  return events;
}

// 20260814T130000Z (UTC) | 20260814T130000 + TZID (se honra la zona REAL del
// evento: una reunion creada "a las 11" en Chile es a las 10 en Bogota) |
// sin TZID: zona local del equipo | 20260814 (dia completo)
function icsDate(v, tzid) {
  const m = v.match(/^(\d{4})(\d{2})(\d{2})(?:T(\d{2})(\d{2})(\d{2})?(Z)?)?$/);
  if (!m) return null;
  const [, Y, Mo, D, h = "0", mi = "0", s = "0", z] = m;
  if (z) return Date.UTC(+Y, +Mo - 1, +D, +h, +mi, +s) / 1000;
  if (tzid) {
    const t = zonedToEpoch(+Y, +Mo, +D, +h, +mi, +s, tzid);
    if (t !== null) return t;   // TZID raro (no-IANA, estilo Outlook): cae abajo
  }
  return new Date(+Y, +Mo - 1, +D, +h, +mi, +s).getTime() / 1000;
}

// Convierte "hora de pared" en una zona IANA a epoch usando Intl (dos
// iteraciones para clavar el offset incluso en cambios de horario de verano).
function zonedToEpoch(y, mo, d, h, mi, s, tz) {
  try {
    const target = Date.UTC(y, mo - 1, d, h, mi, s);
    let t = target;
    for (let i = 0; i < 2; i++) {
      const parts = new Intl.DateTimeFormat("en-US", {
        timeZone: tz, hour12: false, year: "numeric", month: "2-digit",
        day: "2-digit", hour: "2-digit", minute: "2-digit", second: "2-digit",
      }).formatToParts(new Date(t));
      const g = (k) => +parts.find((p) => p.type === k).value;
      const asUTC = Date.UTC(g("year"), g("month") - 1, g("day"),
                             g("hour") % 24, g("minute"), g("second"));
      t += target - asUTC;
    }
    return t / 1000;
  } catch { return null; }
}

// n-ésimo día-de-semana de un mes (nth>0 desde el inicio, nth<0 desde el
// final: "2TU" = segundo martes, "-1FR" = último viernes). null si no existe.
function nthWeekday(year, month, dow, nth, h, mi) {
  let day;
  if (nth > 0) {
    const first = new Date(year, month, 1);
    day = 1 + ((7 + dow - first.getDay()) % 7) + (nth - 1) * 7;
  } else {
    const last = new Date(year, month + 1, 0);
    day = last.getDate() - ((7 + last.getDay() - dow) % 7) + (nth + 1) * 7;
  }
  const d = new Date(year, month, day, h, mi);
  return d.getMonth() === month ? d : null;
}

// Expande recurrencias (DAILY/WEEKLY/MONTHLY con INTERVAL, BYDAY, BYMONTHDAY,
// UNTIL, COUNT y EXDATE) dentro de la ventana. null = regla no soportada.
function expandEvent(ev, winStart, winEnd) {
  // Sin DTEND o con duracion cero (algunos calendarios lo emiten): 1h
  let dur = (ev.end || ev.start + 3600) - ev.start;
  if (dur <= 0) dur = 3600;
  ev = { ...ev, end: ev.start + dur };
  if (!ev.rrule) {
    return (ev.start >= winStart && ev.start <= winEnd) ? [ev] : [];
  }
  const R = Object.fromEntries(ev.rrule.split(";").map(p => p.split("=")));
  const interval = parseInt(R.INTERVAL || "1", 10);
  const until = R.UNTIL ? icsDate(R.UNTIL) : winEnd;
  let count = R.COUNT ? parseInt(R.COUNT, 10) : Infinity;
  const DMAP = { SU: 0, MO: 1, TU: 2, WE: 3, TH: 4, FR: 5, SA: 6 };
  const base = new Date(ev.start * 1000);
  const out = [];
  const emit = (t) => {
    count--;
    if (t >= winStart && t <= Math.min(until, winEnd) && !ev.exdates.includes(t)) {
      out.push({ ...ev, uid: `${ev.uid}#${t}`, start: t, end: t + dur, rrule: null });
    }
  };

  if (R.FREQ === "MONTHLY") {
    // "el día 15 de cada mes" (BYMONTHDAY) o "el 2º martes" (BYDAY=2TU)
    const byday = R.BYDAY ? R.BYDAY.match(/^(-?\d+)([A-Z]{2})$/) : null;
    if (R.BYDAY && !byday) return null;   // formas exóticas: fuera
    const bymonthday = R.BYMONTHDAY ? parseInt(R.BYMONTHDAY, 10)
                                    : (byday ? null : base.getDate());
    for (let k = 0; k < 24 && count > 0 && out.length < 80; k++) {
      const y = base.getFullYear();
      const m = base.getMonth() + k * interval;
      let occ = null;
      if (bymonthday != null) {
        occ = new Date(y, m, bymonthday, base.getHours(), base.getMinutes());
        if (occ.getDate() !== bymonthday) occ = null;   // día 31 en mes corto
      } else {
        occ = nthWeekday(y, m, DMAP[byday[2]], parseInt(byday[1], 10),
                         base.getHours(), base.getMinutes());
      }
      if (occ === null) continue;
      const t = occ.getTime() / 1000;
      if (t > Math.min(until, winEnd)) break;
      emit(t);
    }
    return out;
  }

  if (R.FREQ !== "DAILY" && R.FREQ !== "WEEKLY") return null;   // YEARLY etc.
  const bydays = R.BYDAY ? R.BYDAY.split(",").map(d => DMAP[d.slice(-2)]) : null;
  const day0 = (d) => { const x = new Date(d); x.setHours(0, 0, 0, 0); return x.getTime(); };
  const weekStart = (d) => day0(new Date(d.getTime() - ((d.getDay() + 6) % 7) * DAY_MS));
  const d = new Date(base);
  for (let it = 0; it < 500 && out.length < 80 && count > 0; it++) {
    const t = d.getTime() / 1000;
    if (t > Math.min(until, winEnd)) break;
    let occurs;
    if (R.FREQ === "DAILY") {
      occurs = Math.round((day0(d) - day0(base)) / DAY_MS) % interval === 0;
    } else {
      const weeks = Math.round((weekStart(d) - weekStart(base)) / (7 * DAY_MS));
      occurs = weeks % interval === 0 &&
               (bydays ? bydays.includes(d.getDay()) : d.getDay() === base.getDay());
    }
    if (occurs) emit(t);
    d.setDate(d.getDate() + 1);
  }
  return out;
}

// Texto ICS → eventos listos para POST /api/events/import. Devuelve también
// cuántos se omitieron (día completo, cancelados, recurrencias no soportadas).
function icsToImportable(text, windowDays = 60) {
  const now = Date.now() / 1000;
  const winEnd = now + windowDays * 86400;
  const out = [];
  let skipped = 0;
  for (const ev of parseICS(text)) {
    if (!ev.uid || !ev.title || !ev.start) { skipped++; continue; }
    if (ev.status === "CANCELLED" || ev.allDay) { skipped++; continue; }
    const ex = expandEvent(ev, now - 3600, winEnd);
    if (ex === null) { skipped++; continue; }
    out.push(...ex);
  }
  return {
    events: out.slice(0, 150).map(e => ({
      uid: e.uid,
      title: e.title.slice(0, 47),
      start: Math.round(e.start),
      end: Math.round(e.end),
    })),
    skipped,
  };
}
