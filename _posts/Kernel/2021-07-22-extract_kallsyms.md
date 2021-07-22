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

2. binwalk 통한 이미지 확인

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

해당 이미지를 확인하고 zlib, LZMA로 compressed 된 것을 확인하고 이건 zImage라고 가정하고 압축해제 결과 cpio는 archive만 존재하고 zlib자체는 압축이 풀리지도 않음, lzma도 정상적인 lzma가 아니라고 나옴. 정상적인 커널 이미지가 아닌건지?

```zsh
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

가정을 몇 개 세워봄   
> 정상적으로 추출된 커널이 아니다.(툴에 오류가 있다)   
> uImage / zImage가 아니다   
> 내가 뭔가를 잘못 알고 있다.   

3. Xiaomi open source 에서 apollo관련 빌드 환경 셋업을 통한 정상 추출 커널인지를 확인

처음엔 빌드가 잘 되지 않았지만, dtc관련 에러 수정을 통하여 빌드 성공.   
빌드 성공하여 나온 Image에 binwalk

```zsh
 tty1538  ~/data/project/xiaomi/Xiaomi_Kernel_OpenSource/out/arch/arm64/boot  ➦ acee5ab68148 ±  binwalk Image

DECIMAL       HEXADECIMAL     DESCRIPTION
--------------------------------------------------------------------------------
125992        0x1EC28         Zlib compressed data, best compression
199872        0x30CC0         SHA256 hash constants, little endian
22568960      0x1586000       ELF, 64-bit LSB shared object, version 1 (SYSV)
22575568      0x15879D0       SHA256 hash constants, little endian
22592328      0x158BB48       gzip compressed data, maximum compression, from Unix, last modified: 1970-01-01 00:00:00 (null date)
22821760      0x15C3B80       CRC32 polynomial table, little endian
23055607      0x15FCCF7       Intel x86 or x64 microcode, sig 0x01010000, pf_mask 0x4011e30, 1E30-04-01, rev 0x33525800, size 2048
...
24685160      0x178AA68       Certificate in DER format (x509 v3), header length: 4, sequence length: 676
24803301      0x17A77E5       Certificate in DER format (x509 v3), header length: 4, sequence length: 224
27398051      0x1A20FA3       Unix path: /lib/zlib_deflate/deflate.c
...
28421761      0x1B1AE81       Copyright string: "Copyright 2005-2007 Rodolfo Giometti <giometti@linux.it>"
28505887      0x1B2F71F       Unix path: /sys/kernel/debug/fg_sram
28626149      0x1B4CCE5       Copyright string: "Copyright(c) Pierre Ossman"
28671829      0x1B57F55       Unix path: /sys/firmware/devicetree/base
28675802      0x1B58EDA       Unix path: /sys/firmware/fdt': CRC check failed
29832898      0x1C736C2       Neighborly text, "neighbor table overflow!s %x"
29896668      0x1C82FDC       Neighborly text, "NeighborSolicitsdev_snmp6"
29896685      0x1C82FED       Neighborly text, "NeighborAdvertisementsnuse %d"
29904692      0x1C84F34       Neighborly text, "neighbor %.2x%.2x.%pM lostport %u(%s) forward delay timer"
29957571      0x1C91DC3       Neighborly text, "NeighborhHWMPactivePathTimeout"
31574272      0x1E1C900       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: ECB, keymode: 8bit
31807744      0x1E55900       Certificate in DER format (x509 v3), header length: 4, sequence length: 1353
31840684      0x1E5D9AC       ASCII cpio archive (SVR4 with no CRC), file name: "dev", file name length: "0x00000004", file size: "0x00000000"
31840800      0x1E5DA20       ASCII cpio archive (SVR4 with no CRC), file name: "dev/console", file name length: "0x0000000C", file size: "0x00000000"
31840924      0x1E5DA9C       ASCII cpio archive (SVR4 with no CRC), file name: "root", file name length: "0x00000005", file size: "0x00000000"
31841040      0x1E5DB10       ASCII cpio archive (SVR4 with no CRC), file name: "TRAILER!!!", file name length: "0x0000000B", file size: "0x00000000"
37672584      0x23ED688       LZMA compressed data, properties: 0xC8, dictionary size: 536870912 bytes, uncompressed size: 78 bytes
39491599      0x25A980F       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
39632359      0x25CBDE7       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
39641879      0x25CE317       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
39748359      0x25E8307       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
39817279      0x25F903F       mcrypt 2.2 encrypted data, algorithm: blowfish-448, mode: CBC, keymode: 8bit
```

확인 결과 기존 이미지와 동일한 형태인 것을 확인 => 커널은 정상적으로 추출 되었다.


4. Image build script 확인

확인 결과 Image는 zImage가 아니다. arm 32비트까지는 zImage가 기본 이미지로 설정되었지만 64비트는 zImage생성을 하지 않고 gzip compressed를 할지 말지만 결정한다.   
(이상한건 bz가 아니라 그냥 gzip만 한다.)

> arch/arm/Makefile
```makefile
# Default target when executing plain make
boot := arch/arm/boot
ifeq ($(CONFIG_XIP_KERNEL),y)
KBUILD_IMAGE := $(boot)/xipImage
else ifeq ($(CONFIG_BUILD_ARM_APPENDED_DTB_IMAGE),y)
KBUILD_IMAGE := $(boot)/zImage-dtb
else
KBUILD_IMAGE := $(boot)/zImage
endif
```

> arch/arm64/Makefile
```makefile
ifeq ($(CONFIG_BUILD_ARM64_KERNEL_COMPRESSION_GZIP),y)
KBUILD_IMAGE   := Image.gz
else
KBUILD_IMAGE   := Image
endif
```

해당 Image와 kallsyms 등을 만드는 순서는 다음과 같다.

- 기존 vmlinux에서 kallsyms를 통하여 모든 symbol을 nm으로 빼 오고 해당 symbol을 .s 파일로 생성
- .s로 파일된 파일을 컴파일하여 .o 파일로 변환
- vmlinux_link를 통하여 해당 .o파일을 vmlinux 끝에 rodata로 링크시킴
- vmlinux를 objcopy로 -s를 통하여 symbol strip을 한 binary를 Image로 저장
- 결론적으로 해당 Image에는 elf포맷으로 변환해도 symbol은 없음. 다만 rodata속에 kallsyms 구조체가 들어가 있으므로 파싱이 가능

```zsh
# nm .tmp_vmlinux1 and run scripts/kallsyms to make symbol with .s file 
# and then gcc that .s file to .tmp_kallsyms1.o
kallsyms .tmp_vmlinux1 .tmp_kallsyms1.o

