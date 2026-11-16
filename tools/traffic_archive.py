#!/usr/bin/env python3
"""Osmonauts – merges the last 14 days of GitHub traffic into a permanent archive.

usage: GH_TOKEN=... REPO=owner/repo python3 tools/traffic_archive.py <folder>
writes <folder>/views.json, clones.json (per day), referrers.json, paths.json (per run), summary.json
"""
import json, os, sys, urllib.request, datetime

folder = sys.argv[1] if len(sys.argv) > 1 else 'traffic'
repo, token = os.environ['REPO'], os.environ.get('GH_TOKEN', '')
if not token:
    sys.exit('GH_TOKEN (secret TRAFFIC_TOKEN) is missing - see .github/workflows/traffic-archive.yml')
os.makedirs(folder, exist_ok=True)

def api(path):
    req = urllib.request.Request(f'https://api.github.com/repos/{repo}/{path}', headers={
        'Authorization': f'Bearer {token}', 'Accept': 'application/vnd.github+json', 'X-GitHub-Api-Version': '2022-11-28'})
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)

def load(name, default):
    p = os.path.join(folder, name)
    return json.load(open(p)) if os.path.exists(p) else default

def save(name, data):
    json.dump(data, open(os.path.join(folder, name), 'w'), indent=1, sort_keys=True)

today = datetime.date.today().isoformat()
for kind in ('views', 'clones'):
    archive = load(f'{kind}.json', {})
    for day in api(f'traffic/{kind}')[kind]:            # 14 days, newest data wins
        archive[day['timestamp'][:10]] = {'count': day['count'], 'uniques': day['uniques']}
    save(f'{kind}.json', archive)
for kind, path in (('referrers', 'traffic/popular/referrers'), ('paths', 'traffic/popular/paths')):
    archive = load(f'{kind}.json', {})
    archive[today] = api(path)
    save(f'{kind}.json', archive)

info = api('')
views, clones = load('views.json', {}), load('clones.json', {})
save('summary.json', {
    'updated': today, 'stars': info['stargazers_count'], 'forks': info['forks_count'], 'watchers': info['subscribers_count'],
    'views_total': sum(v['count'] for v in views.values()), 'clones_total': sum(v['count'] for v in clones.values()),
    'days_archived': len(views)})
print('traffic archived for', repo, today)
