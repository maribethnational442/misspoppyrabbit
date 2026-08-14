// ============================================================================
// content.js — captura pasiva de eventos desde la pestaña del calendario.
// Funciona en Google Calendar y Outlook Web, en ingles y espanol, SIN tocar
// sus DOMs propietarios: busca elementos cuyo aria-label (la capa de
// accesibilidad, lo mas estable que tienen) parsee como fecha + rango
// horario. Mientras navegas tus semanas, acumula lo que ves; el popup lo
// envia al dispositivo.
// ============================================================================

const MONTHS = {
  // ingles
  january: 1, february: 2, march: 3, april: 4, may: 5, june: 6, july: 7,
  august: 8, september: 9, october: 10, november: 11, december: 12,
  // espanol
  enero: 1, febrero: 2, marzo: 3, abril: 4, mayo: 5, junio: 6, julio: 7,
  agosto: 8, septiembre: 9, octubre: 10, noviembre: 11, diciembre: 12,
};

// "October 14, 2025" | "August 14" (Google suele OMITIR el año) |
// "14 de octubre de 2025" | "14 de agosto". Sin año se infiere: si la fecha
// quedaria a mas de 2 meses en el pasado, es del año siguiente.
function parseDate(label) {
  let y = null, mo = null, d = null;
  for (const m of label.matchAll(/([A-Za-zÀ-ú]+)\s+(\d{1,2})(?:,?\s+(\d{4}))?/g)) {
    const month = MONTHS[m[1].toLowerCase()];
    if (month) { mo = month; d = +m[2]; y = m[3] ? +m[3] : null; break; }
  }
  if (mo === null) {
    for (const m of label.matchAll(/(\d{1,2})\s+de\s+([a-zA-Záéíóúñ]+)(?:\s+de\s+(\d{4}))?/gi)) {
      const month = MONTHS[m[2].toLowerCase()];
      if (month) { mo = month; d = +m[1]; y = m[3] ? +m[3] : null; break; }
    }
  }
  if (mo === null) return null;
  if (y === null) {
    const now = new Date();
    y = now.getFullYear();
    if (new Date(y, mo - 1, d).getTime() < now.getTime() - 60 * 86400000) y += 1;
  }
  return { y, mo, d };
}

// Tokens de hora: "10:30", "10:30am", "10 AM", "10:30 p.m." — un numero suelto
// sin :mm ni am/pm NO es hora (seria el dia del mes).
const TIME_TOKEN = /(\d{1,2}):(\d{2})\s*([ap])\.?\s?\.?m?\.?\b|(\d{1,2})\s*([ap])\.?\s?m\.?\b|(\d{1,2}):(\d{2})/gi;

function tokenTo24h(m) {
  let h, min, mer;
  if (m[1] !== undefined) { h = +m[1]; min = +m[2]; mer = m[3]; }
  else if (m[4] !== undefined) { h = +m[4]; min = 0; mer = m[5]; }
  else { h = +m[6]; min = +m[7]; mer = null; }
  if (mer) {
    const p = mer.toLowerCase() === "p";
    if (p && h < 12) h += 12;
    if (!p && h === 12) h = 0;
  }
  return { h, min, hadMer: !!mer };
}

function parseTimes(label) {
  const tokens = [...label.matchAll(TIME_TOKEN)].slice(0, 2);
  if (tokens.length === 2) {
    const a = tokenTo24h(tokens[0]);
    const b = tokenTo24h(tokens[1]);
    // "2:30 – 3:20pm" = ambos PM; "11 to 1pm" = 11am; "8:30 to 12pm" = 8:30am.
    // Regla: si el fin es PM pasada la 1pm y el inicio (en 12h) no lo supera,
    // comparten meridiano.
    if (!a.hadMer && b.hadMer && b.h > 12 && a.h < 12 && a.h <= b.h - 12) {
      a.h += 12;
    }
    return { a, b };
  }
  // "10 to 11am" donde el 10 no matchea como token: forma especial
  const m = label.match(/(\d{1,2})\s*(?:to|a|hasta|–|—|-)\s*(\d{1,2})\s*([ap])\.?\s?m\.?\b/i);
  if (m) {
    const mer = m[3].toLowerCase();
    let h1 = +m[1], h2 = +m[2];
    if (mer === "p") { if (h2 < 12) h2 += 12; if (h1 < 12 && h1 <= h2 - 12) h1 += 12; }
    return { a: { h: h1, min: 0 }, b: { h: h2, min: 0 } };
  }
  return null;
}

// Titulo: en Outlook el aria-label suele EMPEZAR por el titulo; en Google
// suele venir tras el rango horario. Se prueban los primeros segmentos del
// label (rechazando los que parseen como hora/fecha) y el texto del chip.
function pickTitle(label, text) {
  for (const seg of label.split(",").slice(0, 2).map((s) => s.trim())) {
    if (seg.length > 2 && !parseTimes(seg) && !parseDate(seg) && !/\d{4}/.test(seg)) {
      return seg.slice(0, 47);
    }
  }
  const t = (text || "")
    .replace(/^[\s,–—-]*(\d{1,2}(:\d{2})?\s*([ap]\.?\s?m\.?)?\s*[–—a-]{0,5}\s*){1,2}/i, "")
    .replace(/\s+/g, " ")
    .trim();
  if (t.length > 2) return t.slice(0, 47);
  return null;
}

