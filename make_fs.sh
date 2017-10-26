#!/bin/sh

if [ "$#" -ne 1 ] || ! [ -f "$1" ]; then
    echo "Usage: $0 <disk image>" >&2
    exit 1
fi

disk="$1"
root="$(dirname $(realpath "$0"))"
mount_point="${root}/mnt"
dir_levels=39

[ -d ${mount_point} ] || mkdir ${mount_point}

mkfs.ext3 -q "$disk" || exit 1
mount "$disk" "$mount_point" || exit 1

cd "$mount_point"

for i in `seq 1 $dir_levels`; do
    echo "Creating directory level $i..."

    # Each x symbolic link points to a directory prefixed with ./magic_.
    mkdir magic_$i
    ln -s ./magic_$i/ x$i

    # Create dummy directory entries.
    for x in `seq 1 256`; do
        touch entry_$RANDOM
    done

    cwd=$(pwd)
    mkdir y$i
    cd y$i

    # Create a tree: y{i}/x{i+1}/.../x{n} pointing to "./magic_{n}"
    for j in `seq $[i+1] $[dir_levels-1]`; do
        mkdir x$j
        cd x$j
    done
    ln -s ./magic_${dir_levels}/ x$dir_levels

    cd "${cwd}/x$i"

    # Create a dummy file entry to fill up space.
    dd if=/dev/urandom of=$RANDOM.bin bs=4M count=1 status=none
done

# Dummy target files.
echo "nothing to see here" > maps
echo "nothing to see here" > passwd

umount "$mount_point"
