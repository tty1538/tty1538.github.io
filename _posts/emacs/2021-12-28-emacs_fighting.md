---
title: New Emacs Settings 
date: 2021-12-27 09:00:00 +09:00
categories: [linux emacs]
tags: [emacs editor]
description: My New Settings For Emacs
---


# Projectiles

C-p p 하고 나서 각각의 project에서 M-o를 입력시 상세 정보를 볼 수 있다.
다른것도 마찬가지. C-p f 하고 나서도 M-o 입력시 각각의 정보를 볼 수 있음.
C-p E 하면 여러가지 옵션이 보이는데 projectiles의 설정 옵션이라고 생각하면 됨.
예를들면 projectile run command 등을 입력할 수 있음. 그러고 나서
그 다음에 S-M-: 을 누르고 hack-dir-local-variable을 하면 projectile run command 수정을 자동으로 반영해줌
rg = rip grep 으로 rust로 쓰여진 grep보다 빠른것
여기서 C-c C-o를 누르면 버퍼가 따로 나옴. 여기서 누르면 꽤나 빨리 grep할 수 있음.

