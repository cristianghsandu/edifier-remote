#include <IRLibSendBase.h>
#include <IRLibDecodeBase.h>
#include <IRLib_P01_NEC.h>
#include <IRLibCombo.h>
#include <IRLibRecvPCI.h>
#include <IRLib_HashRaw.h>

const int RECV_PIN = 2;

const unsigned long LG_VOL_UP = 0xEF00FF;
const unsigned long LG_VOL_DOWN = 0xEF807F;
const unsigned long LG_MUTE = 0xEF6897;

IRrecvPCI irrecv(RECV_PIN);
IRsend irsend; // PWM pin 3

IRdecode necDecoder;

// TODO: use NEC instead of RAW pls
uint16_t EDI_VOL_UP_RAW[] = {8917, 4480, 555, 533, 555, 555, 555, 555, 555, 555, 555, 1643, 555, 555, 555, 533, 555, 555, 555, 1643, 555, 1643, 555, 1643, 555, 555, 533, 555, 555, 1643, 555, 1643, 555, 1643, 555, 555, 555, 1643, 555, 1643, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 555, 1643, 555, 555, 533, 555, 555, 1643, 555, 1643, 555, 1643, 555, 1643, 555, 1643, 555, 41899, 8917, 2240, 555, 8917, 8896, 2261, 512}; //AnalysIR Batch Export (IRremote) - RAW
uint16_t EDI_VOL_DOWN_RAW[] = {8853, 4523, 512, 576, 491, 619, 469, 619, 469, 619, 469, 1707, 512, 597, 469, 619, 469, 619, 469, 1707, 512, 1685, 512, 1685, 512, 597, 469, 619, 469, 1707, 512, 1685, 512, 1685, 512, 1685, 512, 1685, 512, 1685, 512, 597, 469, 619, 469, 619, 469, 1707, 512, 597, 469, 619, 469, 619, 469, 619, 469, 1707, 512, 1685, 512, 1685, 512, 597, 469, 1685, 533, 41877, 8875, 2283, 512};                       //AnalysIR Batch Export (IRremote) - RAW
uint16_t EDI_MUTE_RAW[] = {8875, 4523, 512, 576, 512, 576, 512, 576, 512, 576, 512, 1707, 512, 576, 491, 597, 491, 597, 491, 1685, 533, 1685, 533, 1685, 533, 576, 491, 597, 491, 1685, 533, 1685, 512, 1685, 533, 1685, 533, 576, 491, 597, 491, 597, 491, 597, 491, 597, 491, 1707, 512, 576, 491, 597, 491, 1685, 533, 1685, 512, 1664, 533, 1685, 512, 1685, 512, 576, 512, 1685, 533, 41856, 8875, 2283, 512};                           //AnalysIR Batch Export (IRremote) - RAW

// TODO: does not seem to do what I want it to(speed up volume control)
const unsigned int SIGNAL_REPEAT = 1;

void setup()
{
  Serial.begin(9600);
  // In case the interrupt driver crashes on setup, give a clue
  // to the user what's going on.
  Serial.println("Enabling IRin");
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("Enabled IRin");
}

void loop()
{
  if (irrecv.getResults())
  {
    necDecoder.decode();

    Serial.println(necDecoder.value, HEX);
    Serial.println(Pnames(necDecoder.protocolNum));

    switch (necDecoder.value)
    {
    case LG_VOL_DOWN:
      for (size_t i = 0; i < SIGNAL_REPEAT; i++)
      {
        irsend.send(EDI_VOL_DOWN_RAW, sizeof(EDI_VOL_DOWN_RAW) / sizeof(EDI_VOL_DOWN_RAW[0]), 38);
      }
      break;
    case LG_VOL_UP:
      for (size_t i = 0; i < SIGNAL_REPEAT; i++)
      {
        irsend.send(EDI_VOL_UP_RAW, sizeof(EDI_VOL_UP_RAW) / sizeof(EDI_VOL_UP_RAW[0]), 38);
      }
      break;
    case LG_MUTE:
      for (size_t i = 0; i < SIGNAL_REPEAT; i++)
      {
        irsend.send(EDI_MUTE_RAW, sizeof(EDI_MUTE_RAW) / sizeof(EDI_MUTE_RAW[0]), 38);
      }
      break;
    default:
      break;
    }
  }

  irrecv.enableIRIn();
  delay(100);
}