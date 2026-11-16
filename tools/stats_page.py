#!/usr/bin/env python3
"""Osmonauts – builds the public statistics page (GitHub Pages) from the archived data.

usage: python3 tools/stats_page.py <data folder> <site folder> [owner/repo]
Active devices per day = increase of the download counter of ping-*.json between two daily
snapshots: every ESP32 controller with the update check enabled downloads this file once a day.
"""
import datetime, html, json, os, sys

data, site = sys.argv[1], sys.argv[2]
repo = sys.argv[3] if len(sys.argv) > 3 else 'MoManufaktur/Osmonauts-Controller'
os.makedirs(site, exist_ok=True)


def load(name, default):
    p = os.path.join(data, name)
    return json.load(open(p)) if os.path.exists(p) else default


downloads, views, clones, summary = load('downloads.json', {}), load('views.json', {}), load('clones.json', {}), load('summary.json', {})
days = sorted(downloads)

# active devices per day and variant
active, variants = [], set()
for prev, day in zip(days, days[1:]):
    per = {}
    for asset, count in downloads[day].items():
        name = asset.split('/', 1)[1]
        if name.startswith('ping-'):
            variant = name[5:].rsplit('.', 1)[0]
            variants.add(variant)
            per[variant] = per.get(variant, 0) + max(0, count - downloads[prev].get(asset, 0))
    active.append((day, per))
active_total = [(d, sum(p.values())) for d, p in active]
last_active = active_total[-1][1] if active_total else 0
week = [v for _, v in active_total[-7:]]
avg7 = round(sum(week) / len(week), 1) if week else 0

# firmware downloads (latest snapshot)
latest = downloads[days[-1]] if days else {}
fw = {}
for asset, count in latest.items():
    tag, name = asset.split('/', 1)
    if not name.startswith('ping-'):
        fw.setdefault(tag, {})[name] = count
fw_total = sum(c for t in fw.values() for c in t.values())


