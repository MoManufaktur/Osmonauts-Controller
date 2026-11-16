#!/usr/bin/env python3
"""Generates docs/osmonauts_blockschaltbild.svg – usage: python3 tools/blockdiagram.py docs/osmonauts_blockschaltbild.svg
   PNG: rsvg-convert -w 1700 docs/osmonauts_blockschaltbild.svg -o docs/osmonauts_blockschaltbild.png"""
import sys
out = sys.argv[1]
W, H = 1700, 1400
F = "font-family='Helvetica, Arial, sans-serif'"
el = []

COL = {  # fill, stroke, text
    'ctrl': ('#EEEDFE', '#534AB7', '#3C3489'),
    'in':   ('#E1F5EE', '#0F6E56', '#085041'),
    'out':  ('#FAECE7', '#993C1D', '#712B13'),
    'pow':  ('#FAEEDA', '#854F0B', '#633806'),
    'opt':  ('#F1EFE8', '#5F5E5A', '#444441'),
    'water':('#E6F1FB', '#185FA5', '#0C447C'),
    'drain':('#F1EFE8', '#5F5E5A', '#444441'),
    'alarm':('#FCEBEB', '#A32D2D', '#791F1F'),
}

def text(x, y, s, size=13, weight=400, color='#2C2C2A', anchor='middle'):
    s = s.replace('&', '&amp;').replace('<', '&lt;').replace('>', '&gt;')
    el.append(f"<text x='{x}' y='{y}' {F} font-size='{size}' font-weight='{weight}' fill='{color}' text-anchor='{anchor}' dominant-baseline='central'>{s}</text>")