# link that symbol.o file to vmlinux with .ro.data
vmlinux_link .tmp_kallsyms1.o .tmp_vmlinux2

# same thing with tmp_vmlinux2
kallsyms .tmp_vmlinux2 .tmp_kallsyms2.o

# check two .o file size is same
/bin/bash ../scripts/file-size.sh .tmp_kallsyms1.o
size1=2432632
/bin/bash ../scripts/file-size.sh .tmp_kallsyms2.o
size2=2432632
'[' 2432632 -ne 2432632 ']'

# make system amp with vmlinux
mksysmap vmlinux System.map

# make arch/arm64/boot/Image with objcopy -S(with no symbol)
/home/tty1538/data/project/xiaomi/Xiaomi_Kernel_OpenSource/toolchain/bin/aarch64-linux-android-objcopy  -O binary -R .note -R .note.gnu.build-id -R .comment -S vmlinux arch/arm64/boot/Image

# some modpost script
make -f ../scripts/Makefile.modpost
```

5. kallsyms 추출

해당 kallsyms가 image 내에 구조체 형태로 들어있는것은 확인. 그렇다면 이걸 어떻게 추출할 것이냐.. 바이너리 돌면서 일일히 막 하면..될까?   
vmlinux-to-elf 라는 tool을 찾음. 해당 툴은 바이너리를 돌면서 kallsyms signature를 찾고 해당 table순회하면서 symbol collect하는 함수가 있어 이를 이용하여 추출

```zsh
 ✘ tty1538  ~/data/utils/vmlinux-to-elf   master  ./kallsyms-finder exaid.img-kernel 
[+] Version string: Linux version 4.19.81-perf-g320179f (builder@c5-miui-ota-bd16.bj) (clang version 8.0.12 for Android NDK) #1 SMP PREEMPT Fri Sep 25 23:42:47 CST 2020
[+] Guessed architecture: aarch64 successfully in 4.51 seconds
[+] Found relocations table at file offset 0x22abcf0 (count=137627)
[+] Found kernel text candidate: 0xffffff8008080000
[+] Successfully applied 137616 relocations.
[+] Found kallsyms_token_table at file offset 0x01dd4a00
[+] Found kallsyms_token_index at file offset 0x01dd4e00
[+] Found kallsyms_markers at file offset 0x01dd3900
...
ffffff800a824a40 d default_dump_filter
ffffff800a824a48 D panic_on_oops
ffffff800a824a4c D panic_timeout
ffffff800a824a50 D panic_cpu
ffffff800a824a58 d trace_event_type_funcs_cpuhp_enter
ffffff800a824a78 d print_fmt_cpuhp_enter
ffffff800a824ad0 d event_cpuhp_enter
ffffff800a824b60 d trace_event_type_funcs_cpuhp_multi_enter
ffffff800a824b80 d print_fmt_cpuhp_multi_enter
ffffff800a824bd8 d event_cpuhp_multi_enter
ffffff800a824c68 d trace_event_type_funcs_cpuhp_exit
ffffff800a824c88 d print_fmt_cpuhp_exit
ffffff800a824ce0 d event_cpuhp_exit
ffffff800a824d70 d trace_event_type_funcs_cpuhp_latency
ffffff800a824d90 d print_fmt_cpuhp_latency
ffffff800a824e00 d event_cpuhp_latency
...
```