def chart(series, title, color='#1D9E75', series2=None, label2=None):
    """small self-contained SVG line chart: series = [(date, value)]"""
    if not series:
        return f'<div class="chart"><h3>{html.escape(title)}</h3><p class="empty">Noch keine Daten – die Werte erscheinen nach den ersten Tagen.</p></div>'
    W, H, L, B = 720, 220, 44, 28
    vals = [v for _, v in series] + ([v for _, v in series2] if series2 else [])
    vmax = max(4, -(-max(vals) // 4) * 4)   # round up to a multiple of 4 -> whole-number grid lines
    n = len(series)
    x = lambda i: L + (W - L - 12) * (i / max(1, n - 1))
    y = lambda v: H - B - (H - B - 12) * v / vmax
    def path(s):
        return ' '.join(f'{"M" if i == 0 else "L"}{x(i):.1f},{y(v):.1f}' for i, (_, v) in enumerate(s))
    grid = ''.join(f'<line x1="{L}" x2="{W-12}" y1="{y(vmax*k/4):.1f}" y2="{y(vmax*k/4):.1f}" stroke="#e4e7eb"/>'
                   f'<text x="{L-6}" y="{y(vmax*k/4)+4:.1f}" text-anchor="end">{round(vmax*k/4)}</text>' for k in range(5))
    step = max(1, n // 6)
    xl = ''.join(f'<text x="{x(i):.1f}" y="{H-8}" text-anchor="middle">{series[i][0][5:]}</text>' for i in range(0, n, step))
    dots = ''.join(f'<circle cx="{x(i):.1f}" cy="{y(v):.1f}" r="3" fill="{color}"><title>{d}: {v}</title></circle>' for i, (d, v) in enumerate(series))
    second = ''
    if series2:
        second = f'<path d="{path(series2)}" fill="none" stroke="#3B8BD4" stroke-width="2" stroke-dasharray="5 4"/>'
    legend = f'<div class="legend"><span style="color:{color}">━ {html.escape(title)}</span>' + (f' <span style="color:#3B8BD4">┅ {html.escape(label2)}</span>' if series2 else '') + '</div>'
    return (f'<div class="chart"><h3>{html.escape(title)}</h3><svg viewBox="0 0 {W} {H}" role="img" aria-label="{html.escape(title)}">'
            f'{grid}{xl}<path d="{path(series)}" fill="none" stroke="{color}" stroke-width="2.5"/>{second}{dots}</svg>{legend}</div>')


views_s = [(d, views[d]['count']) for d in sorted(views)]
uniq_s = [(d, views[d]['uniques']) for d in sorted(views)]
clones_s = [(d, clones[d]['count']) for d in sorted(clones)]

cards = [
    ('Aktive Geräte', last_active, 'gestern (Update-Checks)'),
    ('Ø 7 Tage', avg7, 'aktive Geräte pro Tag'),
    ('Gemeldete Nachbauten', summary.get('build_reports', 0), '„Register your build“'),
    ('Firmware-Downloads', fw_total, 'alle Releases'),
    ('Stars', summary.get('stars', 0), 'auf GitHub'),
    ('Forks', summary.get('forks', 0), 'eigene Weiterentwicklungen'),
]
card_html = ''.join(f'<div class="card"><b>{v}</b><span>{html.escape(t)}</span><small>{html.escape(s)}</small></div>' for t, v, s in cards)
rows = ''.join(f'<tr><td>{html.escape(tag)}</td><td>{html.escape(name)}</td><td>{c}</td></tr>'
               for tag in sorted(fw, reverse=True) for name, c in sorted(fw[tag].items()))
variant_rows = ''.join(f'<tr><td>{html.escape(v)}</td><td>{active[-1][1].get(v, 0) if active else 0}</td></tr>' for v in sorted(variants))

page = f'''<!DOCTYPE html>
<html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Osmonauts – Statistik</title>
<style>
body{{margin:0;font-family:Helvetica,Arial,sans-serif;background:#f3f5f7;color:#1f2933}}
header{{background:#172a40;color:#fff;padding:22px 20px}} header h1{{margin:0;font-size:24px}} header p{{margin:4px 0 0;opacity:.75;font-size:14px}}
header a{{color:#9fe1cb}}
main{{max-width:900px;margin:0 auto;padding:16px;display:grid;gap:16px}}
.cards{{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px}}
.card{{background:#fff;border-radius:12px;padding:14px;box-shadow:0 1px 4px rgba(0,0,0,.08);display:flex;flex-direction:column}}
.card b{{font-size:28px;color:#1D9E75}} .card span{{font-weight:700;font-size:14px}} .card small{{color:#6b7785;font-size:12px}}
.chart,.box{{background:#fff;border-radius:12px;padding:14px 16px;box-shadow:0 1px 4px rgba(0,0,0,.08)}}
.chart h3,.box h3{{margin:0 0 8px;font-size:15px}} svg{{width:100%;height:auto;font-size:11px;fill:#6b7785}}
.legend{{font-size:12px;color:#6b7785}} .empty{{color:#6b7785;font-size:14px}}
table{{width:100%;border-collapse:collapse;font-size:14px}} td,th{{text-align:left;padding:5px 6px;border-bottom:1px solid #eef0f2}}
.note{{font-size:13px;color:#6b7785;line-height:1.6}} .links a{{display:inline-block;margin:4px 12px 4px 0}}
footer{{text-align:center;font-size:12px;color:#6b7785;padding:10px 0 24px}}
</style></head><body>
<header><h1>Osmonauts – OpenOsmosis Controller</h1>
<p>Statistik · Stand {html.escape(summary.get("updated", datetime.date.today().isoformat()))} · <a href="https://github.com/{repo}">GitHub</a></p></header>
<main>
<div class="cards">{card_html}</div>
{chart(active_total, "Aktive Geräte pro Tag")}
{chart(views_s, "Seitenaufrufe pro Tag", "#1D9E75", uniq_s, "eindeutige Besucher")}
{chart(clones_s, "Klone (Downloads des Codes) pro Tag", "#7F77DD")}
<div class="box"><h3>Aktive Geräte nach Hardware (gestern)</h3>
<table><tr><th>Variante</th><th>Geräte</th></tr>{variant_rows or '<tr><td colspan="2">noch keine Daten</td></tr>'}</table></div>
<div class="box"><h3>Firmware-Downloads nach Release</h3>
<table><tr><th>Release</th><th>Datei</th><th>Downloads</th></tr>{rows or '<tr><td colspan="3">noch kein Release</td></tr>'}</table></div>
<div class="box links"><h3>Mehr</h3>
<a href="https://github.com/{repo}/issues/new?template=build-report.yml">Eigenen Nachbau melden</a>
<a href="animation.html">3D-Animation der Spülvorgänge</a>
<a href="blockschaltbild.png">Blockschaltbild</a>
<a href="https://github.com/{repo}">Code &amp; Anleitung</a></div>
<p class="note">So wird gezählt: Jeder ESP32-Controller, bei dem die Update-Prüfung eingeschaltet ist, lädt einmal am Tag
eine kleine Datei aus dem neuesten Release. GitHub zählt diese Downloads, wir sehen nur die Summe. Es werden keine Geräte-,
Standort- oder Nutzungsdaten übertragen. Arduino-Uno/Mega-Controller haben kein Netzwerk und werden nicht gezählt.</p>
</main>
<footer>Osmonauts – OpenOsmosis Controller · Kai Mommsen &amp; Elias Dörre · CC BY-NC-SA 4.0</footer>
</body></html>'''
open(os.path.join(site, 'index.html'), 'w', encoding='utf-8').write(page)
print('statistics page written to', site)
