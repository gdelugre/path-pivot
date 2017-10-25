#!/bin/sh

if [ "$#" -ne 1 ] || ! [ -f "$1" ]; then
    echo "Usage: $0 <disk image>" >&2
    exit 1
fi

disk="$1"
root="$(dirname $(realpath "$0"))"
mount_point="${root}/mnt"

[ -d ${mount_point} ] || mkdir ${mount_point}

mkfs.ext3 -q "$disk" || exit 1
mount "$disk" "$mount_point" || exit 1

cd "$mount_point"

for i in `seq 1 39`; do
    echo "Creating directory level $i..."
    mkdir magic_$i
    ln -s ./magic_$i/ xxx$i

    for j in `seq 1 256`; do
        touch entry_$RANDOM
    done

    cd xxx$i
    dd if=/dev/urandom of=$RANDOM.bin bs=4M count=1 status=none
done

umount "$mount_point"
