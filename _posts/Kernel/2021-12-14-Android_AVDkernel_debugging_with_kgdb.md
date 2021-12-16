---
title: Android AVD kernel debugging with kgdb
date: 2021-12-14 09:18:00 +09:00
categories: [linux kernel]
tags: [kernel build system, kgdb]
description: How to use kgdb in AVD kernel
---

# 준비사항

필요한 디펜던시 모듈들과 repo 다운로드
```sh
#############################################|#############################################
######                  Build an AVD Kernel and its Ramdisk Modules                  ######
######                        with the official AOSP Build ENV                       ######
#############################################|#############################################

#############################################|#############################################
######                  Install build dependencies, libs and tools                   ######
#############################################|#############################################

sudo apt-get install -y build-essential libssl-dev kernel-package libncurses5-dev bzip2 \
lib32z1 bison flex libelf-dev qt5-default qttools5-dev-tools qttools5-dev meld geany \
gtk+-2.0 libgtk-3-dev libwebkit2gtk-4.0-dev autogen libgtk2.0-dev libglade2-dev

#############################################|#############################################
######                             Get the AOSP repo bin                             ######
#############################################|#############################################

sudo wget https://storage.googleapis.com/git-repo-downloads/repo -O /usr/bin/repo
sudo chmod a+x /usr/bin/repo
```

# 1. Up to Android 10

안드로이드 10까지의 커널은 기본적으로 커널 빌드만 해도 사용이 가능하다. 다른 ko파일들이 빌드하면서 따로 나오는 것들이 없어서 커널만 바꿔쳐도 사용이 가능함.
다만, kgdb를 사용하기 위한 커널 config만 설정해주면 된다.

```sh
git clone https://android.googlesource.com/kernel/goldfish
cd goldfish
git checkout android-goldfish-4.9-dev
/*download config.new file from below*/
mv config.new ./.config
export ARCH=x86_64
make menuconfig
make -j64
```

| Should On Config            | Should Off Config                         |
|-----------------------------|-------------------------------------------|
| CONFIG_HAVE_ARCH_KGDB(KGDB) | CONFIG_DEBUG_RODATA(rodata read-only off) |
| CONFIG_KGDB                 | CONFIG_RANDOMIZE_BASE(kaslr off)          |
| CONFIG_KGDB_LOW_LEVEL_TRAP  | CONFIG_DEBUG_SET_MODULE_RONX              |
| CONFIG_MAGIC_SYSRQ          |                                           |

다음과 같이 설정한 커널을 안드로이드 emulator로 실행만 시키면 된다. -s 옵션은 시작하면서 gdbserver를 port1234로 실행한다.
```sh
emulator -verbose -show-kernel -debug init -avd test -kernel /home/tty1538/project/kernel_goldfish/goldfish/arch/x86/boot/bzImage -qemu -s
```

| 간단한 kgdb 명령어                                                    | 설명                                        |
|----------------------------------------------------------------------|-------------------------------------------|
| lx-symbols                                                           | Loading symbol                            |
| lx-dmesg                                                             | Printing kernel log in gdb                |
| lx-lsmod                                                             | Printing loaded module                    |
| p $lx_current().pid                                                  | Print Current pid                         |
| p $lx_current().comm                                                 | Print comm                                |
| p $lx_current().cred                                                 | Print current process credential          |
| ls-ps                                                                | Print kernel proccesses                   |
| p $container_of(init_task.tasks.next, "struct task_struct", "tasks") | Can print using container_of kernel macro |
| p/x $lx_thread_info($lx_task_by_pid(1))                              | Using thread info macro for task struct   |

# 2. Android11, 12

