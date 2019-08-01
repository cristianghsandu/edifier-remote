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

const unsigned int SIGNAL_REPEAT = 10;

unsigned long time;

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
    time = millis();

    necDecoder.decode();

    // Serial.println(necDecoder.value, HEX);
    // Serial.println(Pnames(necDecoder.protocolNum));
    delay(100);
    
    switch (necDecoder.value)
    {
    case LG_VOL_DOWN:
      irsend.send(NEC, EDI_VOL_DOWN);
      break;
    case LG_VOL_UP:
      irsend.send(NEC, EDI_VOL_UP);
      break;
    case LG_MUTE:
      irsend.send(NEC, EDI_VOL_MUTE);
      break;
    default:
      break;
    }
  }

  irrecv.enableIRIn();
}