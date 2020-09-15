#ifndef FADING_BLINKER_H
#define FADING_BLINKER_H

#include "Arduino.h"
#include "fadingblinker_data.hpp"

// warn about unsupported architectures
#if !(defined(__AVR_ATmega328P__)|| defined(__AVR_ATmega4809__))
#warning Unknown platform, use at your own risk!
#endif

class FadingBlinker
{
	// forward declarations
private:
	enum class DirectionState;
	enum class BrightnessState;

public:
	FadingBlinker(uint8_t ledLeftPin, uint8_t ledRightPin, uint8_t buzzerPin) :m_leftPin(ledLeftPin), m_rightPin(ledRightPin), m_buzzerPin(buzzerPin), m_directionState(DirectionState::Inactive), m_brightnessState(BrightnessState::Up)
	{

		// set direction registers
		pinMode(m_leftPin, OUTPUT);
		pinMode(m_rightPin, OUTPUT);

		// set initial state
		digitalWrite(m_leftPin, HIGH);
		digitalWrite(m_rightPin, HIGH);
	}

	inline void activateLeft()
	{
		m_directionState = DirectionState::LeftActive;
		m_setupTimer();
	}

	inline void activateRight()
	{
		m_directionState = DirectionState::RightActive;
		m_setupTimer();
	}

	inline void activateBoth()
	{
		m_directionState = DirectionState::BothActive;
		m_setupTimer();
	}

	inline void deactivate()
	{
		m_directionState = DirectionState::Inactive;
	}

	void inline timerCallbackCOMPA()
	{
		// turn LEDs on if required
		if (m_brightnessState != BrightnessState::Off)
		{
			switch (m_directionState)
			{
			case DirectionState::LeftActive:
				digitalWrite(m_leftPin, HIGH);
				m_advanceTimer();
				break;

			case DirectionState::RightActive:
				digitalWrite(m_rightPin, HIGH);
				m_advanceTimer();
				break;

			case DirectionState::BothActive:
				digitalWrite(m_leftPin, HIGH);
				digitalWrite(m_rightPin, HIGH);
				m_advanceTimer();
				break;

			default:
				break;
			}
		}
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
	// TODO: Move to constructor
#if defined(__AVR_ATmega328P__)
	inline void m_setupTimer()
	{
		m_currTableIndex = 0x00;
		m_brightnessState = BrightnessState::Up;

		// interrupts for COMPA and COMPB
		TIMSK1 |= (1 << OCIE1A) | (1 << OCIE1B);

		// prescaler: 1
		TCCR1B = (1 << CS10);

		// CTC mode
		TCCR1A = 0x0000;
		TCCR1B |= (1 << WGM12);

		// CTC Limit
		OCR1A = fadingblinker_data.timerTop;
	}

	inline void m_setCompareValue()
	{
		OCR1B = fadingblinker_data.pwmData[m_currTableIndex];
	}

#elif defined (__AVR_ATmega4809__)

	inline void m_setupTimer()
	{
		m_currTableIndex = 0x00;
		m_brightnessState = BrightnessState::Up;

		// Reverse PORTMUX setting
		PORTMUX.TCAROUTEA &= ~(PORTMUX_TCA0_PORTB_gc);

		// Period setting
		// TODO: Is -1 still necessary?
		TCA0.SINGLE.PER = fadingblinker_data.timerTop - 1;

		// Interrupts
		TCA0.SINGLE.INTCTRL = (TCA_SINGLE_CMP0_bm | TCA_SINGLE_OVF_bm);
	}

	inline void m_setCompareValue()
	{
		TCA0.SINGLE.CMP0 = fadingblinker_data.pwmData[m_currTableIndex];
	}
#endif

	/* platform independent code */
	inline void m_advanceTimer()
	{
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
				m_holdCounter = fadingblinker_data.holdOn;
				tone(m_buzzerPin, fadingblinker_data.buzzerFreq);
			}
			break;

		case BrightnessState::On:
			if (m_currTableIndex == 0xFF)
			{
				// always decrement counter: one cycle has already passed since the transition from UP
				m_holdCounter--;
				if (m_holdCounter == 0)
				{
					m_brightnessState = BrightnessState::Down;
					noTone(m_buzzerPin);
				}
			}
			break;

		case BrightnessState::Down:
			if (m_currTableIndex == 0x00)
			{
				// this implies that OFF holding time is at least 1 cycle
				m_brightnessState = BrightnessState::Off;
				m_holdCounter = fadingblinker_data.holdOff;
			}
		case BrightnessState::Off:
			if (m_currTableIndex == 0xFF)
			{
				// always decrement counter: one cycle has already passed since the transition from DOWN
				m_holdCounter--;
				if (m_holdCounter == 0)
				{
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
	uint8_t m_buzzerPin;

	// direction state
	DirectionState m_directionState;

	// brightness state
	BrightnessState m_brightnessState;

	// constants from generated data
	fadingblinker_data_struct fadingblinker_data;

	// variables
	uint8_t m_currTableIndex;
	uint8_t m_holdCounter;

	// direction states
	enum class DirectionState { Inactive, LeftActive, RightActive, BothActive };

	// brightness states
	enum class BrightnessState { Off, Up, On, Down };
};

#endif
