#include "FadingBlinker.hpp"

#define ledPin1 2
#define ledPin2 3

// instantiate FadingBlinker
FadingBlinker blinker(ledPin1, ledPin2);

void setup()
{
  //Serial.begin(9600);   // Initiate serial to PC Host
  //Serial.println("Setup()");
}

void loop()
{
  // run all functions

  blinker.activateLeft();
  delay(3000);
  blinker.deactivate();
  delay(1000);


  /*
    blinker.activateRight();
    delay(3000);
    blinker.activateBoth();
    delay(3000);

  */
}

// Timer Callbacks
ISR(TCA0_CMP0_vect)
{
  //Serial.println("CMP0");
  blinker.timerCallbackCOMPB();

  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP0_bm;

}

ISR(TCA0_OVF_vect)
{
  //Serial.println("OVF");
  blinker.timerCallbackCOMPA();

  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_OVF_bm;
}
