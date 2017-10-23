#!/bin/sh

root=$(dirname $(realpath "$0"))
mount_point=${root}/mnt

modprobe -r g_mass_storage
fusermount -u "${mount_point}"
