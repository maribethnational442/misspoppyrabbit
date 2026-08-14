#!/usr/bin/env python3
# ============================================================================
# gen_mockups.py — regenera los mockups SVG de docs/screenshots/ con la
# paleta y sprites reales del firmware. Correr tras cambios visuales grandes:
#   python3 tools/gen_mockups.py
# ============================================================================
import pathlib

OUT = pathlib.Path(__file__).parent.parent / "docs" / "screenshots"

BG, FG, POPPY, DIM, STEM, GRAY, DARK = "#000000", "#ffffff", "#E63946", "#8a222a", "#6A994E", "#888888", "#2a2a2a"
AMBER, BLUE = "#E9C46A", "#5FA8D3"
MONO = 'font-family="Menlo,Consolas,monospace"'

RABBIT = [
    "   ##    ##     ", "  ####  ####    ", "  ####  ####    ", "  ####  ####    ",
    "  ####  #### RR ", "  ##########RRRR", " ###########RRRR", " ##  ######  RR ",
    " ##  ######  G  ", " ############G  ", " ######RR#####  ", "  #####RR####   ",
    "  ###########   ", "   #########    ", "    #######     ", "                ",
]
RABBIT_SLEEP = list(RABBIT)
RABBIT_SLEEP[7] = " ############RR "
RABBIT_SLEEP[8] = " ##--######--G  "
CMAP = {"#": FG, "R": POPPY, "r": DIM, "G": STEM, "-": DARK}


def sprite(rows, x, y, s):
    out = []
    for ry, row in enumerate(rows):
        for rx, ch in enumerate(row):
            if ch in CMAP:
                out.append(f'<rect x="{x+rx*s}" y="{y+ry*s}" width="{s}" height="{s}" fill="{CMAP[ch]}"/>')
    return "".join(out)


def text(x, y, s, size, color, anchor="start", bold=False):
    w = ' font-weight="bold"' if bold else ""
    return f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" text-anchor="{anchor}" {MONO}{w}>{s}</text>'


def statusbar(t="14:32"):
    p = [text(4, 10, t, 7, FG)]
    for i, h in enumerate([3, 6, 9]):
        p.append(f'<rect x="{198+i*4}" y="{11-h}" width="3" height="{h}" fill="{STEM}"/>')
    p.append(f'<rect x="216" y="3" width="18" height="8" fill="none" stroke="{FG}" stroke-width="1"/>')
    p.append(f'<rect x="218" y="5" width="11" height="4" fill="{STEM}"/>')
    p.append(f'<rect x="0" y="13" width="240" height="1" fill="{DARK}"/>')
    return "".join(p)


def frame(body, name):
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="480" height="270" viewBox="0 0 240 135" shape-rendering="crispEdges">'
           f'<rect width="240" height="135" rx="4" fill="{BG}"/>{body}</svg>')
    (OUT / f"{name}.svg").write_text(svg)
    print(f"{name}.svg ({len(svg)}B)")


# --- Launcher (7 apps) ------------------------------------------------------
apps = ["Agenda", "Tasks", "Pomodoro", "Notes", "Settings", "Clock", "About"]
b = [statusbar()]
b.append(f'<rect x="4" y="15" width="148" height="15" rx="4" fill="{DIM}" stroke="{POPPY}" stroke-width="1"/>')
for i, a in enumerate(apps):
    col = FG if i == 0 else GRAY
    b.append(f'<rect x="10" y="{17+i*16}" width="9" height="9" fill="none" stroke="{col}" stroke-width="1.5"/>')
    b.append(text(34, 26 + i * 16, a, 7, col))
b.append(sprite(RABBIT, 176, 34, 3))
b.append(f'<rect x="154" y="48" width="16" height="14" fill="none" stroke="{FG}" stroke-width="1.5"/>')
b.append(f'<rect x="154" y="48" width="16" height="4" fill="{POPPY}"/>')
b.append(text(200, 92, "Miss P. Rabbit", 6, POPPY, "middle"))
frame("".join(b), "launcher")

