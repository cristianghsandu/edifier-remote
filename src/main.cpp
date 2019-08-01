#include <IRLibSendBase.h>
#include <IRLibDecodeBase.h>
#include <IRLib_P01_NEC.h>
#include <IRLibCombo.h>
#include <IRLibRecvPCI.h>

const int RECV_PIN = 2;

const uint32_t LG_VOL_UP = 0xEF00FF;
const uint32_t LG_VOL_DOWN = 0xEF807F;
const uint32_t LG_MUTE = 0xEF6897;

const uint32_t EDI_VOL_UP = 0x08E7609F;
const uint32_t EDI_VOL_DOWN = 0x08E7E21D;
const uint32_t EDI_VOL_MUTE = 0x08E7827D;
const uint32_t EDI_REPEAT = 0xFFFFFFFF;

IRrecvPCI irrecv(RECV_PIN);
IRsend irsend; // PWM pin 3

IRdecode necDecoder;

// TODO: does not seem to do what I want it to(speed up volume control)
const unsigned int SIGNAL_REPEAT = 10;

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

    // Serial.println(necDecoder.value, HEX);
    // Serial.println(Pnames(necDecoder.protocolNum));

    irrecv.disableIRIn();
    switch (necDecoder.value)
    {
    case LG_VOL_DOWN:
      irsend.send(NEC, EDI_VOL_DOWN, 32, 38);
      for (size_t i = 0; i < SIGNAL_REPEAT; i++)
      {
        irsend.send(NEC, EDI_REPEAT, 32, 38);
      }
      break;
    case LG_VOL_UP:
      irsend.send(NEC, EDI_VOL_UP);
      for (size_t i = 0; i < SIGNAL_REPEAT; i++)
      {
        irsend.send(NEC, EDI_REPEAT, 0);
      }
      break;
    case LG_MUTE:
      irsend.send(NEC, EDI_VOL_MUTE, 32, 38);
      for (size_t i = 0; i < SIGNAL_REPEAT; i++)
      {
        irsend.send(NEC, EDI_REPEAT, 32, 38);
      }
      break;
    default:
      break;
    }
  }

  delay(100);
  irrecv.enableIRIn();
}