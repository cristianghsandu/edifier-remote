#include <Arduino.h>
#include <RMTLib.h>

const int RECV_PIN = 13;
const int SEND_PIN = 12;

const uint32_t LG_VOL_UP = 0xEF00FF;
const uint32_t LG_VOL_DOWN = 0xEF807F;
const uint32_t LG_MUTE = 0xEF6897;

const uint32_t EDI_VOL_UP = 0x08E7609F;
const uint32_t EDI_VOL_DOWN = 0x08E7E21D;
const uint32_t EDI_VOL_MUTE = 0x08E7827D;
const uint32_t EDI_REPEAT = 0xFFFFFFFF;

const int TX_PIN = 12;
const int RX_PIN = 13;

RMTLib rmt;

void setup()
{
  Serial.begin(1152200);

  rmt.setTxPin(TX_PIN);
  rmt.setRxPin(RX_PIN);
}

void loop()
{
  rmt.decodeNEC();
}