# --- Agenda -----------------------------------------------------------------
b = [statusbar()]
b.append(text(4, 28, "Thu 14 Aug 2026", 11, POPPY, bold=True))
evs = [("09:00-10:00", "Standup Job 1", POPPY, FG), ("11:30-12:30", "Design w/ Client A", BLUE, FG),
       ("15:00", "!Call the bank", AMBER, POPPY), ("16:00-17:30", "Sprint review", STEM, STEM)]
for i, (t, title, bar, col) in enumerate(evs):
    y = 36 + i * 14
    b.append(f'<rect x="4" y="{y}" width="3" height="11" fill="{bar}"/>')
    b.append(text(11, y + 8, f"{t} {title}", 6.5, col))
b.append(text(4, 132, "n:new v:week ,/:day t:today", 6, DARK))
frame("".join(b), "agenda")

# --- Alerta -----------------------------------------------------------------
b = [statusbar()]
b.append(f'<rect x="2" y="16" width="236" height="117" rx="6" fill="none" stroke="{POPPY}" stroke-width="2"/>')
b.append(text(14, 36, "Are you in this meeting?", 10, POPPY, bold=True))
b.append(f'<rect x="14" y="52" width="4" height="24" fill="{BLUE}"/>')
b.append(text(24, 68, "Design w/ Client A", 11, FG, bold=True))
b.append(text(24, 86, "11:30  J1 Client A", 6.5, GRAY))
b.append(text(14, 126, "[ENTER] yes, I&#39;m in   [`] ask in 5 min", 6, DARK))
frame("".join(b), "alert")

# --- Pomodoro ---------------------------------------------------------------
b = [statusbar()]
b.append(f'<circle cx="64" cy="74" r="42" fill="none" stroke="{DARK}" stroke-width="11"/>')
b.append(f'<path d="M 64 32 A 42 42 0 1 1 24.5 88.4" fill="none" stroke="{POPPY}" stroke-width="11"/>')
b.append(text(64, 79, "18:42", 12, FG, "middle", bold=True))
b.append(text(130, 34, "WORK", 12, POPPY, bold=True))
b.append(text(130, 54, "Running...", 6.5, GRAY))
b.append(text(130, 68, "Today: 3 pomodoros", 6.5, STEM))
b.append(text(130, 100, "ENTER: pause", 6, DARK))
b.append(text(130, 112, "r: reset", 6, DARK))
frame("".join(b), "pomodoro")

# --- Reloj ------------------------------------------------------------------
b = [statusbar()]
b.append(text(120, 58, "14:32:07", 24, FG, "middle", bold=True))
b.append(text(120, 80, "Thu 14 Aug 2026", 7, POPPY, "middle"))
for i, (t, title, bar, col) in enumerate([("15:00", "!Call the bank", AMBER, POPPY),
                                          ("16:00", "Sprint review", STEM, GRAY)]):
    y = 92 + i * 14
    b.append(f'<rect x="30" y="{y}" width="3" height="10" fill="{bar}"/>')
    b.append(text(38, y + 8, f"{t} {title}", 6.5, col))
frame("".join(b), "clock")

# --- Notes ------------------------------------------------------------------
b = [statusbar()]
rows = [("mic", "14 Aug 12:30", "> 00:42", STEM), ("txt", "14 Aug 09:15", "Idea: poppy garden", FG),
        ("mic", "13 Aug 18:02", "  01:12", GRAY), ("txt", "12 Aug 21:40", "Call landlord re: rent", GRAY)]
