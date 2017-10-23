#!/bin/sh

disk="$1"
root=$(dirname $(realpath "$0"))

${root}/fuse/bin/path-pivot -t 3 -L /tmp/path_pivot.log --pivot-to /etc -n 6 "$disk" ${root}/mnt
modprobe g_mass_storage file=${root}/mnt/path_pivot.img ro=1

trap ${root}/umount.sh "SIGINT"
tail -f /tmp/path_pivot.log
