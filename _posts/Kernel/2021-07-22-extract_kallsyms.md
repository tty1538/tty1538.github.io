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