b.append(f'<rect x="2" y="17" width="236" height="14" rx="3" fill="{DIM}"/>')
for i, (kind, when, right, col) in enumerate(rows):
    y = 20 + i * 14
    if kind == "mic":
        b.append(f'<rect x="7" y="{y}" width="4" height="6" fill="{POPPY}"/><rect x="6" y="{y+6}" width="6" height="2" fill="{FG}"/>')
    else:
        b.append(f'<rect x="6" y="{y}" width="7" height="8" fill="none" stroke="{FG}" stroke-width="1"/>')
    b.append(text(18, y + 7, when, 6.5, FG if i == 0 else GRAY))
    b.append(text(116, y + 7, right, 6.5, col))
b.append(text(4, 132, "n:text m:voice d:del ENTER:open", 6, DARK))
frame("".join(b), "notes")

# --- Grabadora --------------------------------------------------------------
b = [statusbar()]
b.append(f'<circle cx="38" cy="48" r="9" fill="{POPPY}"/>')
b.append(text(56, 52, "REC", 12, FG, bold=True))
b.append(text(116, 52, "00:12 / 01:00", 10, GRAY))
b.append(f'<rect x="28" y="74" width="184" height="12" fill="none" stroke="{DARK}"/>')
b.append(f'<rect x="30" y="76" width="118" height="8" fill="{STEM}"/>')
b.append(text(4, 131, "[ENTER] save   [`] discard", 6, DARK))
frame("".join(b), "recorder")

# --- Bloqueo ----------------------------------------------------------------
b = []
b.append(sprite(RABBIT_SLEEP, 24, 44, 3))
b.append(text(96, 72, "14:32", 24, FG, bold=True))
b.append(f'<rect x="102" y="98" width="10" height="7" fill="{FG}"/><rect x="104" y="93" width="6" height="6" fill="none" stroke="{FG}" stroke-width="1.5"/>')
b.append(text(120, 104, "Fn+L to unlock", 6.5, GRAY))
frame("".join(b), "lock")

# --- WebUI ------------------------------------------------------------------
b = [f'<rect x="0" y="0" width="240" height="12" fill="{DARK}"/>']
b.append(text(8, 8, "prabbit.local", 5.5, GRAY))
b.append(sprite(RABBIT, 8, 16, 1))
b.append(text(30, 25, "Miss Poppy Rabbit", 9, FG, bold=True))
b.append(f'<circle cx="228" cy="22" r="3" fill="{STEM}"/>')
b.append(f'<rect x="6" y="36" width="228" height="28" rx="4" fill="none" stroke="{DARK}"/>')
b.append(text(12, 45, "POMODORO", 6, POPPY, bold=True))
b.append(text(12, 58, "18:42", 10, FG, bold=True))
b.append(text(50, 52, "WORK", 5.5, POPPY))
b.append(f'<rect x="80" y="52" width="148" height="4" rx="2" fill="{DARK}"/>')
b.append(f'<rect x="80" y="52" width="100" height="4" rx="2" fill="{POPPY}"/>')
b.append(f'<rect x="6" y="68" width="228" height="30" rx="4" fill="none" stroke="{DARK}"/>')
b.append(text(12, 77, "TASKS", 6, POPPY, bold=True))
for i, (done, t) in enumerate([(True, "Publish on M5Burner"), (False, "Water the poppy")]):
    y = 81 + i * 8
    fill = STEM if done else "none"
    b.append(f'<rect x="12" y="{y}" width="5" height="5" fill="{fill}" stroke="{STEM if done else FG}" stroke-width="1"/>')
    b.append(text(22, y + 5, t, 5.5, GRAY if done else FG))
b.append(f'<rect x="6" y="102" width="228" height="28" rx="4" fill="none" stroke="{DARK}"/>')
b.append(text(12, 111, "NOTES", 6, POPPY, bold=True))
b.append(text(12, 121, "&#127908; 14 Aug 12:30  00:42  &#9654;", 5.5, FG))
b.append(text(150, 121, "&#128221; Idea: poppy garden", 5.5, GRAY))
frame("".join(b), "webui")

print("listo")
