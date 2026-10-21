import hashlib
import json
import shutil
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / '.toolchain'


def download(url, target, digest, headers=None):
    target.parent.mkdir(parents=True, exist_ok=True)
    if not target.exists():
        temporary = target.with_suffix(target.suffix + '.partial')
        with urllib.request.urlopen(urllib.request.Request(url, headers=headers or {}), timeout=60) as src:
            with temporary.open('wb') as dst:
                shutil.copyfileobj(src, dst, 1024 * 1024)
        with temporary.open('rb') as src:
            actual = hashlib.file_digest(src, 'sha256').hexdigest()
        if actual != digest:
            raise RuntimeError(f'Checksum mismatch for {target.name}: {actual}')
        temporary.replace(target)
    with target.open('rb') as src:
        if hashlib.file_digest(src, 'sha256').hexdigest() != digest:
            raise RuntimeError(f'Existing file checksum mismatch: {target}')
    print(f'Verified {target.name}', flush=True)


def main():
    lock = json.loads((ROOT / 'foundation.lock.json').read_text())
    sdk = lock['sdk']
    layer = TOOLS / 'downloads/devkitarm-layer.tar.gz'
    if not layer.exists():
        with urllib.request.urlopen('https://auth.docker.io/token?service=registry.docker.io&scope=repository:devkitpro/devkitarm:pull', timeout=30) as reply:
            token = json.load(reply)['token']
        headers = {'Authorization': 'Bearer ' + token}
    else:
        headers = None
    download('https://registry-1.docker.io/v2/devkitpro/devkitarm/blobs/sha256:' + sdk['layer_sha256'], layer, sdk['layer_sha256'], headers)
    emulator = lock['azahar']
    download(emulator['url'], TOOLS / 'downloads/azahar.AppImage', emulator['sha256'])
    base = lock['linux_base']
    download(base['url'], TOOLS / 'downloads/ubuntu-base.tar.gz', base['sha256'])
    for name in ('makerom', 'bannertool'):
        download(lock[name]['url'], TOOLS / 'downloads' / (name + '.zip'), lock[name]['sha256'])


if __name__ == '__main__':
    main()
