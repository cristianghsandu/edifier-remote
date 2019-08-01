#include <Arduino.h>
#include <IRremote.h>
#include <ESP32_IR_Remote.h>

TaskHandle_t senderTask;
TaskHandle_t receiverTask;

xQueueHandle sendQueue;

const int RECV_PIN = 13;
const int SEND_PIN = 12;

const unsigned long LG_VOL_UP = 0xEF00FF;
const unsigned long LG_VOL_DOWN = 0xEF807F;
const unsigned long LG_MUTE = 0xEF6897;

// IRremote only supports receiving on the ESP32
ESP32_IRrecv irrecv;
ESP32_IRrecv irsend;

enum edi_codes_t
{
  VOL_UP,
  VOL_DOWN,
  MUTE,
  NONE
};

// TODO: use NEC instead of RAW pls
int EDI_VOL_UP_RAW[] = {8875, -4523, 533, -576, 491, -597, 491, -597, 491, -597, 491, -1685, 533, -576, 491, -597, 491, -597, 491, -1707, 512, -1664, 533, -1685, 533, -576, 491, -597, 491, -1685, 533, -1685, 533, -1685, 533, -576, 491, -1685, 533, -1685, 533, -576, 491, -597, 491, -597, 491, -597, 491, -597, 491, -1685, 533, -576, 491, -597, 491, -1685, 533, -1685, 533, -1685, 533, -1685, 533, -1685, 533, -41941, 8875, -2304, 512, 0};
int EDI_VOL_DOWN_RAW[] = {597, -1792, 3115, -8619, 363, -3221, 149, -8896, 4501, -533, 555, -533, 555, -533, 555, -533, 555, -533, 1643, -555, 555, -533, 555, -533, 555, -533, 1664, -533, 1643, -555, 1643, -576, 533, -555, 555, -555, 1643, -576, 1643, -576, 1643, -576, 1643, -576, 1643, -555, 1643, -576, 533, -555, 533, -555, 555, -555, 1643, -576, 533, -555, 533, -555, 533, -555, 533, -555, 1643, -576, 1643, -576, 1643, -576, 533, -555, 1643, -576, 41835, -8875, 2283, 0};
int EDI_MUTE_RAW[] = {8896, -4480, 555, -555, 533, -555, 533, -555, 533, -555, 533, -1643, 555, -555, 533, -555, 533, -555, 533, -1643, 555, -1643, 555, -1643, 555, -555, 533, -555, 533, -1643, 555, -1643, 555, -1664, 533, -1643, 555, -555, 533, -555, 533, -555, 533, -555, 533, -555, 533, -1643, 555, -555, 533, -555, 533, -1643, 555, -1643, 555, -1643, 555, -1664, 533, -1664, 533, -555, 533, -1643, 555, -41835, 8917, -2240, 555, -96405, 8896, -2240, 555, 0};

// TODO: does not seem to do what I want it to(speed up volume control)
const unsigned int SIGNAL_REPEAT = 1;

decode_results results;

void recvTaskFunc(void *params)
{
  const TickType_t ticksToWait = pdMS_TO_TICKS(100);

  for (;;)
  {
    edi_codes_t codeToSend = NONE;
    int data[4];

    int count = irrecv.readNEC(&data[0], 4);
    // if (count == 0)
    // {
    //   for (int i = 0; i < 4; i++) {
    //     Serial.println(data[i], HEX);
    //   }

    //   switch (count)
    //   {
    //   case LG_VOL_DOWN:
    //     for (size_t i = 0; i < SIGNAL_REPEAT; i++)
    //     {
    //       codeToSend = VOL_DOWN;
    //       xQueueSendToFront(sendQueue, &codeToSend, ticksToWait);
    //     }
    //     break;
    //   case LG_VOL_UP:
    //     for (size_t i = 0; i < SIGNAL_REPEAT; i++)
    //     {
    //       codeToSend = VOL_UP;
    //       xQueueSendToFront(sendQueue, &codeToSend, ticksToWait);
    //     }
    //     break;
    //   case LG_MUTE:
    //     for (size_t i = 0; i < SIGNAL_REPEAT; i++)
    //     {
    //       codeToSend = MUTE;
    //       xQueueSendToFront(sendQueue, &codeToSend, ticksToWait);
    //     }
    //     break;
    //   default:
    //     break;
    //   }
    // }
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
        irsend.sendIR(EDI_VOL_DOWN_RAW, sizeof(EDI_VOL_DOWN_RAW) / sizeof(EDI_VOL_DOWN_RAW[0]) - 1);
        break;
      case VOL_UP:
        irsend.sendIR(EDI_VOL_UP_RAW, sizeof(EDI_VOL_UP_RAW) / sizeof(EDI_VOL_UP_RAW[0]) - 1);
        break;
      case MUTE:
        irsend.sendIR(EDI_VOL_DOWN_RAW, sizeof(EDI_MUTE_RAW) / sizeof(EDI_MUTE_RAW[0]) - 1);
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

  irsend.ESP32_IRsendPIN(SEND_PIN, 0);
  irsend.initSend();

  irrecv.ESP32_IRrecvPIN(RECV_PIN, 1);
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
