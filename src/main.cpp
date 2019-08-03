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

const unsigned long LG_VOL_UP = 0xEF00FF;
const unsigned long LG_VOL_DOWN = 0xEF807F;
const unsigned long LG_MUTE = 0xEF6897;

const uint32_t EDI_VOL_UP = 0x8E7609F;
const uint32_t EDI_VOL_DOWN = 0x8E7E21D;
const uint32_t EDI_MUTE = 0x8E7827D;

// IRremote only supports receiving on the ESP32
IRremoteESP32 irrecv;
IRremoteESP32 irsend;

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
  const TickType_t ticksToWait = pdMS_TO_TICKS(100);

  for (;;)
  {
    edi_codes_t codeToSend = NONE;

    uint32_t data;
    if (irrecv.readNEC(&data))
    {
      Serial.println(data, HEX);

        switch (data)
        {
        case LG_VOL_DOWN:
          for (size_t i = 0; i < SIGNAL_REPEAT; i++)
          {
            codeToSend = VOL_DOWN;
            xQueueSendToFront(sendQueue, &codeToSend, ticksToWait);
          }
          break;
        case LG_VOL_UP:
          for (size_t i = 0; i < SIGNAL_REPEAT; i++)
          {
            codeToSend = VOL_UP;
            xQueueSendToFront(sendQueue, &codeToSend, ticksToWait);
          }
          break;
        case LG_MUTE:
          for (size_t i = 0; i < SIGNAL_REPEAT; i++)
          {
            codeToSend = MUTE;
            xQueueSendToFront(sendQueue, &codeToSend, ticksToWait);
          }
          break;
        default:
          break;
        }
    }
  }
}

void sendTaskFunc(void *params)
{
  const TickType_t ticksToWait = pdMS_TO_TICKS(100);

  edi_codes_t codeToSend;
  for (;;)
  {
    if (xQueueReceive(sendQueue, &codeToSend, ticksToWait) == pdPASS)
    {
      switch (codeToSend)
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
}

void loop()
{
  // Nada
}

void setup()
{
  Serial.begin(115200);

  irsend.setSendPin(SEND_PIN, 0);
  irsend.initSend();

  irrecv.setRecvPin(RECV_PIN, 1);
  irrecv.initReceive();

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