안드로이드 11부터 적용된 [GKI](https://source.android.google.cn/devices/architecture/kernel/generic-kernel-image?hl=ko)는 커널은 기본적으로 리눅스 커널을 쓰고 나머지 벤더에 관련된 파일들은 모듈로 모아 ko파일로 생성된다. 생성된 커널의 bzImage와 나머지 ko파일들은 버전을 확인하여 맞지 않는 버전일 시 제대로 로드되지 않는다. 따라서 커널과 짝이 맞는 ko모듈을 넣어야지만 정상적으로 부팅시킬 수 있다.

## 2.1. 커널만 빌드하기
[참고 사이트](https://forum.xda-developers.com/t/guide-build-mod-update-kernel-ranchu-goldfish-5-4-5-10-gki-ramdisk-img-modules-rootavd-android-11-r-12-s-avd-google-play-store-api.4220697/)

커널만 빌드하기 위해서는 ko와 짝을 맞추어야 하므로 기존의 커널 버전을 알아야만 한다. Android Studio (리눅스의 경우 ~/android-studio/bin/studio.sh) 를 실행시켜 필요한 Android12/11 image 및 tools등을 다운로드 받는다.
![sdkmanager_platformtools](/assets/img/2021-12-14-Android_AVDKernel_debugging_with_kgdb/sdkmanager_platforms.png)
![sdkmanager_platformtools](/assets/img/2021-12-14-Android_AVDKernel_debugging_with_kgdb/sdkmanager_tools.png)

다운로드를 다 받게 되면 avdmanager에서 적절한(Android11/12)등으로 이루어진 이미지 하나를 생성한다.  
![avdmanager_android12](/assets/img/2021-12-14-Android_AVDKernel_debugging_with_kgdb/avdmanager_android12.png)

생성 후 아래와 같이 -show-kernel -verbose 옵션을 주고 해당 avd를 실행시키면 커널 로그가 생성된다.
```sh
emulator -avd Pixel_5_API_31 -show-kernel -verbose -debug init
```

```log
[    0.000000] Linux version 5.4.61-android11-2-00064-g4271ad6e8ade-ab6991359 (build-user@build-host) (Android (6443078 based on r383902) clang version 11.0.1 (https://android.googlesource.com/toolchain/llvm-project b397f81060ce6d701042b782172ed13bee898b79), LLD 11.0.1 (/buildbot/tmp/tmp6_m7QH b397f81060ce6d701042b782172ed13bee898b79)) #1 SMP PREEMPT Mon Nov 23 17:45:44 UTC 2020
```

생성되는 로그에서 커널 버전이 나오게 되는데, 그 내용은 다음과 같다.
| Value                                                    | Meaning                                                                                          |
|----------------------------------------------------------|--------------------------------------------------------------------------------------------------|
| Linux version 5.4.61​                                    | Kernel VERSION.PATCHLEVEL.SUBLEVEL​                                                              |
| android11-2​                                             | retagged Kernel branch and KMI generation number -> android11-5.4​                               |
| -g4271ad6e8ade​                                          | Local Version String (very important to avoid: disagrees about version of symbol module_layout)​ |
| -ab6991359​                                              | Automatically append version information to the version string​                                  |
| build-user​                                              | KBUILD_BUILD_USER​                                                                               |
| build-host​                                              | KBUILD_BUILD_HOST​                                                                               |
| Android (6443078 based on r383902) clang version 11.0.1​ | Android Clang/LLVM Prebuilt Version r383902​                                                     |

이 때 중요한것은 android11-2, Local Version String 및 Clang Version이다. 해당 버전에 맞는 커널 빌드를 시도해 볼 것이다.

### 2.1.1. 커널 버전 맞추기

일단 커널에 관련된 많은 파일들을 넣어야 하니 적당한 폴더를 하나 생성하자.
```sh
mkdir avdkernel5.4compile && cd avdkernel5.4compile
```

그 다음 android11-2라는 tag를 맞춰줘야 하는데, 우리는 이미 해당 태그가 android11-5.4라는 브랜치라는것을 알고 있으므로 아래와 같이 single branch만 긁어오면 된다.
```sh
git clone -b android11-5.4 --single-branch https://android.googlesource.com/kernel/common
```

만약 해당 tag의 브랜치를 모른다면, 시간은 오래걸리겠지만, 어쩔 수 없이 아래와 같은 과정을 거쳐야 한다.
```sh
git clone  https://android.googlesource.com/kernel/common
git tag
#find android11-2
git reset --hard `$tag`
```

그 다음 clang version을 맞추어야 한다. 위에서 r383902였기 때문에 다음과 같이 clang버전을 다운로드 받자.
```sh
mkdir clang-r383902 && cd clang-r383902
wget https://android.googlesource.com/platform/prebuilts/clang/host/linux-x86/+archive/android-11.0.0_r28/clang-r383902.tar.gz
tar -xzf clang-r383902.tar.gz && cd ..
```

이후 build-tools 및 gcc도 다운로드 받는다.

```sh
git clone https://android.googlesource.com/kernel/prebuilts/build-tools
git clone \
    -b android-11.0.0_r28 \
    --single-branch https://android.googlesource.com/platform/prebuilts/gcc/linux-x86/x86/x86_64-linux-android-4.9
```

이제 커널 다운로드 준비가 완료되었다.

### 2.1.2. 커널 config 복사 및 빌드하기

아까의 emulator config를 가져오자. 빌드 할 때 해당 config로 빌드 할 예정이다.

```sh
adb pull /proc/config.gz
gunzip -k config.gz
```

우리가 이 방법 말고, 커널과 모듈 전체를 빌드하는 방법을 쓸 때는 build config를 가져와서 보통 빌드 하는데 그 때의 build config를 참고해서 보자면 

build.config.common
```config
BRANCH=android12-5.10
KMI_GENERATION=9

LLVM=1
DEPMOD=depmod
DTC=dtc
CLANG_PREBUILT_BIN=prebuilts-master/clang/host/linux-x86/clang-r416183b/bin
BUILDTOOLS_PREBUILT_BIN=build/build-tools/path/linux-x86

EXTRA_CMDS=''
STOP_SHIP_TRACEPRINTK=1
IN_KERNEL_MODULES=1
DO_NOT_STRIP_MODULES=1

HERMETIC_TOOLCHAIN=${HERMETIC_TOOLCHAIN:-1}
```

등등의 필요한 것들을 make안에 넣어주어야 한다. 따라서 다음과 같은 쉘코드를 작성하였다.

```sh
cd common
export ARCH=x86_64
export CLANG_TRIPLE=x86_64-linux-gnu-
export CROSS_COMPILE=x86_64-linux-androidkernel-
export LINUX_GCC_CROSS_COMPILE_PREBUILTS_BIN=/home/tty1538/data/project/avdkernel5.10compile/x86_64-linux-android-4.9/bin
export CLANG_PREBUILT_BIN=/home/tty1538/data/project/avdkernel5.10compile/clang-r416183b/bin
export BUILDTOOLS_PREBUILT_BIN=/home/tty1538/data/project/avdkernel5.10compile/build-tools/linux-x86/bin
DEVEXPS='LLVM=1 LLVM_IAS=1 DTC=dtc CC=clang LD=ld.lld NM=llvm-nm OBJCOPY=llvm-objcopy DEPMOD=depmod EXTRA_CMDS='' STOP_SHIP_TRACEPRINTK=1 DO_NOT_STRIP_MODULES=1'
export KBUILD_BUILD_USER=build-user
export KBUILD_BUILD_HOST=build-host
export PATH=$LINUX_GCC_CROSS_COMPILE_PREBUILTS_BIN:$CLANG_PREBUILT_BIN:$BUILDTOOLS_PREBUILT_BIN:$PATH
make $DEVEXPS mrproper
cp ../config .config
echo $PATH
make $DEVEXPS xconfig
bear make $DEVEXPS -j$(nproc)
```

여기서 꼭 해주어야 할 것이 xconfig 설정할 시에 꼭 LOCAL_VERSION 부분을 해당 커널 버전으로 수정해 주어야 한다.  
![local version](/assets/img/2021-12-14-Android_AVDKernel_debugging_with_kgdb/LocalVersion.png)

이 후 emulator 구동 시에 -kernel 옵션을 줘서 새로 빌드한 커널로 부팅하게끔 하면 끝!
해당 커널을 system-images안의 kernel-ranchu로 변경시키면 따로 kernel옵션을 안줘도 되긴 하지만, 부팅이 안되면 매우 속상하니 웬만하면 -kernel옵션을 주고 실행시키자.

## ko모듈을 포함하여 빌드해서 넣기

### 준비사항
[rootAVD](https://github.com/newbit1/rootAVD) 를 다운로드 하여 준비한다.

### 빌드 과정
virtual mahcine에서 돌아가는 커널은 예전 goldfish와는 다르게 common에 커널이 들어있고 common-modules에 virtual machine관련된 device driver가 들어있다.(goldfish 포함)  
android repo 에서 manifest별로 한번에 다운로드 받으면 되는데 이는 다음과 같다. 이번 예제에서는 5.10커널 기준으로 설명한다.

```sh
BRANCH=common-android12-5.10-lts
ROOTDIR=AVD-kernel-$BRANCH
mkdir $ROOTDIR && cd $ROOTDIR
repo init --depth=1 -u https://android.googlesource.com/kernel/manifest -b $BRANCH
repo sync --force-sync --no-clone-bundle --no-tags -j$(nproc)
```

우리는 virtual machine의 kgdb를 돌리는것이 목적이므로, kgdb관련 설정을 하면 되는데 먼저 몇 가지 수정 사항이 필요하다.
첫 번째로는 kgdb buildconfig에서 bzImage와 goldfish module이 필요하므로 다음과  같은 수정사항을 반영한다.
buid.config.virtual_device.x86_64에다가 kgdb관련 수정사항의 config를 넣어줘도 되지만 거의 비슷한 수정을 거치기에 해당 방법으로 진행하도록 하겠다.

```diff
diff --git a/build.config.virtual_device_kgdb.x86_64 b/build.config.virtual_device_kgdb.x86_64
index d577b7d..957da6f 100644
--- a/build.config.virtual_device_kgdb.x86_64
+++ b/build.config.virtual_device_kgdb.x86_64
@@ -5,4 +5,6 @@
 DEFCONFIG=vd_x86_64_gki_defconfig
 PRE_DEFCONFIG_CMDS="KCONFIG_CONFIG=${ROOT_DIR}/${KERNEL_DIR}/arch/x86/configs/${DEFCONFIG} ${ROOT_DIR}/${KERNEL_DIR}/scripts/kconfig/merge_config.sh -m -r ${ROOT_DIR}/${KERNEL_DIR}/arch/x86/configs/gki_defconfig ${ROOT_DIR}/common-modules/virtual-device/virtual_device.fragment ${ROOT_DIR}/common-modules/virtual-device/kgdb.fragment ${ROOT_DIR}/common-modules/virtual-device/kgdb-x86.fragment"
 POST_DEFCONFIG_CMDS="rm ${ROOT_DIR}/${KERNEL_DIR}/arch/x86/configs/${DEFCONFIG}"
+BUILD_GOLDFISH_DRIVERS=m
+MAKE_GOALS="bzImage modules"
 MAKE_GOALS="${MAKE_GOALS} scripts_gdb"
```

이후에 해당 build config와 gki_defconfig를 이용하여 config.sh를 실행시킨다.

```sh
BUILD_CONFIG=common-modules/virtual-device/build.config.virtual_device_kgdb.x86_64 \
FRAGMENT_CONFIG=common/arch/x86/configs/gki_defconfig \
build/config.sh
```

이 후 커널 빌드를 한다.

```sh
BUILD_CONFIG=common-modules/virtual-device/build.config.virtual_device_kgdb.x86_64 build/build.sh -j$(nproc)  
```

빌드된 커널은 out/android-xx/dist/ 안에 들어있다. 아마 bzImage와 initramfs.img가 들어있을 것이다. 이것들을 rootAVD폴더에 복사한다. 이 후에 emulator를 켜 주고 rootAVD를 통하여 ramdisk img에 대한 패치를 진행한다. 이 방법은 android version이 12버전일 경우 라서 API=s라고 명시했다. 11일 경우는 11이라고 써주면 된다.

```sh
./rootAVD.sh ~/Android/sdk/system-images/android-31/google_apis/x86_64/ramdisk.img InstallKernelModules API=s
```

이후 다시 emulator를 켜 주면 정상적으로 바뀐 커널이 올라가 있는것을 확인할 수 있다.

```sh
emulator -avd Pixel_5_API_31 -show-kernel -verbose -debug init -qemu -s 
```

```log
[    0.000000] Linux version 5.10.83-android12-9-g6e6898e23cab-dirty (build-user@build-host) (Android (7284624, based on r416183b) clang version 12.0.5 (https://android.googlesource.com/toolchain/llvm-project c935d99d7cf2016289302412d708641d52d2f7ee), LLD 12.0.5 (/buildbot/src/android/llvm-toolchain/out/llvm-project/lld c935d99d7cf2016289302412d708641d52d2f7ee)) #1 SMP PREEMPT Sat Dec 4 10:55:29 UTC 2021
```

이후에 빌드하는 커널에 대해서는 램디스크 패치까지는 필요 없고 커널만 설정해주면 ko모듈은 잘 로딩된다.
```sh
emulator -avd Pixel_5_API_31 -show-kernel -verbose -debug init -qemu -s -kernel /home/tty1538/data/project/AVD-kernel-common-android12-5.10-lts/out/android12-5.10/dist/bzImage
```

이후 빌드된 vmlinux로 target remote:1234로 gdb접속시 kgdb가 정상적으로 실행됨을 확인할 수 있다.
![kgdb_android12]/assets/img/2021-12-14-Android_AVDKernel_debugging_with_kgdb/kgdb_android12.png)