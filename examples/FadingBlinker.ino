#include "FadingBlinker.hpp"

#define ledPin1 2
#define ledPin2 3

// instantiate FadingBlinker
FadingBlinker blinker(ledPin1, ledPin2);

void setup() {}

void loop()
{
 // run all functions
  blinker.activateLeft();
  delay(3000);
  blinker.activateRight();
  delay(3000);
  blinker.activateBoth();
  delay(3000);
  blinker.deactivate();
  delay(1000);
}

// Timer Callbacks
ISR(TIMER1_COMPB_vect)
{
  blinker.timerCallbackCOMPB();
}

ISR(TIMER1_COMPA_vect)
{
  blinker.timerCallbackCOMPA();
}
