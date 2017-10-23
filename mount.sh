#!/bin/sh

if [ "$#" -ne 1 ] || ! [ -f "$1" ]; then
    echo "Usage: $0 <disk image>" >&2
    exit 1
fi

disk="$1"
root=$(dirname $(realpath "$0"))
mount_point=${root}/mnt

[ -d ${mount_point} ] || mkdir ${mount_point}

${root}/fuse/bin/path-pivot -t 3 -L /tmp/path_pivot.log --pivot-to /etc -n 6 "$disk" "${mount_point}"
modprobe g_mass_storage file=${root}/mnt/path_pivot.img ro=1

trap ${root}/umount.sh "SIGINT"
tail -f /tmp/path_pivot.log
