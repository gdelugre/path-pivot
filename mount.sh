#!/bin/sh

if [ "$#" -ne 1 ] || ! [ -f "$1" ]; then
    echo "Usage: $0 <disk image>" >&2
    exit 1
fi

disk="$1"
root="$(dirname $(realpath "$0"))"
mount_point="${root}/mnt"

[ -d ${mount_point} ] || mkdir ${mount_point}

#
# -t: number of seconds to wait when a symlink is followed.
# -L: log file
# --pivot-to: pivot destination of the last symlink
# -n: number of hits required before pivoting the symlink
#
${root}/fuse/bin/path-pivot -t 3 -L /tmp/path_pivot.log --pivot-to /proc/self -n 3 "$disk" "$mount_point"
modprobe g_mass_storage file="${root}/mnt/path_pivot.img" ro=1

trap ${root}/umount.sh "SIGINT"
tail -f /tmp/path_pivot.log