def box(x, y, w, h, kind, title, sub=None, sub2=None, dashed=False, rx=8):
    f, s, t = COL[kind]
    da = " stroke-dasharray='6 4'" if dashed else ""
    el.append(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='{rx}' fill='{f}' stroke='{s}' stroke-width='1.2'{da}/>")
    lines = [l for l in (title, sub, sub2) if l]
    n = len(lines)
    cy = y + h / 2 - (n - 1) * 9
    text(x + w / 2, cy, title, 14, 700, t)
    if sub: text(x + w / 2, cy + 18, sub, 12, 400, s)
    if sub2: text(x + w / 2, cy + 34, sub2, 12, 400, s)

def line(pts, color='#5F5E5A', width=1.4, arrow=True, dashed=False):
    d = 'M' + ' L'.join(f'{x} {y}' for x, y in pts)
    da = " stroke-dasharray='6 4'" if dashed else ""
    m = " marker-end='url(#ar)'" if arrow else ""
    el.append(f"<path d='{d}' fill='none' stroke='{color}' stroke-width='{width}'{da}{m}/>")

def pipe(pts, color='#378ADD', width=5, arrow=True):
    d = 'M' + ' L'.join(f'{x} {y}' for x, y in pts)
    el.append(f"<path d='{d}' fill='none' stroke='{color}' stroke-width='{width}' stroke-linejoin='round' stroke-linecap='round'/>")
    if arrow and len(pts) >= 2:
        (x1, y1), (x2, y2) = pts[-2], pts[-1]
        mx, my = (x1 + x2) / 2, (y1 + y2) / 2
        el.append(f"<path d='M{x1} {y1} L{mx} {my}' fill='none' stroke='{color}' stroke-width='1' marker-end='url(#arw)'/>")

def panel(x, y, w, h, title, sub):
    el.append(f"<rect x='{x}' y='{y}' width='{w}' height='{h}' rx='14' fill='none' stroke='#B4B2A9' stroke-width='1'/>")
    text(x + 20, y + 24, title, 18, 700, '#2C2C2A', 'start')
    text(x + 20, y + 46, sub, 12, 400, '#5F5E5A', 'start')

# ------------------------------------------------------------------ header
text(40, 38, 'Osmonauts – OpenOsmosis Controller · Blockschaltbild', 26, 700, '#2C2C2A', 'start')
text(40, 66, 'Steuerung für Umkehrosmoseanlagen (Arduino) · Firmware v6.0 · Kai Mommsen & Elias Dörre · CC BY-NC-SA 4.0 · github.com/MoManufaktur/osmonauts-controller', 14, 400, '#5F5E5A', 'start')

# ------------------------------------------------------------------ panel A: electrical
AX, AY, AW, AH = 30, 90, 1000, 600
panel(AX, AY, AW, AH, 'A · Elektrik und Signale', 'Pins: config.h (Uno/Nano) · config_elias.h (Nano) · config_kai.h (Mega)')

# power
box(60, 160, 200, 60, 'pow', 'Netzteil Steuerung', 'USB / 7–12 V DC')
box(60, 560, 200, 60, 'pow', 'Netzteil Ventile', 'je nach Ventiltyp')

# inputs
inputs = [
    ('Taster 1', 'kurz: zapfen · lang: Stopp', 'D2 · D3 · 23'),
    ('Taster 2 (optional)', 'kurz: Stopp', 'D3 · – · 25'),
    ('Taster 3', 'lang: Desinfektion', '– · D11 · 27'),
    ('Taster 4', 'lang: Filterwechsel', '– · D2 · 29'),
    ('Aquastop-Sensor', 'Regen- oder Pegelsensor', 'A0 · A7 · A0'),
]
iy = 245
for i, (t, s, p) in enumerate(inputs):
    y = iy + i * 62
    box(60, y, 200, 52, 'in' if i < 4 else 'alarm', t, s)
    text(272, y + 26, p, 11, 400, '#0F6E56' if i < 4 else '#A32D2D', 'start')
    line([(380, y + 26), (418, y + 26)])

# arduino
box(420, 160, 200, 460, 'ctrl', 'Arduino', 'Uno / Nano / Mega 2560', 'Osmonauts Firmware')
line([(260, 190), (418, 190)])
# outputs
outs = [
    ('Relais 1 → Ventil V1', 'Wasserzulauf', 'D4 · D4 · 22', 'out'),
    ('Relais 2 → Ventil V2', 'Filtern / Zapfen', 'D5 · D5 · 24', 'out'),
    ('Relais 3 → Ventil V3', 'Standwasser → Abfluss', 'D6 · D6 · 26', 'out'),
    ('Relais 4 → Ventil V4', 'Membranspülung → Abfluss', 'D7 · D7 · 28', 'out'),
]
oy = 170
for i, (t, s, p, k) in enumerate(outs):
    y = oy + i * 70
    line([(622, y + 30), (668, y + 30)])
    box(670, y, 230, 62, k, t, s, 'Pins ' + p)
# relay module bracket
el.append("<rect x='660' y='160' width='250' height='290' rx='10' fill='none' stroke='#993C1D' stroke-dasharray='5 4'/>")
text(785, 465, '4-Kanal-Relaismodul (aktiv LOW/HIGH per #define)', 11, 400, '#993C1D')
line([(260, 590), (300, 590), (300, 640), (640, 640), (640, 430), (658, 430)], '#854F0B')
text(470, 654, 'Ventilspannung über die Relaiskontakte', 11, 400, '#854F0B')
# hydraulic link
line([(912, 300), (1000, 300), (1000, 458), (1076, 458)], '#185FA5', 1.4, True, True)
text(990, 290, 'zu den Ventilen', 11, 400, '#185FA5', 'end')

# feedback
box(670, 495, 110, 52, 'out', 'Buzzer', 'D9 · D9 · 30')
box(790, 495, 120, 52, 'opt', 'LED-Ring', 'WS2812B · 31', dashed=True)
box(670, 557, 240, 52, 'opt', 'OLED 128×32 (I2C)', 'A4/A5 · – · 20/21', dashed=True)
line([(622, 520), (668, 520)])
line([(622, 580), (668, 580)], dashed=True)
el.append("<path d='M650 520 L650 485 L850 485 L850 493' fill='none' stroke='#5F5E5A' stroke-width='1.4' stroke-dasharray='6 4' marker-end='url(#ar)'/>")
text(470, 674, 'Pin-Angaben: config.h · config_elias.h · config_kai.h   (– = nicht belegt)   gestrichelt = optional', 11, 400, '#5F5E5A', 'start')

# ------------------------------------------------------------------ panel B: hydraulics
BX, BY, BW, BH = 1050, 90, 620, 830
panel(BX, BY, BW, BH, 'B · Hydraulik', 'aus der Relaislogik abgeleitet – bitte mit eurer Verrohrung abgleichen')
P = '#378ADD'; PP = '#1D9E75'; PC = '#888780'
box(1080, 160, 170, 50, 'water', 'Leitungswasser', 'Hauswasser')
box(1080, 250, 170, 50, 'water', 'Sedimentfilter', 'Vorfilter 1')
box(1080, 340, 170, 50, 'water', 'Aktivkohlefilter', 'Vorfilter 2')
box(1080, 430, 170, 56, 'out', 'V1 Zulauf', 'Relais 1')
box(1060, 530, 210, 50, 'opt', 'Injektionsport', 'Rückschlagventil oben', dashed=True)
pipe([(1165, 210), (1165, 248)]); pipe([(1165, 300), (1165, 338)]); pipe([(1165, 390), (1165, 428)])
pipe([(1165, 486), (1165, 528)])
pipe([(1165, 580), (1165, 640), (1300, 640)])
# membrane
pipe([(1165, 580), (1165, 645), (1318, 645)])
el.append("<rect x='1320' y='610' width='280' height='70' rx='35' fill='#E6F1FB' stroke='#185FA5' stroke-width='1.4'/>")
text(1460, 635, 'RO-Membran', 15, 700, '#0C447C')
text(1460, 655, 'Feed → Permeat + Konzentrat', 12, 400, '#185FA5')
# permeate: V2 (tap) left, V3 (standing water) right
pipe([(1440, 610), (1440, 560)], PP, 5, False)
pipe([(1440, 560), (1370, 560), (1370, 446)], PP)
pipe([(1440, 560), (1540, 560), (1540, 446)], PP)
box(1300, 390, 140, 54, 'out', 'V2 Zapfen', 'Relais 2')
box(1470, 390, 140, 54, 'out', 'V3 Standwasser', 'Relais 3')
pipe([(1370, 390), (1370, 330)], PP)
box(1300, 260, 140, 68, 'water', 'Zapfhahn', 'Reinwasser', '(Permeat)')
pipe([(1610, 417), (1645, 417), (1645, 887), (1424, 887)], PC)
# concentrate
pipe([(1580, 676), (1590, 690), (1590, 822)], PC, 5, False)
pipe([(1590, 760), (1562, 760)], PC)
pipe([(1590, 822), (1562, 822)], PC)
box(1420, 735, 140, 50, 'drain', 'Durchfluss-', 'begrenzer')
box(1420, 795, 140, 54, 'out', 'V4 Spülung', 'Relais 4')
pipe([(1420, 760), (1380, 760), (1380, 863)], PC); pipe([(1420, 822), (1380, 822)], PC, 4, False)
box(1260, 865, 160, 44, 'drain', 'Abfluss')
text(1600, 700, 'Konzentrat', 11, 400, '#5F5E5A', 'start')
text(1450, 590, 'Permeat', 11, 400, '#0F6E56', 'start')
text(1175, 610, 'Feed', 11, 400, '#185FA5', 'start')
# legend
lx, ly = 1080, 690
for i, (c, t) in enumerate([(P, 'Feed / Leitungswasser'), (PP, 'Permeat (Reinwasser)'), (PC, 'Konzentrat / Abwasser')]):
    el.append(f"<path d='M{lx} {ly + i * 22} L{lx + 34} {ly + i * 22}' stroke='{c}' stroke-width='5' stroke-linecap='round'/>")
    text(lx + 44, ly + i * 22, t, 12, 400, '#444441', 'start')
text(1080, 770, 'Annahmen: Injektionsport auf der Feed-Seite;', 11, 400, '#A32D2D', 'start')
text(1080, 786, 'Konzentrat über Durchflussbegrenzer,', 11, 400, '#A32D2D', 'start')
text(1080, 802, 'V4 überbrückt ihn bei der Membranspülung.', 11, 400, '#A32D2D', 'start')

# ------------------------------------------------------------------ panel C: modes
CX, CY, CW, CH = 30, 710, 1000, 660
panel(CX, CY, CW, CH, 'C · Betriebsarten (Firmware v6)', 'genau eine Betriebsart aktiv · Stopp: Taster 1 lang oder Taster 2 kurz → alle Ventile zu → Leerlauf')
box(410, 910, 180, 70, 'ctrl', 'LEERLAUF', 'MODE_IDLE')
modes = [
    (70, 780, 'Wasser zapfen', 'Taster 1 kurz'),
    (300, 780, 'Nachspülung', '3 min nach dem Zapfen'),
    (530, 780, 'Auto-Spülung', 'alle 6 h ohne Nutzung'),
    (760, 780, 'Filterwechsel', 'Taster 4 lang'),
]
for x, y, t, s in modes:
    box(x, y, 200, 56, 'in', t, s)
    cx = x + 100
    line([(cx, 838), (cx, 870), (500, 870), (500, 906)], '#0F6E56', 1.2, True)
box(70, 1020, 200, 56, 'in', 'Desinfektion', 'Taster 3 lang')
line([(270, 1048), (300, 1048), (300, 945), (406, 945)], '#0F6E56', 1.2)
box(760, 1020, 200, 56, 'alarm', 'AQUASTOP', '5× nass · bis Reset')
line([(594, 945), (860, 945), (860, 1016)], '#A32D2D', 1.2)
text(728, 933, 'aus jeder Betriebsart', 11, 400, '#A32D2D')
text(500, 1000, 'Zapfen unterbricht Auto-/Nachspülung;', 11, 400, '#5F5E5A')
text(500, 1016, 'während Desinfektion / Filterwechsel abgelehnt', 11, 400, '#5F5E5A')

# valve matrix
mx, my = 60, 1100
cols = ['Schritt', 'V1', 'V2', 'V3', 'V4', 'Elias', 'Kai']
cw = [330, 60, 60, 60, 60, 120, 120]
rows = [
    ('Membranspülung (Vorspülung)*', '●', '', '', '●', '10 s', '15 s'),
    ('Standwasserspülung (Vorspülung)*', '●', '', '●', '', '90 s', '90 s'),
    ('Filtern / Zapfen', '●', '●', '', '', '215 s', '6 min'),
    ('Nachspülung Membran', '●', '', '', '●', '15 s', '20 s'),
    ('Filterwechsel: Membran + Standwasser', '●', '', '(●)', '(●)', '15 + 15 min', '15 + 15 min'),
    ('Desinfektion: H₂O₂ einspritzen (2×)', '', '', '', '●', '2 min', '2 min'),
    ('Desinfektion: Pulse (7×) + Pause', '●', '●', '', '', '8 s / 10 min', '8 s / 10 min'),
    ('Desinfektion: Endspülungen (3×)', '●', '(●)', '(●)', '(●)', 'je 5 min', 'je 5 min'),
]
x = mx
for i, c in enumerate(cols):
    el.append(f"<rect x='{x}' y='{my}' width='{cw[i]}' height='26' fill='#F1EFE8' stroke='#D3D1C7'/>")
    text(x + (10 if i == 0 else cw[i] / 2), my + 13, c, 12, 700, '#444441', 'start' if i == 0 else 'middle')
    x += cw[i]
for r, row in enumerate(rows):
    x = mx; y = my + 26 + r * 26
    for i, c in enumerate(row):
        el.append(f"<rect x='{x}' y='{y}' width='{cw[i]}' height='26' fill='#FFFFFF' stroke='#D3D1C7'/>")
        color = '#993C1D' if i in (1, 2, 3, 4) else '#2C2C2A'
        text(x + (10 if i == 0 else cw[i] / 2), y + 13, c, 12, 700 if i in (1, 2, 3, 4) else 400, color, 'start' if i == 0 else 'middle')
        x += cw[i]
text(mx, my + 26 + len(rows) * 26 + 18, '* entfällt, wenn die letzte Spülung weniger als 15 min zurückliegt   (●) = nacheinander, nicht gleichzeitig   ● = Ventil offen', 11, 400, '#5F5E5A', 'start')

# ------------------------------------------------------------------ panel D: safety
DX, DY = 1050, 940
panel(DX, DY, 620, 430, 'D · Sicherheit und Hinweise', 'gilt für alle Konfigurationen')
notes = [
    'Aquastop: Sensor jede Sekunde, 5 nasse Messungen in Folge →',
    '   alle Ventile zu, Alarmton, Sperre bis Reset (kein Auto-Neustart).',
    'Nach dem Einschalten sind alle Relais AUS (Ventile geschlossen).',
    'Relais werden mit Pause (rst) nacheinander geschaltet → kleinere',
    '   Stromspitzen; in dieser Zeit (3 × rst) reagieren keine Taster.',
    'Desinfektion: H₂O₂ 3 % mit Einwegspritze, Anleitung im Code.',
    '   Nach Abbruch steht ggf. H₂O₂ im System → nächstes Zapfen spült vor.',
    'A6/A7 gibt es nur beim Nano / bei SMD-Boards, nicht beim DIP-Uno.',
    'Ventilnetzteil fachgerecht anschließen; bei Netzspannung',
    '   Elektrofachkraft hinzuziehen. Wasser und Strom trennen!',
    'Hydraulik in Teil B ist aus der Software abgeleitet –',
    '   vor dem Nachbau mit der realen Anlage vergleichen.',
]
for i, n in enumerate(notes):
    text(DX + 24, DY + 80 + i * 24, n, 13, 400, '#2C2C2A', 'start')

svg = f"""<svg xmlns='http://www.w3.org/2000/svg' width='{W}' height='{H}' viewBox='0 0 {W} {H}'>
<title>Osmonauts – OpenOsmosis Controller · Blockschaltbild</title>
<defs>
<marker id='ar' viewBox='0 0 10 10' refX='8' refY='5' markerWidth='7' markerHeight='7' orient='auto-start-reverse'><path d='M2 1L8 5L2 9' fill='none' stroke='context-stroke' stroke-width='1.6' stroke-linecap='round' stroke-linejoin='round'/></marker>
<marker id='arw' viewBox='0 0 10 10' refX='5' refY='5' markerWidth='9' markerHeight='9' orient='auto'><path d='M1 1L7 5L1 9Z' fill='#FFFFFF' stroke='none'/></marker>
</defs>
<rect width='{W}' height='{H}' fill='#FFFFFF'/>
{chr(10).join(el)}
</svg>"""
open(out, 'w').write(svg)
