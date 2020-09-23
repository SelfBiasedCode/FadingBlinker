#ifndef FANCYBLINKER_H
#define FANCYBLINKER_H

#include "Arduino.h"
#include "FancyBlinker_Data.hpp"

// warn about unsupported architectures
#if !(defined(__AVR_ATmega328P__)|| defined(__AVR_ATmega4809__))
#warning Unknown platform, use at your own risk!
#endif

class FancyBlinker
{
	// forward declarations
private:
	enum class OperationState : uint8_t;
	enum class BrightnessState : uint8_t;

public:

	// different constructors are used with and without beeper functionality
#if FB_BEEPER_ENABLED

	FancyBlinker(uint8_t ledLeftPin, uint8_t ledRightPin, uint8_t beeperPin) : m_leftPin(ledLeftPin), m_rightPin(ledRightPin), m_beeperPin(beeperPin), m_OperationState(OperationState::Inactive), m_brightnessState(BrightnessState::Up)
	{

		// set direction registers
		pinMode(m_leftPin, OUTPUT);
		pinMode(m_rightPin, OUTPUT);

		// set initial state
		digitalWrite(m_leftPin, LOW);
		digitalWrite(m_rightPin, LOW);
	}

#else

	FancyBlinker(uint8_t ledLeftPin, uint8_t ledRightPin) : m_leftPin(ledLeftPin), m_rightPin(ledRightPin), m_OperationState(OperationState::Inactive), m_brightnessState(BrightnessState::Up)
	{

		// set direction registers
		pinMode(m_leftPin, OUTPUT);
		pinMode(m_rightPin, OUTPUT);

		// set initial state
		digitalWrite(m_leftPin, LOW);
		digitalWrite(m_rightPin, LOW);
	}

#endif

	inline void init()
	{
		// initialize timer (platform dependent)
		m_setupTimer();
	}

	inline void activateLeft()
	{
		m_startOperation(OperationState::LeftActive);
	}

	inline void activateRight()
	{
		m_startOperation(OperationState::RightActive);
	}

	inline void activateBoth()
	{
		m_startOperation(OperationState::BothActive);
	}

	inline void flashBoth()
	{
		m_startOperation(OperationState::Flash);
	}

	inline void deactivate()
	{
		// timer reset is not necessary for deactivation
		m_OperationState = OperationState::Inactive;
	}

	void inline timerCallbackCOMPA()
	{
		// turn LEDs on if required
		if (m_brightnessState != BrightnessState::Off)
		{
			switch (m_OperationState)
			{
			case OperationState::LeftActive:
				digitalWrite(m_leftPin, HIGH);
				break;

			case OperationState::RightActive:
				digitalWrite(m_rightPin, HIGH);
				break;

			case OperationState::BothActive:
			case OperationState::Flash:
				digitalWrite(m_leftPin, HIGH);
				digitalWrite(m_rightPin, HIGH);
				break;

			default:
				break;
			}
		}

		// always advance timer
		m_advanceTimer();
	}

	void inline timerCallbackCOMPB()
	{
		// turn LEDs off
		if (m_brightnessState != BrightnessState::On)
		{
			digitalWrite(m_leftPin, LOW);
			digitalWrite(m_rightPin, LOW);
		}
	}

private:

	/* platform dependent code */

#if defined(__AVR_ATmega328P__)

	inline void m_setupTimer()
	{
		// interrupts for COMPA and COMPB
		TIMSK1 |= (1 << OCIE1A) | (1 << OCIE1B);

		// prescaler: 1
		TCCR1B = (1 << CS10);

		// CTC mode
		TCCR1A = 0x0000;
		TCCR1B |= (1 << WGM12);

		// CTC Limit
		OCR1A = FancyBlinker_Data.timerTop;
	}

	inline void m_setCompareValue()
	{
		OCR1B = FancyBlinker_Data.pwmData[m_currTableIndex];
	}

#elif defined (__AVR_ATmega4809__)

	inline void m_setupTimer()
	{
		// Reverse PORTMUX setting from Arduino
		PORTMUX.TCAROUTEA &= ~(PORTMUX_TCA0_PORTB_gc);

		// Set period
		TCA0.SINGLE.PER = FancyBlinker_Data.timerTop;

		// Set to Single Slope PWM and synchronize register updates
		TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc | TCA_SINGLE_ALUPD_bm;

		// Activate Interrupts
		TCA0.SINGLE.INTCTRL = (TCA_SINGLE_CMP0_bm | TCA_SINGLE_OVF_bm);
	}

