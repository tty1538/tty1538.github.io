---
title: Extract kallsyms from Xiaomi phone 
date: 2021-07-22 16:14:00 +09:00
categories: [linux kernel]
tags: [kernel build system, kallsyms]
description: How to get Kallsyms from bootimg
---


# Extract kallsyms from Xiaomi phone

- Xiaomi 버전과 binary는 가지고 있음
- rooting하려면 168시를 기다려야만 함
- 바이너리에서 kallsyms를 빼낼 수 있는 방법이 없는지 확인


1. boot.img에서 kernel 이미지 추출
```zsh
 tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images  unpackbootimg -i boot.img -o bootimg_unpackbootimg
BOARD_KERNEL_CMDLINE console=ttyMSM0,115200n8 androidboot.hardware=qcom androidboot.console=ttyMSM0 androidboot.memcg=1 lpm_levels.sleep_disabled=1 video=vfb:640x400,bpp=32,memsize=3072000 msm_rtb.filter=0x237 service_locator.enable=1 androidboot.usbcontroller=a600000.dwc3 swiotlb=2048 loop.max_part=7 cgroup.memory=nokmem,nosocket reboot=panic_warm buildvariant=user
BOARD_KERNEL_BASE 00000000
BOARD_NAME 
BOARD_PAGE_SIZE 4096
BOARD_HASH_TYPE sha1
BOARD_KERNEL_OFFSET 00008000
BOARD_RAMDISK_OFFSET 01000000
BOARD_SECOND_OFFSET 00f00000
BOARD_TAGS_OFFSET 00000100
BOARD_OS_VERSION 10.0.0
BOARD_OS_PATCH_LEVEL 2020-09
BOARD_DT_SIZE 2
 tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images  cd bootimg_unpackbootimg 
 tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images/bootimg_unpackbootimg  ls -al
total 44064
drwxrwxr-x 2 tty1538 tty1538     4096 Jul 22 17:05 .
drwxrwxr-x 8 tty1538 tty1538     4096 Jul 22 17:05 ..
-rw-rw-r-- 1 tty1538 tty1538        9 Jul 22 17:05 boot.img-base
-rw-rw-r-- 1 tty1538 tty1538        1 Jul 22 17:05 boot.img-board
-rw-rw-r-- 1 tty1538 tty1538      348 Jul 22 17:05 boot.img-cmdline
-rw-rw-r-- 1 tty1538 tty1538        2 Jul 22 17:05 boot.img-dtb
-rw-rw-r-- 1 tty1538 tty1538        5 Jul 22 17:05 boot.img-hash
-rw-rw-r-- 1 tty1538 tty1538        9 Jul 22 17:05 boot.img-kerneloff
-rw-rw-r-- 1 tty1538 tty1538        8 Jul 22 17:05 boot.img-oslevel
-rw-rw-r-- 1 tty1538 tty1538        7 Jul 22 17:05 boot.img-osversion
-rw-rw-r-- 1 tty1538 tty1538        5 Jul 22 17:05 boot.img-pagesize
-rw-rw-r-- 1 tty1538 tty1538   814192 Jul 22 17:05 boot.img-ramdisk.gz
-rw-rw-r-- 1 tty1538 tty1538        9 Jul 22 17:05 boot.img-ramdiskoff
-rw-rw-r-- 1 tty1538 tty1538        9 Jul 22 17:05 boot.img-secondoff
-rw-rw-r-- 1 tty1538 tty1538        9 Jul 22 17:05 boot.img-tagsoff
-rw-rw-r-- 1 tty1538 tty1538 44247052 Jul 22 17:05 boot.img-zImage
```

2.binwalk 통한 이미지 확인

