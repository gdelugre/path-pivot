#!/bin/sh

if [ "$#" -eq 0 ] || [ "$#" -gt 4 ]; then
    echo "Usage: $0 [--partition] <disk image> [size] [fs]" >&2
    exit 1
fi

if [ "$1" = "--partition" ]; then
    use_partition=1
    shift
else
    use_partition=0
fi

if [ "$#" -ge 2 ]; then
    disk_size="$2"
else
    disk_size="256M"
fi

if [ "$#" -ge 3 ]; then
    disk_fs="$3"
else
    disk_fs="ext3"
fi

disk="$1"
root="$(dirname $(realpath "$0"))"
mount_point="${root}/mnt"
dir_levels=39


# Create the mountpoint directory.
[ -d ${mount_point} ] || mkdir ${mount_point}

# Allocate the file and create the partition.
fallocate -l "$disk_size" "$disk" || exit 1

if [ "$use_partition" -eq 1 ]; then
    echo ';' | sfdisk -q "$disk" || exit 1
fi

# Create the loop device.
loop_dev=$(losetup --find --partscan --show "$disk") || exit 1

if [ "$use_partition" -eq 1 ]; then
    part_dev="${loop_dev}p1"
else
    part_dev="$loop_dev"
fi

# Format and mount the partition.
mkfs.${disk_fs} -q "$part_dev" || exit 1
mount "$part_dev" "$mount_point" || exit 1

cd "$mount_point"

for i in `seq 1 $dir_levels`; do
    echo "Creating directory level $i..."

    # Each x symbolic link points to a directory prefixed with ./magic_.
    mkdir magic_$i
    ln -s ./magic_$i/ xxx$i

    # Create dummy directory entries.
    for x in `seq 1 256`; do
        touch entry_$RANDOM
    done

    cd xxx$i

    # Create a dummy file entry to fill up space.
    dd if=/dev/urandom of=$RANDOM.bin bs=4M count=1 status=none
done

# Dummy target files.
echo "nothing to see here" > maps
echo "nothing to see here" > passwd

cd $root
sync
umount "$mount_point"
losetup -d "$loop_dev"