// API principal (pura, testeable): aria-label + textContent → evento o null
function parseCandidate(label, text) {
  if (!label || label.length < 12 || !/\d/.test(label)) return null;
  const date = parseDate(label);
  if (!date) return null;
  const times = parseTimes(label);
  if (!times) return null;   // sin rango horario = dia completo u otra cosa
  const title = pickTitle(label, text);
  if (!title) return null;

  const start = new Date(date.y, date.mo - 1, date.d, times.a.h, times.a.min).getTime() / 1000;
  let end = new Date(date.y, date.mo - 1, date.d, times.b.h, times.b.min).getTime() / 1000;
  if (end <= start) end += 86400;   // cruza medianoche (raro, pero valido)
  if (end - start > 12 * 3600) return null;   // sospechoso: mejor no capturar
  return { title, start, end };
}

// hash djb2 → uid estable: mismo titulo+hora = mismo uid = sin duplicados
function hashUid(s) {
  let h = 5381;
  for (let i = 0; i < s.length; i++) h = ((h * 33) ^ s.charCodeAt(i)) >>> 0;
  return h.toString(36);
}

// En el Google Calendar actual los chips de evento NO llevan aria-label:
// el texto accesible vive en un span oculto DENTRO del chip. Se busca el
// texto mas corto del subarbol que contenga fecha + rango horario.
// (Fuera del guard DOM: el bookmarklet de Safari también la usa.)
function bestLabelFrom(el) {
  let best = null;
  for (const n of [el, ...el.querySelectorAll("*")]) {
    const t = (n.textContent || "").replace(/\s+/g, " ").trim();
    if (t.length < 12 || t.length > 400) continue;
    if (!parseDate(t) || !parseTimes(t)) continue;
    if (!best || t.length < best.length) best = t;
  }
  return best;
}

// --- Parte con DOM (no corre en los tests de Node) --------------------------
if (typeof document !== "undefined" && typeof chrome !== "undefined" && chrome.storage) {
  let pending = false;
  let observer = null;

  async function scan() {
    pending = false;
    // Si la extension fue recargada, este script quedo huerfano (sin puente
    // chrome.*): se apaga solo. La pestaña recargada traera el script nuevo.
    if (!chrome.runtime?.id) {
      observer?.disconnect();
      return;
    }
    const found = {};
    const add = (parsed) => {
      const uid = `cap-${hashUid(parsed.title + "|" + parsed.start)}`;
      found[uid] = { uid, ...parsed };
    };

    let ariaCands = 0;
    for (const el of document.querySelectorAll("[aria-label]")) {
      const label = el.getAttribute("aria-label");
      if (label && label.length > 12 && /\d/.test(label)) ariaCands++;
      const parsed = parseCandidate(label, el.textContent);
      if (parsed) add(parsed);
    }

    // Chips de evento (Google): parsear su span de accesibilidad interno
    const chips = document.querySelectorAll("[data-eventid]");
    for (const el of chips) {
      const label = bestLabelFrom(el);
      if (!label) continue;
      const parsed = parseCandidate(label, "");
      if (parsed) add(parsed);
    }

    // Diagnóstico visible en la consola DevTools de la pestaña del calendario
    const now0 = Date.now() / 1000;
    const past = Object.values(found).filter((e) => e.start < now0 - 2 * 86400).length;
    console.info(`[MissPoppy v4] scan: ${ariaCands} aria-candidatos, ${chips.length} chips data-eventid, ` +
                 `${Object.keys(found).length} parseados (${past} ya pasados: se descartan)`);
    if (!Object.keys(found).length) return;

    try {
      const { captured = {} } = await chrome.storage.local.get("captured");
      Object.assign(captured, found);
      // Poda: eventos ya pasados (2 dias de gracia) y tope de seguridad
      const now = Date.now() / 1000;
      for (const k of Object.keys(captured)) {
        if (captured[k].start < now - 2 * 86400) delete captured[k];
      }
      const keys = Object.keys(captured);
      if (keys.length > 600) {
        keys.sort((a, b) => captured[b].start - captured[a].start)
            .slice(600).forEach((k) => delete captured[k]);
      }
      await chrome.storage.local.set({ captured });
    } catch (e) {
      // Contexto invalidado en pleno vuelo (extension recargada): apagarse
      observer?.disconnect();
    }
  }

  observer = new MutationObserver(() => {
    if (!pending) {
      pending = true;
      setTimeout(scan, 1500);   // deja que la vista termine de pintar
    }
  });
  observer.observe(document.body, { subtree: true, childList: true });

  setTimeout(scan, 3000);   // barrido inicial al abrir la pestaña
}