```zsh
 tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images/bootimg_unpackbootimg  binwalk boot.img-zImage

DECIMAL       HEXADECIMAL     DESCRIPTION
--------------------------------------------------------------------------------
125992        0x1EC28         Zlib compressed data, best compression
199872        0x30CC0         SHA256 hash constants, little endian
22568960      0x1586000       ELF, 64-bit LSB shared object, version 1 (SYSV)
22575568      0x15879D0       SHA256 hash constants, little endian
22595680      0x158C860       gzip compressed data, maximum compression, from Unix, last modified: 1970-01-01 00:00:00 (null date)
22825472      0x15C4A00       CRC32 polynomial table, little endian
23059319      0x15FDB77       Intel x86 or x64 microcode, sig 0x01010000, pf_mask 0x4011e30, 1E30-04-01, rev 0x33525800, size 2048
...
23129686      0x160EE56       Boot section Start 0x42635842 End 0x6E42
28814696      0x1B7AD68       Certificate in DER format (x509 v3), header length: 4, sequence length: 676
...
31281152      0x1DD5000       Unix path: /home/work/apollo-q-stable-build/kernel/msm-4.19/init/main.c
31284180      0x1DD5BD4       Unix path: /home/work/apollo-q-stable-build/kernel/msm-4.19/init/do_mounts_dm.c
31285278      0x1DD601E       Unix path: /home/work/apollo-q-stable-build/kernel/msm-4.19/init/initramfs.c
...
34221089      0x20A2C21       Unix path: /home/work/apollo-q-stable-build/kernel/msm-4.19/lib/timerqueue.c
36010240      0x2257900       Certificate in DER format (x509 v3), header length: 4, sequence length: 1342
36043236      0x225F9E4       ASCII cpio archive (SVR4 with no CRC), file name: "dev", file name length: "0x00000004", file size: "0x00000000"
36043352      0x225FA58       ASCII cpio archive (SVR4 with no CRC), file name: "dev/console", file name length: "0x0000000C", file size: "0x00000000"
36043476      0x225FAD4       ASCII cpio archive (SVR4 with no CRC), file name: "root", file name length: "0x00000005", file size: "0x00000000"
36043592      0x225FB48       ASCII cpio archive (SVR4 with no CRC), file name: "TRAILER!!!", file name length: "0x0000000B", file size: "0x00000000"
41870984      0x27EE688       LZMA compressed data, properties: 0xC8, dictionary size: 536870912 bytes, uncompressed size: 78 bytes
43693439      0x29AB57F       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
43834199      0x29CDB57       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
43843719      0x29D0087       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
43950239      0x29EA09F       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
44019199      0x29FADFF       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
```

해당 이미지를 확인하고 zlib, LZMA로 compressed 된 것을 확인하고 이건 zImage라고 가정하고 압축해제 결과 cpio는 archive만 존재

```console
tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images/bootimg/_boot.img-zImage.extracted  ls
158D200  1F5C8  1F5C8.zlib  2260384.cpio  27EF028  27EF028.7z  cpio-root
 tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images/bootimg/_boot.img-zImage.extracted  ls -alR cpio-root
cpio-root:
total 16
drwxrwxr-x 4 tty1538 tty1538 4096 Jul 22 17:17 .
drwxrwxr-x 3 tty1538 tty1538 4096 Jul 22 17:17 ..
drwxr-xr-x 2 tty1538 tty1538 4096 Jul 22 17:17 dev
drwx------ 2 tty1538 tty1538 4096 Jul 22 17:17 root

cpio-root/dev:
total 8
drwxr-xr-x 2 tty1538 tty1538 4096 Jul 22 17:17 .
drwxrwxr-x 4 tty1538 tty1538 4096 Jul 22 17:17 ..

cpio-root/root:
total 8
drwx------ 2 tty1538 tty1538 4096 Jul 22 17:17 .
drwxrwxr-x 4 tty1538 tty1538 4096 Jul 22 17:17 ..
 tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images/bootimg/_boot.img-zImage.extracted  file 27EF028
27EF028: data
 tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images/bootimg/_boot.img-zImage.extracted  file 1F5C8.zlib 
1F5C8.zlib: zlib compressed data
 tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images/bootimg/_boot.img-zImage.extracted  zcat 1F5C8.zlib 

gzip: 1F5C8.zlib: not in gzip format
 ✘ tty1538  ~/data/project/binary/xiaomi/12.0.6.0/fastbootrom/apollo_eea_global_images_V12.0.6.0.QJDEUXM_20200926.0000.00_10.0_eea/images/bootimg/_boot.img-zImage.extracted  
```