# FadingBlinker

## Summary

FadingBlinker is an Arduino component to provide a smooth fading direction indicator for vehicles. It is currently tested for the Atmel ATmega328P (e.g. Arduino Nano) and in development for the ATmega4809 (Arduino Every). It uses a python script to precalculate gamma correction tables and is optimized for performance.

## Technical Details
This component is written fully inline to maximize execution speed, especially for the ISRs. ISRs have to be created externally, which makes sure that interrupt vectors aren't spread all over the final code.

## Usage
### Table Generation
If necessary, edit the parameters in **fadingblinker_data_generator.py**, then run the script. It will create a header file **fadingblinker_data.hpp** which contains the necessary data.

### Integration
#### ATmega328P
FadingBlinker requires two interrupt vectors to be registered: *TIMER1_COMPA* and *TIMER1_COMPB*. Each must call the matching function in FadingBlinker (and could also execute other code if required). An eample can be found at the top of **FadingBlinker.hpp**.

### Setup
1. Instantiate the class. Parameters are the pin numbers for the left and right blinkers.
 
### Control
There are four functions available for indicator control:
* activateLeft(): Activates the left side blinker.
* activateRight(): Activates the right side blinker.
* activateBoth(): Activates both blinker sides (hazard lights).
* deactivate(): Deactivates both sides.

All of these functions set internal state variables for fast return. The currently active Timer1 cycle will be completed before the transition to the new state is made. This ensures fast switching and minimal performance overhead.

## Known Issues
* The current minimal timer value is used as a table index rather than an actual value.

## TODO
* add namespace