	inline void m_setCompareValue()
	{
		TCA0.SINGLE.CMP0BUF = FancyBlinker_Data.pwmData[m_currTableIndex];
	}

#endif

	/* platform independent code */
	inline void m_startOperation(OperationState newState)
	{
		// do not restart already running programs
		if (newState == m_OperationState)
			return;

		// reset state machine
		m_OperationState = newState;

		// reset dimming index
		m_currTableIndex = 0x00;

		// initialize state machine depending on operation mode
		if (m_OperationState == OperationState::Flash)
		{
			
#if FB_BEEPER_ENABLED
			// turn buzzer off
			noTone(m_beeperPin);
#endif
			// move to next state
			m_brightnessState = BrightnessState::On;

			// the hold counter has to be preloaded as it would usually be set during state transition
			m_holdCounter = FancyBlinker_Data.flashCycles;
		}
		else
		{

#if FB_BEEPER_ENABLED
			// turn buzzer on
			tone(m_beeperPin, FancyBlinker_Data.beeperFreq);

#endif
			m_brightnessState = BrightnessState::Up;
		}
	}

	inline void m_advanceTimer()
	{
		if (m_OperationState == OperationState::Inactive)
			return;

		// set timer value for current state
		m_setCompareValue();

		// calculate next state and control buzzer
		switch (m_brightnessState)
		{
		case BrightnessState::Up:
			if (m_currTableIndex == 0xFF)
			{
				// this implies that ON holding time is at least 1 cycle
				m_brightnessState = BrightnessState::On;
				m_holdCounter = FancyBlinker_Data.holdOnCycles;

#if FB_BEEPER_ENABLED
				// turn off buzzer
				noTone(m_beeperPin);
#endif
			}
			break;

		case BrightnessState::On:
			// always decrement counter: one cycle has already passed since the transition from UP
			m_holdCounter--;
			if (m_holdCounter == 0)
			{
				// go to next state depending on operation mode
				if (m_OperationState == OperationState::Flash)
				{
					//move to next state
					m_brightnessState = BrightnessState::Off;
					m_holdCounter = FancyBlinker_Data.flashCycles;
				}
				else
				{
					//move to next state
					m_brightnessState = BrightnessState::Down;
				}
			}
			break;

		case BrightnessState::Down:
			if (m_currTableIndex == 0x00)
			{
				// this implies that OFF holding time is at least 1 cycle
				m_brightnessState = BrightnessState::Off;
				m_holdCounter = FancyBlinker_Data.holdOffCycles;
			}
			break;

		case BrightnessState::Off:
			// always decrement counter: one cycle has already passed since the transition from DOWN
			m_holdCounter--;
			if (m_holdCounter == 0)
			{
				// go to next state depending on operation mode
				if (m_OperationState == OperationState::Flash)
				{
					//move to next state
					m_brightnessState = BrightnessState::On;
					m_holdCounter = FancyBlinker_Data.flashCycles;
				}
				else
				{

#if FB_BEEPER_ENABLED
					// activate buzzer
					tone(m_beeperPin, FancyBlinker_Data.beeperFreq);
#endif

					//  move to next state
					m_brightnessState = BrightnessState::Up;
				}
			}
			break;
		}

		// calculate next value
		switch (m_brightnessState)
		{
		case BrightnessState::Up:
			m_currTableIndex++;
			break;
		case BrightnessState::Down:
			m_currTableIndex--;
			break;
		}
	}

	// pin assignments
	uint8_t m_leftPin;
	uint8_t m_rightPin;

#if FB_BEEPER_ENABLED
	uint8_t m_beeperPin;
#endif

	// direction state
	OperationState m_OperationState;

	// brightness state
	BrightnessState m_brightnessState;

	// variables
	uint8_t m_currTableIndex;
	uint8_t m_holdCounter;

	// direction states
	enum class OperationState : uint8_t { Inactive, LeftActive, RightActive, BothActive, Flash };

	// brightness states
	enum class BrightnessState : uint8_t { Off, Up, On, Down };
};

#endif
