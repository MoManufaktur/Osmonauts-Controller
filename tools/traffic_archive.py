#!/usr/bin/env python3
"""Osmonauts – archives repository statistics permanently (GitHub keeps traffic only 14 days).

usage: REPO=owner/repo GITHUB_TOKEN=... [TRAFFIC_TOKEN=...] python3 tools/traffic_archive.py <folder>

writes into <folder>:
  views.json, clones.json      per day (needs TRAFFIC_TOKEN with "Administration: Read-only")
  referrers.json, paths.json   per run (needs TRAFFIC_TOKEN)
  downloads.json               per run: download counter of every release asset
                               (ping-*.json = daily update checks of ESP32 controllers)
  summary.json                 stars, forks, watchers, build reports, totals
"""
import datetime, json, os, sys, urllib.error, urllib.request

folder = sys.argv[1] if len(sys.argv) > 1 else 'traffic'
repo = os.environ['REPO']
token = os.environ.get('GITHUB_TOKEN', '')
traffic_token = os.environ.get('TRAFFIC_TOKEN', '')
os.makedirs(folder, exist_ok=True)
today = datetime.date.today().isoformat()


def api(path, tok=None):
    headers = {'Accept': 'application/vnd.github+json', 'X-GitHub-Api-Version': '2022-11-28'}
    if tok or token:
        headers['Authorization'] = f'Bearer {tok or token}'
    req = urllib.request.Request(f'https://api.github.com/repos/{repo}{path}', headers=headers)
    with urllib.request.urlopen(req, timeout=30) as r:
        return json.load(r)


def load(name, default):
    p = os.path.join(folder, name)
    return json.load(open(p)) if os.path.exists(p) else default


def save(name, data):
    json.dump(data, open(os.path.join(folder, name), 'w'), indent=1, sort_keys=True)


# traffic (only with TRAFFIC_TOKEN)
if traffic_token:
    try:
        for kind in ('views', 'clones'):
            archive = load(f'{kind}.json', {})
            for day in api(f'/traffic/{kind}', traffic_token)[kind]:      # last 14 days, newest data wins
                archive[day['timestamp'][:10]] = {'count': day['count'], 'uniques': day['uniques']}
            save(f'{kind}.json', archive)
        for kind, path in (('referrers', '/traffic/popular/referrers'), ('paths', '/traffic/popular/paths')):
            archive = load(f'{kind}.json', {})
            archive[today] = api(path, traffic_token)
            save(f'{kind}.json', archive)
    except urllib.error.HTTPError as e:
        print(f'::warning::traffic not available (HTTP {e.code}) - check the secret TRAFFIC_TOKEN')
else:
    print('::notice::no TRAFFIC_TOKEN - views and clones are skipped')

# release downloads (public data)
downloads = load('downloads.json', {})
snapshot = {}
for rel in api('/releases?per_page=100'):
    for asset in rel.get('assets', []):
        snapshot[f"{rel['tag_name']}/{asset['name']}"] = asset['download_count']
downloads[today] = snapshot
save('downloads.json', downloads)

# build reports (issues with label build-report)
reports = api('/issues?labels=build-report&state=all&per_page=100')
info = api('')
views, clones = load('views.json', {}), load('clones.json', {})
save('summary.json', {
    'updated': today,
    'stars': info['stargazers_count'], 'forks': info['forks_count'], 'watchers': info['subscribers_count'],
    'build_reports': len([i for i in reports if 'pull_request' not in i]),
    'views_total': sum(v['count'] for v in views.values()),
    'clones_total': sum(v['count'] for v in clones.values()),
    'days_archived': len(downloads),
})
print('statistics archived for', repo, today)
