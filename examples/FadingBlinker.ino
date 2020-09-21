#include "FadingBlinker.hpp"

#define ledPin1 2
#define ledPin2 3
#define buzzerPin 4

// instantiate FadingBlinker
FadingBlinker blinker(ledPin1, ledPin2, buzzerPin);

void setup()
{
  // initialize timer
  blinker.init();
}

void loop()
{
  // run all functions
  blinker.activateLeft();;
  delay(3000);
  blinker.activateRight();
  delay(3000);
  blinker.activateBoth();
  delay(3000);
  blinker.flashBoth();
  delay(3000);
  blinker.deactivate();
  delay(1000);

}

// Timer Callbacks
#if defined(__AVR_ATmega328P__)

ISR(TIMER1_COMPB_vect)
{
  blinker.timerCallbackCOMPB();
}

ISR(TIMER1_COMPA_vect)
{
  blinker.timerCallbackCOMPA();
}

#elif defined(__AVR_ATmega4809__)

ISR(TCA0_OVF_vect)
{
  // execute blinker callback
  blinker.timerCallbackCOMPA();

  // reset interrupt flag
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
}

ISR(TCA0_CMP0_vect)
{
  // execute blinker callback
  blinker.timerCallbackCOMPB();

  // reset interrupt flag
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;
}

#endif
