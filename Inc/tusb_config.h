#pragma once

#define CFG_TUSB_MCU                 OPT_MCU_STM32F1
#define CFG_TUSB_OS                  OPT_OS_FREERTOS
#define CFG_TUSB_DEBUG               0

#define CFG_TUD_ENABLED              1
#define CFG_TUH_ENABLED              0

#define CFG_TUSB_RHPORT0_MODE        (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)
#define CFG_TUD_ENDPOINT0_SIZE       64

#define CFG_TUD_CDC                  1
#define CFG_TUD_MIDI                 1

#define CFG_TUD_MSC                  0
#define CFG_TUD_HID                  0
#define CFG_TUD_VENDOR               0

#define CFG_TUD_CDC_RX_BUFSIZE       64
#define CFG_TUD_CDC_TX_BUFSIZE       64

#define CFG_TUD_MIDI_RX_BUFSIZE      64
#define CFG_TUD_MIDI_TX_BUFSIZE      64

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN           __attribute__((aligned(4)))