---
title: Android Kernel Navigation with vscode
date: 2021-11-11 14:55:00 +09:00
categories: [android_kernel, vscode, clang]
tags: [vscode, gen_compile_commands, compile_commands]
description: Android Kernel with vscode
---

원래 Emacs만 쓰고 있고 그거에 대해서 자부심을 가지고 있었지만, 최근에 너무 좋은 IDE들이 많이 나오면서 코드 분석도구를 고민하다가 clion을 쓰고 있었지만, 유료 였기에 최근에 들어서 vscode로 다시 사용중이다.
무료임에도 불구하고 정말 많은 extension들이 있고 이에 대해 만족하면서 쓰고 있는데 한 가지 불만이 안드로이드 커널을 사용할 때 너무 네비게이션이 잘 되지 않는 것이다. Emacs를 쓸 때는 Ctags/Etags 등을 썼는데, 이는 그냥 단순한 정적분석이어서 빌드 시 줬던 CONFIG, make option과 비슷한 여러 기능들의 적용이 불가능하다.
최근에 Chromium을 쓰게되면서 llvm/clang 등으로 컴파일 할 시에 *.o.cmd 파일들이 생성되고 이를 한번에 모아줘서 새롭게 만든 파일로 코드 네비게이션을 쉽게 할 수 있다는 방법을 알게 되었는데 이를 안드로이드 커널에도 사용하고 싶었다.

# compile_commands.json
[compilation database](https://clang.llvm.org/docs/JSONCompilationDatabase.html) 를 참고하면 llvm/clang에서 compilation db를 어떻게 사용하고 있는지 알 수 있다. 컴파일 데이터베이스는 "cmd object"의 배열로 구성된 JSON 파일이며, 여기서 명령 개체는 프로젝트에서 번역 단위가 컴파일되는 한 가지 방법을 지정한다. 각 명령 개체에는 번역 단위의 기본 파일, 컴파일 실행의 작업 디렉터리 및 실제 컴파일 명령이 포함된다.

```json
[
  { "directory": "/home/user/llvm/build",
    "command": "/usr/bin/clang++ -Irelative -DSOMEDEF=\"With spaces, quotes and \\-es.\" -c -o file.o file.cc",
    "file": "file.cc" },
]
```

- 디렉토리: 컴파일 작업 디렉토리. 명령 또는 파일 필드에 지정된 모든 경로는 이 디렉토리에 대해 절대적이거나 상대적이어야 한다.
- file: 이 컴파일 단계에서 처리된 기본 번역 단위 소스. 이것은 도구에서 컴파일 데이터베이스의 키로 사용됨. 예를 들어 동일한 소스 파일이 다른 구성으로 컴파일된 경우 같은 파일에 대해 여러 명령 개체가 있을 수 있다.
- 명령: 실행된 컴파일 명령입니다. JSON 이스케이프 해제 후 빌드 시스템이 사용하는 환경에서 번역 단위에 대한 정확한 컴파일 단계를 다시 실행하려면 유효한 명령이어야 한다. 매개변수는 쉘 따옴표와 따옴표의 쉘 이스케이프를 사용하며 '"' 및 '\'만 특수 문자.
- 인수: 문자열 목록으로 실행되는 컴파일 명령. 인수 또는 명령이 필요
- output: 이 컴파일 단계에서 생성된 출력의 이름. 이 필드는 선택 사항이다. 동일한 입력 파일의 다른 처리 모드를 구별하는 데 사용할 수 있다.

# Android Kernel의 compile_commands.json 생성
안드로이드 커널은 최근에 llvm 컴파일러로 전격 교체되었으며 이에 따라 clang을 사용하여 빌드한다. 이에 따라 위의 cmd.o의 cmd object가 생성되고 이것들을 모으기만 한다면 compile_command.json을 만들 수 있다. 이에따라 리눅스 커널에서는 해당 기능을 지원했으나 안드로이드 커널에서는 아직 적용이 안 되었고, 이를 포팅해서 가져오면 된다. 링크는 다음에서 가져올 수 있다. [gen_compile_commands.py](https://github.com/torvalds/linux/commit/b30204640192)

나의 경우에는 kernel/scripts/ 안에 해당 파일을 넣고 
```shell
python gen_compile_commands.py -d /home/tty1538/data/project/pixel5_kernel/out -o /home/tty1538/data/project/pixel5_kernel                  
```
다은과 같은 명령어로 compile_commands.json을 생성 후에 이 후에 clangd extension을 사용하면 정상적으로 사용이 가능했다.

# bear
llvm/clang등의 컴파일러 콜을 인터셉트해서 compilation json 을 빌드시에 만들어주는 프로그램이다. 아마 jetbrain에서 지원을 해서 만들어진 것 같다.
```shell
sudo apt install bear
bear sh ./build_redbull.sh
```
이런식으로 정말 간단하게 사용이 가능하다. 위의 방법보다 아래가 더 간단하여 앞으로는 아래 bear를 더 이용해볼 예정
[bear](https://github.com/rizsotto/Bear)

