#include <stdio.h>
#include <stdlib.h>

#define byte unsigned char
#define uint unsigned int

void usb_xmodem(xmodem_t *xmodem) {
  /* first check message length, sequence number, and crc checksum */

  /* command parsing begins */
  byte cmd = (xmodem->msg).cmd;
  if (cmd == 0xfe) { /* head command */
    int file_type = (xmodem->msg).file_type;
    if ((seq == 0) && (msg_len == 14) && (file_type - 1 & 0xff) < 2) {
      uint length = xmodem->msg[4] << 0x18 | xmodem->msg[5] << 0x10 |
                    xmodem->msg[6] << 0x08 | xmodem->msg[7];
      uint address = xmodem->msg[8] << 0x18 | xmodem->msg[9] << 0x10 |
                     xmodem->msg[10] << 0x08 | xmodem->msg[11];

      /* ISSUE:
        address is always set in the internal structure
        before verified */
      xmodem->file_type = file_type;
      xmodem->file_download_length = length;
      xmodem->file_download_addr_1 = address;
      xmodem->file_download_addr_2 = address;

      if (address == 0x22000) { /* limit download address */
        if ((length % 1024) == 0)
          size = 1;
        else
          size = 2;
        /* initialize inner struct to the download details */
        xmodem->total_received = 0;
        xmodem->latest_seen_seq = 0;
        xmodem->total_frame_count = size + (length / 1024);
        xmodem->next_seq = 1;
        send_usb_response(xmodem, 0xaa);
        return;
      }
      /* ISSUE:
        xmodem->next_seq is NOT reset if the address was invalid */
      send_usb_response(xmodem, 0x07); /* address error */
      return;
    }
    send_usb_response(xmodem, 0x55);
    return;
  }

  if (xmodem->next_seq == 0) {
    /* there hasn't been any head command so far
    but download must start with a head chunk! */
    usb_bulk_in__listen(xmodem);
    return;
  }
  /* after this, data and tail chunk are
    both processed and accepted */
  //...
}
/* after this, data and tail chunk are processed
without any checking on xmodem->total_frame_count */
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

/*
}