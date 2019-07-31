#include <IRremote.h>

TaskHandle_t senderTask;
TaskHandle_t receiverTask;

xQueueHandle sendQueue;

const int RECV_PIN = 2;

const unsigned long LG_VOL_UP = 0xEF00FF;
const unsigned long LG_VOL_DOWN = 0xEF807F;
const unsigned long LG_MUTE = 0xEF6897;

IRrecv irrecv(RECV_PIN);
IRsend irsend; // PWM pin 3

enum edi_codes_t {
  VOL_UP,
  VOL_DOWN,
  MUTE,
  NONE
};

// TODO: use NEC instead of RAW pls
const unsigned int EDI_VOL_UP_RAW[] = {8917, 4480, 555, 533, 555, 555, 555, 555, 555, 555, 555, 1643, 555, 555, 555, 533, 555, 555, 555, 1643, 555, 1643, 555, 1643, 555, 555, 533, 555, 555, 1643, 555, 1643, 555, 1643, 555, 555, 555, 1643, 555, 1643, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 1643, 555, 555, 533, 555, 555, 1643, 555, 1643, 555, 1643, 555, 1643, 555, 1643, 555, 41899, 8917, 2240, 555, 96491, 8896, 2261, 512}; //AnalysIR Batch Export (IRremote) - RAW
const unsigned int EDI_VOL_DOWN_RAW[] = {8853, 4523, 512, 576, 491, 619, 469, 619, 469, 619, 469, 1707, 512, 597, 469, 619, 469, 619, 469, 1707, 512, 1685, 512, 1685, 512, 597, 469, 619, 469, 1707, 512, 1685, 512, 1685, 512, 1685, 512, 1685, 512, 1685, 512, 597, 469, 619, 469, 619, 469, 1707, 512, 597, 469, 619, 469, 619, 469, 619, 469, 1707, 512, 1685, 512, 1685, 512, 597, 469, 1685, 533, 41877, 8875, 2283, 512};                       //AnalysIR Batch Export (IRremote) - RAW
const unsigned int EDI_MUTE_RAW[] = {8875, 4523, 512, 576, 512, 576, 512, 576, 512, 576, 512, 1707, 512, 576, 491, 597, 491, 597, 491, 1685, 533, 1685, 533, 1685, 533, 576, 491, 597, 491, 1685, 533, 1685, 512, 1685, 533, 1685, 533, 576, 491, 597, 491, 597, 491, 597, 491, 597, 491, 1707, 512, 576, 491, 597, 491, 1685, 533, 1685, 512, 1664, 533, 1685, 512, 1685, 512, 576, 512, 1685, 533, 41856, 8875, 2283, 512};                           //AnalysIR Batch Export (IRremote) - RAW

// TODO: does not seem to do what I want it to(speed up volume control)
const unsigned int SIGNAL_REPEAT = 1;

decode_results results;

void recvTaskFunc(void *params)
{
  const TickType_t ticksToWait = pdMS_TO_TICKS(100);

  for (;;)
  {
    edi_codes_t codeToSend = NONE;
    if (irrecv.decode(&results))
    {
      Serial.println(results.value, HEX);
      Serial.println(results.decode_type);

      switch (results.value)
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

    delay(100);
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
        irsend.sendRaw(EDI_VOL_DOWN_RAW, sizeof(EDI_VOL_DOWN_RAW) / sizeof(EDI_VOL_DOWN_RAW[0]), 38);
        break;
      case VOL_UP:
        irsend.sendRaw(EDI_VOL_UP_RAW, sizeof(EDI_VOL_UP_RAW) / sizeof(EDI_VOL_UP_RAW[0]), 38);
        break;
      case MUTE:
        irsend.sendRaw(EDI_VOL_DOWN_RAW, sizeof(EDI_MUTE_RAW) / sizeof(EDI_MUTE_RAW[0]), 38);
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
  // In case the interrupt driver crashes on setup, give a clue
  // to the user what's going on.
  Serial.println("Enabling IRin");
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("Enabled IRin");

  // A queue of max 5 elements
  sendQueue = xQueueCreate(5, sizeof(edi_codes_t));

  xTaskCreate(
      recvTaskFunc,
      "recvTask",
      10000,
      NULL,
      1,
      NULL);

  xTaskCreate(
      sendTaskFunc,
      "sendTask",
      10000,
      NULL,
      1,
      NULL);
}
