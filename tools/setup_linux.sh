#!/bin/sh
set -eu
base=${FOXHOLEN3DS_CACHE:-/home/user/.cache/foxholen3ds}
root=$base/noble
workspace=$(cd "$(dirname "$0")/.." && pwd)
python3 - "$workspace" <<'PY'
import hashlib, json, sys
from pathlib import Path
workspace=Path(sys.argv[1])
lock=json.loads((workspace/'foundation.lock.json').read_text())
for filename, expected in [('devkitarm-layer.tar.gz',lock['sdk']['layer_sha256']),('azahar.AppImage',lock['azahar']['sha256']),('ubuntu-base.tar.gz',lock['linux_base']['sha256']),('makerom.zip',lock['makerom']['sha256']),('bannertool.zip',lock['bannertool']['sha256'])]:
    digest=hashlib.sha256()
    with (workspace/'.toolchain/downloads'/filename).open('rb') as source:
        for block in iter(lambda:source.read(1024*1024),b''): digest.update(block)
    if digest.hexdigest()!=expected: raise SystemExit('Checksum mismatch: '+filename)
PY
mkdir -p "$base/sdk" "$base/azahar" "$workspace/artifacts/research"
if [ ! -f "$base/sdk/devkitARM/bin/arm-none-eabi-gcc" ]; then
    tar -xzf "$workspace/.toolchain/downloads/devkitarm-layer.tar.gz" -C "$base/sdk" --strip-components=2 opt/devkitpro
fi
if [ ! -f "$base/azahar/squashfs-root/AppRun" ]; then
    cp "$workspace/.toolchain/downloads/azahar.AppImage" "$base/azahar/azahar.AppImage"
    chmod +x "$base/azahar/azahar.AppImage"
    (cd "$base/azahar" && ./azahar.AppImage --appimage-extract > "$workspace/artifacts/research/appimage-extract.log")
fi
mkdir -p "$root"
if [ ! -f "$root/etc/os-release" ]; then
    tar -xzf "$workspace/.toolchain/downloads/ubuntu-base.tar.gz" -C "$root"
fi
mkdir -p "$root/work" "$root/sdk" "$root/emulator" "$root/dev" "$root/proc"
for mapping in "$workspace:work" "$base/sdk:sdk" "$base/azahar/squashfs-root:emulator" /dev:dev /proc:proc; do
    source=${mapping%:*}
    destination=$root/${mapping##*:}
    if ! mountpoint -q "$destination"; then mount --bind "$source" "$destination"; fi
done
cp -L /etc/resolv.conf "$root/etc/resolv.conf"
python3 - "$workspace" "$root/usr/local/bin" <<'PY'
import json, sys, zipfile
from pathlib import Path
workspace, bin_dir = Path(sys.argv[1]), Path(sys.argv[2])
lock = json.loads((workspace/'foundation.lock.json').read_text())
for name in ('makerom', 'bannertool'):
    target = bin_dir/name
    target.write_bytes(zipfile.ZipFile(workspace/'.toolchain/downloads'/(name+'.zip')).read(lock[name]['member']))
    target.chmod(0o755)
PY
printf '#!/bin/sh\nexit 101\n' > "$root/usr/sbin/policy-rc.d"
chmod +x "$root/usr/sbin/policy-rc.d"
chroot "$root" /bin/sh -c 'apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y ca-certificates curl git build-essential clang cmake ninja-build pkg-config xvfb xauth mesa-utils libgl1-mesa-dri libegl1-mesa-dev libvulkan1 mesa-vulkan-drivers libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev libxkbcommon-x11-0 libwayland-dev wayland-protocols libdecor-0-dev libasound2-dev libpulse-dev libudev-dev libdbus-1-dev libdrm-dev libgbm-dev zlib1g-dev libjpeg-dev liblz4-dev libmbedtls-dev libopengl0 libxcb-cursor0 python3 python3-pil python3-numpy x11-apps'
chroot "$root" /bin/sh -c 'DEBIAN_FRONTEND=noninteractive apt-get install -y gdb python3-unicorn python3-pyelftools python3-setuptools; dpkg-query -W > /work/artifacts/research/linux-packages.txt; /sdk/devkitARM/bin/arm-none-eabi-gcc --version'
