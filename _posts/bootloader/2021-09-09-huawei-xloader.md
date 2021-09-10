---
title: huawei xloader 1day reproducing
date: 2021-09-09 07:35:00 +09:00
categories: [huawei, bootloader]
tags: [huawei, bootloader, xloader, edl]
description: reproducing huawei xloader exploit 
---

# Kirin Chipset

화웨이에서 2017년부터 개발한 KIRIN Chipset은 자체의 secure boot chian과 구조를 가지고 있다. 이에 대해서 크게 알려진 바가 없었으나 최근에 Blackhat USA 2021 에서 Kirin Chipset에 대한 [How To Tame Your Unicorn - Exploring and Exploiting Zero-Click Remote Interfaces of Modern Huawei Smartphones](https://www.blackhat.com/us-21/briefings/schedule/index.html#how-to-tame-your-unicorn---exploring-and-exploiting-zero-click-remote-interfaces-of-modern-huawei-smartphones-23337)에 대한 발표로 여러가지 정보들이 알려졌다. 이 발표에서 TASZK는 본인들이 발견한 화웨이 xloader의 취약점 및 이를 활용한 xloader에서의 code execution에 대해서 상세히 다루고 있다. 이를 위한 지식들인 secure boot chain, firmware Ecryption Status등에 대한 정보를 하위 파트에서 소개한다.

## secure boot chain

secure boot chain은 모든 칩셋 제조사에서 쓰는 방식으로 전 단계에서 다음 단계로 넘어가기 전에 로딩되는 이미지의 signature/ certificate 등등을 확인하여 정상적인 부트 체인 단계인지를 검증하는 단계이다. 
![secure boot chain](/assets/img/2021-09-09-huawei-xloader/secure_boot_chain.png)

조금 헷갈리는 부분이 LPMCU 라는 용어인데 Low Power Memory Control Unit의 약자인 것으로 보이는데 BOOTROM과 xloader가 둘 다 LPMCU에서 실행된다고 하니 그것이 조금 이상하다. 또한, fastboot이 EL3에서 실행되기 때문에 fastboot exploit이 된다면 EL3의 권한도 함께 얻을 수 있다. 

## encryption status

xloader >> fastboot >> trustfirmware >> teeos >> modem 으로 가는 image loading chain에서 해당 이미지가 encryption 되어 있는지에 대한 표
![encryption status](/assets/img/2021-09-09-huawei-xloader/encryption_status.png)

후에 설명하겠지만 mate 30 pro(KIRIN 980)의 이미지가 정상적으로 보이지 않는걸 보니 encrypted로 바뀐것으로 보임.

## Kirin980 TestPoint

TestPoint Pin은 퀄컴 칩셋에도 EDL(Emergency Downloader)이라고 불리는 기능이다. SBL/XBL등의 개발 시 DDR Training, Booting Process등의 과정을 거쳐야 하는데 코드 오류가 있을 경우 정상적으로 부팅 프로세스가 진행되지 않아 벽돌이 된다. 예를 들어, 삼성폰을 쓰다가 벽돌이 되면 Odin Mode로 들어가 펌웨어를 다시 다운로드 받으면 되는데 만약 Odin이 로딩 되기 전에 이미 벽돌이 되었다면 어떻게 해당 기기를 복구할 것인지에 대한 해결책이다. 따라서 TestPoint Pin을 검사하는 코드는 BootRom에 들어있으며 
- flash storage의 xloader image가 망가졌을 때
- test point가 trigger됬을 때
해당 모드로 진입하게 된다. 

![testpoint](/assets/img/2021-09-09-huawei-xloader/testpoint.jpg)

### TestPoint Test (대략 2주정도 소요)

1. 폰 하나는 뒷면을 뜯은 다음에 해당 핀포인트를 알아보려고 무리하게 뜯었더니 전원버튼 전선필름을 뜯어버려서 전원버튼이 안 먹힘
2. 그러다가 testpoint 여러가지 기능좀 사용해 보려고 하다가 터치 모듈까지 맛탱이 가버려서 버림
3. 다른 폰으로 다시 뜯어서 테스팅 중. testpoint 사용 시에 흑백화면 나타남.
4. usb-c breakout board로 usb-serial, baudrate 921600으로 test했지만 아무것도 나오지 않음
5. 인터넷에서 찾아보니 부팅 시 전원키 5번을 누를시에 emui update mode로 usb download mode를 지원
![usb_download_mode](/assets/img/2021-09-09-huawei-xloader/emui_updatemode.png)
6. 하지만 얘도 serial로 뭔가 전송되지 않음
7. ellysis usb analyzer 설정 (세팅하는데 3-4일 걸린듯)
![ellysis](/assets/img/2021-09-09-huawei-xloader/ellysis.jpg)
8. 패킷 검출은 되었지만, 41 54 5ㄸ 43 55 52 43 3D 0D 이게 반복되서 오다가 suspend되고 ASCII값으로 바꿔서 보면 AT^CURC= 인데 찾아보니까 baseband 신호세기라고 함. 
9. 좌절..
10. 고민해보니 Qualcomm EDL은 EDL Downloader 프로그램이 있고 그에 따른 device driver가 따로 있었음.
11. 혹시 화웨이도 이럴까 하고 찾아보니 Huawei SER(Serial) 드라이버가 있어서 설치 하고 윈도우에서 설치하고 해당 드라이버를 COMPORT로 설정
![comport](/assets/img/2021-09-09-huawei-xloader/comport.jpg)
12. 정상적으로 COMPORT가 잡힘. 이제 해당 COMPORT로 드라이버 실험예정

### xmodem protocol

huawei의 SER은 xmodem protocol을 이용하여 통신한다. 
![xmodem_protocol](/assets/img/2021-09-09-huawei-xloader/xmodem_protocol.png)

해당 HeadChunk를 구현하여 Ack인 0xAA를 받는 파이썬 프로그램을 작성했는데, 계속 NAK인 0x55가 오길래 왜 그렇지? 


라고 고민하니, CRC의 문제였다. 그냥 일반 CRC16을 사용하여 CRC를 작성했는데, xmodem은 화웨이가 만든 전용 프로토콜이 아니라 기존에 있는 프로토콜이었고 xmodem crc가 따로 계싼하는 법이 있었다. 이러한 형태로 CRC를 작성하여 보내니 정상적으로 0xAA return을 받을 수 있었다. (little endian으로 보내지 않아도 큰 상관이 없었음)

```python
def Main():
    ser = serial.Serial('COM8', 921600, timeout=2)
    ser.flushInput()

    testData = bytearray(b'\xFE\x00\xFF\x01\x00\x04\x00\x00\x00\x02\x20\x00')
    testDataCRC = (crcb(testData));
    testDataCRCBytes = testDataCRC.to_bytes(2,'big')
    testDataResult = testData + testDataCRCBytes
    print(testDataResult)

    print(2)
    headChunk = b'\xFE\x00\xFF\x01\x00\x04\x00\x00\x00\x02\x20\x00\x11\xE0'
    ser.write(headChunk)
    recv = ser.read(1)
    print_hex_dump(recv)
    
    ser.close()
if __name__ == '__main__':
    Main()
```

![head_chunk](/assets/img/2021-09-09-huawei-xloader/send_head_chunk.jpg)



# 취약점 정리

## unchecked data length in head chunk

첫 번째 취약점은 부트롬과 xmodem에 있는 xloader 모두에 있는 취약점이다. head chunck의 size value가 검증되지 않아 엄청나게 큰 image size를 보내는 것이 가능하다. 해당 file_download_length와 total_frame_count는 나중에도 어떤 검증 없이 사용된다.

```c
  if (cmd == 0xfe) { /* head command */
    int file_type = (xmodem->msg).file_type;
    if ((seq == 0) && (msg_len == 14) && (file_type - 1 & 0xff) < 2) {
      uint length = xmodem->msg[4] << 0x18 | xmodem->msg[5] << 0x10 |
                    xmodem->msg[6] << 0x08 | xmodem->msg[7];

      //...
      xmodem->file_download_length = length;
      xmodem->total_frame_count = size + (length / 1024);

      return;
      }
```

----
## unchecked data chunk count

해당 취약점은 xloader의 xmodem에 잇는 취약점이다. xmodem implementation에서 성공적으로 받은 data chunk의 갯수를 세지 않는다. 대신에 tail chunk를 받았을 때만 boundary check가 이루어진다. 따라서 head chunk에 적어놓은 갯수보다 더 많은 다운로드가 가능하다. 

```c
//...
if (cmd == 0xda) { /* data command */
  if (seq == (xmodem->next_seq & 0xff)) {
    if (xmodem->next_seq == xmodem->total_frame_count - 1)
      size = xmodem->file_download_length - xmodem->latest_seen_seq * 1024;
    else
      size = 1024;
    if (msg_len == size + 5) {
      memcpy(xmodem->file_download_addr_1 + xmodem->latest_seen_seq * 1024,
             xmodem->msg, size);
      xmodem->total_received = xmodem->total_received - 5;
      xmodem->latest_seen_seq = xmodem->latest_seen_seq + 1;
      xmodem->next_seq = xmodem->next_seq + 1;
      send_usb_response(xmodem, 0xaa);
      return;
    }
    xmodem->total_received -= msg_len;
    send_usb_response(xmodem, 0x55);
    return;
  }
  /* repeated chunk handling code */
  //...
}
```

위의 코드를 보면 알수 있듯이 data command에서는 오로지 sequence / datasize error만 체크한다. 마지막 data chunk가 아니면 사이즈는 항상 1024 여야 하고 마지막 data chunk는 남은 data size를 가진다. 이후에 latest_seen_seq는 계속 증가하므로 total_frame_count를 넘어서도 막지 않는다. 이에 따라 N의 data length를 보낸다고 head에 적어놓고 N/1024보다 더 많은 data를 보내는 것이 가능하며 이는 download buffer를 넘어서 overflow를 일으키는 것이 가능하다.

----
## tail chunk insufficient boundary condition check

해당 취약점은 bootrom과 xloader의 xmodem 코드 전체에 다 있고 또한 tail chunk handling과정에서 생긴다.

```c
if (cmd == 0xed) { /* tail command */
  if ((xmodem->next_seq == seq) || (msg_len == 5)) {
    xmodem->next_seq = xmodem->next_seq + 1;
    xmodem->latest_seen_seq = xmodem->latest_seen_seq + 1;
    if (xmodem->latest_seen_seq != xmodem->total_frame_count) {
      send_usb_response(xmodem, 0x55);
      return;
    }
    send_usb_response(xmodem, 0xaa);
    /* reset the inner struct on receiving a valid tail */
    (...) return;
  }
  send_usb_response(xmodem, 0x55);
  return;
}
```

다음 디컴파일 된 코드에서 보면 알 수 있듯이 tail packet validataion은 sequence counter check와 message length check로 이루어져 있다. message length는 5바이트로 3byte의 preamble(command, sequence, negated sequence)와 2byte의 checksum(XMODEM-CRC)로 되어 있고 해당 packet validity check가 끝나고 나면 next_seq와 latest_seen_seq를 1씩 증가시킨다. 이후 latest_seen_seq와 total_frame_count를 비교하여 정상적으로 끝나는게 맞는지 확인한다. 하지만 만약에 정상적으로 끝나는게 맞지 않다고 하더라도 latest_seen_seq는 줄어들지 않는다. 그렇기 때문에 잘못된 tail chunk를 가지고 있으면 무한히 latest_seq를 늘리는것이 가능한데, latest_seq가 중요한 이유는 메모리 다운로드 시에 해당 코드를 사용하기 때문이다. 

```c
memcpy(xmodem->file_download_addr_1 + xmodem->latest_seen_seq * 1024,
       xmodem->msg, size);
```