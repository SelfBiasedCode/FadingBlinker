# FadingBlinker

## Summary

FadingBlinker is an Arduino component to provide a smooth fading direction indicator for vehicles. Also it features a buzzer output for blinker indication.
It is currently tested for the Atmel ATmega328P (e.g. Arduino Nano) and the ATmega4809 (Arduino Every). It uses a python script to precalculate gamma correction tables and is optimized for performance.

## Technical Details
This component is written fully inline to maximize execution speed, especially for the ISRs. ISRs have to be defined outside this component, making sure that interrupt vectors aren't spread all over the final code. PWM tables are **not** written to *progmem* to minimize read delays during ISRs execution. 

### Limitations
For the ATmega4809, TCA0 is used. This interferes with Arduino's analogWrite() function. Sadly the Arduino framework takes hold of TCA0, TCB0, TCB2 and possibly more timers in such a way that there is no way to run Soft-PWM at an acceptable frequency on a free TCB timer. **As a result, analogWrite() will not work on the ATmega 4809 (e.g. Arduino Every)**.

## Usage
### Table Generation
If necessary, edit the parameters in **fadingblinker_data_generator.py**, then run the script. It will create a header file **fadingblinker_data.hpp** which contains the necessary data.

### Integration
#### ATmega328P
FadingBlinker requires two interrupt vectors to be registered: *TIMER1_COMPA* and *TIMER1_COMPB*. Each must call the matching function in FadingBlinker (and could also execute other code if required). Example:

    ISR(TIMER1_COMPA_vect)
    {
	    FadingBlinker.timerCallbackCOMPA();
    }
    
    ISR(TIMER1_COMPB_vect)
    {
	    FadingBlinker.timerCallbackCOMPB();
    }
#### ATmega4809
Similar to the 328P, two interrupt vectors must be use, though they're named OVF and CMP0:

    ISR(TCA0_OVF_vect)
    {
      blinker.timerCallbackCOMPA();
      TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;
    }
    
    ISR(TCA0_CMP0_vect)
    {
      blinker.timerCallbackCOMPB();
      TCA0.SINGLE.INTFLAGS = TCA_SINGLE_CMP0_bm;
    }

### Setup
1. Instantiate the class. Parameters are the pin numbers for the left and right blinkers plus the buzzer.
2. Call FadingBlinker.init(). This is necessary to undo any changes to the timer configuration done by Arduino. 
 
### Control
There are five functions available for indicator control:
* activateLeft(): Activates the left side blinker.
* activateRight(): Activates the right side blinker.
* activateBoth(): Activates both blinker sides (hazard lights).
* flashBoth(): Rapidly flash both blinkers.
* deactivate(): Deactivates both sides.

All activate*() functions also actuate the buzzer.

All of these functions set internal state variables for fast return. The currently active timer cycle will be completed before the transition to the new state is made. This ensures fast switching and minimal performance overhead.

## Known Issues
None

## TODO
* add namespace
* allow disabling of the buzzer functionality