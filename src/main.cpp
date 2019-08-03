/*
* Copyright 2019 Cristian Sandu
* IR code translator
*/
#include <Arduino.h>
#include <IRremote.h>
#include <IRremoteESP32.h>

TaskHandle_t senderTask;
TaskHandle_t receiverTask;

xQueueHandle sendQueue;

const int RECV_PIN = 13;
const int SEND_PIN = 12;

const uint32_t LG_VOL_UP = 0xEF00FF;
const uint32_t LG_VOL_DOWN = 0xEF807F;
const uint32_t LG_MUTE = 0xEF6897;

const uint32_t EDI_VOL_UP = 0x08E7609F;
const uint32_t EDI_VOL_DOWN = 0x08E7E21D;
const uint32_t EDI_MUTE = 0x08E7827D;

enum edi_codes_t
{
  VOL_UP,
  VOL_DOWN,
  MUTE,
  NONE
};

// TODO: does not seem to do what I want it to(speed up volume control)
const unsigned int SIGNAL_REPEAT = 1;

decode_results results;

void recvTaskFunc(void *params)
{
  IRremoteESP32 irrecv;
  irrecv.setRecvPin(RECV_PIN, 1);
  irrecv.initReceive();

  const TickType_t ticksToWait = pdMS_TO_TICKS(100);

  int *codeToSend = new int(NONE);
  for (;;)
  {

    uint32_t data;
    if (irrecv.readNEC(&data))
    {
      Serial.println(data, HEX);

      switch (data)
      {
      case LG_VOL_DOWN:
        *codeToSend = VOL_DOWN;
        break;
      case LG_VOL_UP:
        *codeToSend = VOL_UP;
        break;
      case LG_MUTE:
        *codeToSend = MUTE;
        break;
      default:
        *codeToSend = NONE;
        break;
      }

      if (*codeToSend != NONE)
      {
        xQueueSendToFront(sendQueue, codeToSend, ticksToWait);
      }
    }
  }

  // TODO: this is unreachable
  delete codeToSend;
}

void sendTaskFunc(void *params)
{
  IRremoteESP32 irsend;
  irsend.setSendPin(SEND_PIN, 0);
  irsend.initSend();

  const TickType_t ticksToWait = pdMS_TO_TICKS(100);

  int *codeToSend = new int(NONE);
  for (;;)
  {
    if (xQueueReceive(sendQueue, codeToSend, ticksToWait) == pdPASS)
    {
      switch (*codeToSend)
      {
      case VOL_DOWN:
        irsend.sendNEC(EDI_VOL_DOWN);
        break;
      case VOL_UP:
        irsend.sendNEC(EDI_VOL_UP);
        break;
      case MUTE:
        irsend.sendNEC(EDI_MUTE);
        break;
      case NONE:
      default:
        break;
      }
    }
  }

  // TODO: this is unreachable
  delete codeToSend;
}

void loop()
{
  // Nada
}

void setup()
{
  Serial.begin(115200);

  // A queue of max 5 elements
  sendQueue = xQueueCreate(5, sizeof(edi_codes_t));

  xTaskCreatePinnedToCore(
      recvTaskFunc,
      "recvTask",
      10000,
      NULL,
      1,
      NULL,
      0);

  xTaskCreatePinnedToCore(
      sendTaskFunc,
      "sendTask",
      10000,
      NULL,
      1,
      NULL,
      1);
}
