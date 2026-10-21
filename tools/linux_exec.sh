#!/bin/sh
set -eu
base=${FOXHOLEN3DS_CACHE:-/home/user/.cache/foxholen3ds}
root=$base/noble
workspace=$(cd "$(dirname "$0")/.." && pwd)
(
    flock 9
    for mapping in "$workspace:work" "$base/sdk:sdk" "$base/azahar/squashfs-root:emulator" /dev:dev /proc:proc; do
        source=${mapping%:*}
        destination=$root/${mapping##*:}
        if ! mountpoint -q "$destination"; then mount --bind "$source" "$destination"; fi
    done
) 9>"$base/mount.lock"
exec chroot "$root" /usr/bin/env -i PATH=/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin DEVKITPRO=/sdk HOME=/root LANG=C.UTF-8 "$@"
