# Path pivot attack

This is the proof-of-concept files used in the demo of the path pivot attack.
The principle behind this attack is to exploit a TOCTOU race by returning
different results from the drive between between the time a file is checked and
when it is accessed.

More details about the attack can be found in [this blog post](https://gdelugre.github.io/2017/11/06/samba-path-pivot-attack/).

## Usage

1. a disk image has to be created first. The script ``make_fs.sh`` will create
   an ``ext3`` filesystem by default. It is important that the different
   symlink inodes are stored on distant locations on the disk. This script tries
   to create many file entries for that aim, but the resulting disk layout can be
   quite different depending on the filesystem driver used. This script has been
   successfully tested on a ``3.14`` kernel, but has shown some problems with more
   recent kernels.

2. run the mass storage gadget and FUSE handler using ``mount.sh`` (edit the
   file first if you need to adjust some parameters)

3. Trigger a file read to a file ``xxx1/xxx2/.../xxx39/<target>``

## Requirements

* A board with a USB OTG port
* ``cmake``
* ``libfuse``
