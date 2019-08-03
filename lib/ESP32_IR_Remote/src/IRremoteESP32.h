/*
* Copyright (c) 2019 Cristian Sandu
*
* Based on https://github.com/Darryl-Scott/ESP32-RMT-Library-IR-code-RAW
* Plus: https://github.com/espressif/esp-idf/blob/master/examples/peripherals/rmt_nec_tx_rx/main/infrared_nec_main.c
* https://github.com/pcbreflux/espressif/blob/master/esp32/arduino/sketchbook/ESP32_IR_Remote/ir_demo/ESP32_IR_Remote.cpp
* Also:
* https://github.com/cyborg5/IRLib2
*  
* A small class to send and receive NEC IR codes with the RMT peripheral on an ESP32, could be pushed to IRremote(or IRlib2)
*/

#ifndef ESP32_IR_REMOTE_H_
#define ESP32_IR_REMOTE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "esp32-hal.h"
#include "esp_intr.h"
#include "driver/gpio.h"
#include "driver/rmt.h"
#include "driver/periph_ctrl.h"
#include "freertos/semphr.h"
#include "soc/rmt_struct.h"

#ifdef __cplusplus
}
#endif

class IRremoteESP32
{
public:
  // Init
  IRremoteESP32();
  void setRecvPin(int recvpin);
  void setSendPin(int sendpin);
  void setRecvPin(int recvpin, int port);
  void setSendPin(int sendpin, int port);

  // Generic methods
  void initReceive();
  void initSend();
  void stopIR();

  void sendRAW(int *data, int IRlength);

  // NEC
  int readNEC(uint32_t *data);
  void sendNEC(const uint32_t &data);

private:
  int gpionum;
  int rmtport;
  void buildItem(rmt_item32_t *item, int high_us, int low_us);
  void buildHeaderItem(rmt_item32_t *item);
  void buildZeroItem(rmt_item32_t *item);
  void buildOneItem(rmt_item32_t *item);
  void buildEndItem(rmt_item32_t *item);

  // NEC
  bool NEC_checkRange(int duration_ticks, int expected_us);
  bool NEC_is0(rmt_item32_t *item);
  bool NEC_is1(rmt_item32_t *item);
  bool NEC_isHeader(rmt_item32_t *item);
  bool NEC_isRepeat(rmt_item32_t *item);
  bool NEC_isEnd(rmt_item32_t *item);
  int decodeNEC(rmt_item32_t *item, int item_num, uint32_t *data);
};

#endif /* ESP32_IR_REMOTE_H_ */
