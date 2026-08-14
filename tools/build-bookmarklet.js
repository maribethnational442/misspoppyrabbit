#!/usr/bin/env node
// ============================================================================
// build-bookmarklet.js — empaqueta el parser de extension/content.js (única
// fuente de verdad) en un bookmarklet `javascript:` y lo inyecta en
// web/index.html entre los marcadores BOOKMARKLET.
//
// El bookmarklet es el camino para navegadores donde no se puede instalar la
// extensión (p.ej. Safari en una máquina corporativa): un clic en la página
// del calendario captura los eventos visibles y los copia como JSON, que se
// pega en la WebUI del dispositivo.
//
// Ejecutar tras cada cambio del parser:  node tools/build-bookmarklet.js
// ============================================================================
const fs = require("fs");
const path = require("path");

const ROOT = path.join(__dirname, "..");
const contentJs = fs.readFileSync(path.join(ROOT, "extension", "content.js"), "utf8");

// Solo la parte pura (hasta el guard del DOM), sin líneas de comentario
const GUARD = "// --- Parte con DOM";
const guardIdx = contentJs.indexOf(GUARD);
if (guardIdx < 0) throw new Error("marcador del guard DOM no encontrado en content.js");
const pure = contentJs.slice(0, guardIdx)
  .split("\n")
  .filter((l) => l.trim() && !l.trim().startsWith("//"))
  .join("\n");

// El colector: escanea, filtra a futuro/60 días y copia el JSON del import
const collector = `
(function(){
  var found = {};
  var add = function(p){ found[p.title + "|" + p.start] = p; };
  document.querySelectorAll("[aria-label]").forEach(function(el){
    var p = parseCandidate(el.getAttribute("aria-label"), el.textContent);
    if (p) add(p);
  });
  document.querySelectorAll("[data-eventid]").forEach(function(el){
    var l = bestLabelFrom(el);
    if (!l) return;
    var p = parseCandidate(l, "");
    if (p) add(p);
  });
  var now = Date.now() / 1000;
  var evs = Object.values(found)
    .filter(function(e){ return e.start >= now - 3600 && e.start <= now + 60 * 86400; })
    .map(function(e){ return { uid: "cap-" + hashUid(e.title + "|" + e.start),
      title: e.title.slice(0, 47), start: Math.round(e.start), end: Math.round(e.end) }; });
  if (!evs.length) { alert("No events found in this view (week view works best)"); return; }
  var json = JSON.stringify({ events: evs });
  var done = function(){ alert("[Miss Poppy] " + evs.length + " events copied - paste them in the rabbit Web UI (Agenda > Paste capture)"); };
  if (navigator.clipboard && navigator.clipboard.writeText) {
    navigator.clipboard.writeText(json).then(done, function(){ prompt("Copy this JSON:", json); });
  } else {
    prompt("Copy this JSON:", json);
  }
})();
`;

const code = pure + "\n" + collector;
new Function(code);   // valida sintaxis antes de publicar
const href = "javascript:" + encodeURIComponent("(function(){" + code + "})()");

// Inyección en la WebUI entre marcadores
const htmlPath = path.join(ROOT, "web", "index.html");
const html = fs.readFileSync(htmlPath, "utf8");
const START = "<!--BOOKMARKLET_START-->";
const END = "<!--BOOKMARKLET_END-->";
const a = html.indexOf(START), b = html.indexOf(END);
if (a < 0 || b < 0) throw new Error("marcadores BOOKMARKLET no encontrados en web/index.html");
const replaced = html.slice(0, a + START.length) +
  `<a id="bm-link" href="${href}">&#128007; Capture bookmarklet</a>` +
  html.slice(b);
fs.writeFileSync(htmlPath, replaced);
console.log(`bookmarklet inyectado: ${(href.length / 1024).toFixed(1)}KB`);
