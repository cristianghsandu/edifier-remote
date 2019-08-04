/*
* Copyright 2019 Cristian Sandu
* IR code translator
*/
#include <Arduino.h>
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
  NONE,
  VOL_UP,
  VOL_DOWN,
  MUTE,
};

const unsigned int SEND_REPEAT = 7;
const unsigned int REPEAT_TRESH_MS = 600;

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
      case NEC_REPEAT_DATA:
        continue; // jumps to for
      default:
        *codeToSend = NONE;
        break;
      }

      if (*codeToSend != NONE)
      {
        xQueueSendToFront(sendQueue, codeToSend, ticksToWait);
      }
    }
  } // loop

  // TODO: this is unreachable
  delete codeToSend;
}

void sendTaskFunc(void *params)
{
  IRremoteESP32 irsend;
  irsend.setSendPin(SEND_PIN, 0);
  irsend.initSend();

  const TickType_t ticksToWait = pdMS_TO_TICKS(20);

  long lastCommand_ticks = 0;
  long timeBetweenRepeats = 0;
  int lastCommand = NONE;
  int repeatCount = 0;

  int *codeToSend = new int(NONE);
  uint32_t necData = 0;
  for (;;)
  {
    if (xQueueReceive(sendQueue, codeToSend, ticksToWait) == pdPASS)
    {
      switch (*codeToSend)
      {
      case VOL_DOWN:
        necData = EDI_VOL_DOWN;
        break;
      case VOL_UP:
        necData = EDI_VOL_UP;
        break;
      case MUTE:
        necData = EDI_MUTE;
        break;
      default:
        break;
      }

      if (*codeToSend != NONE && lastCommand == *codeToSend)
      {
        // There is a repeat
        Serial.print("Repeat: ");
        timeBetweenRepeats = (xTaskGetTickCount() - lastCommand_ticks) / portTICK_PERIOD_MS;
        repeatCount = 0;
        Serial.println(timeBetweenRepeats);
      }
      else
      {
        // No repeat
        timeBetweenRepeats = 0;
      }

      // Get time
      lastCommand_ticks = xTaskGetTickCount();

      // Save last command
      lastCommand = *codeToSend;

      // Send IR
      if (necData)
      {
        irsend.sendNEC(necData);
      }

      // Clear used data
      *codeToSend = NONE;
      necData = 0;
    } // if command queue

    // A repeat happened
    if (timeBetweenRepeats && timeBetweenRepeats < REPEAT_TRESH_MS)
    {
      auto currentTime = xTaskGetTickCount() / portTICK_PERIOD_MS;
      if (currentTime - (lastCommand_ticks / portTICK_PERIOD_MS) < REPEAT_TRESH_MS)
      {
        if (repeatCount <= SEND_REPEAT)
        {
          irsend.sendNEC(NEC_REPEAT_DATA);
          repeatCount++;
        }
      }
      else
      {
        repeatCount = 0;
        timeBetweenRepeats = 0;
      }
    }

  } // loop

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
