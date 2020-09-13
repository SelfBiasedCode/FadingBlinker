#ifndef FADING_BLINKER_H
#define FADING_BLINKER_H

#include "Arduino.h"
#include "fadingblinker_data.hpp"

class FadingBlinker
{
	public:
	
	FadingBlinker(uint8_t ledLeftPin, uint8_t ledRightPin):m_leftPin(ledLeftPin), m_rightPin(ledRightPin), m_brightnessState(INACTIVE), m_currTimerDirUp(true)
	{
	{
		// set direction registers
		pinMode(m_leftPin, OUTPUT);
		pinMode(m_rightPin, OUTPUT);

		// set initial state
		digitalWrite(m_leftPin, HIGH);
		digitalWrite(m_rightPin, HIGH);
		
		// TODO: Use a pointer
		m_maxOCR1A = pgm_read_word_near(fadingblinker_data + 255);
		m_holdOff = pgm_read_word_near(fadingblinker_data + 256);
		m_holdOn = pgm_read_word_near(fadingblinker_data + 257);
	}

	inline void activateLeft()
	{ 
		m_directionState = LEFT_ACTIVE;
		m_setupTimer();
	}

	inline void activateRight()
	{
		m_directionState = RIGHT_ACTIVE;
		m_setupTimer();
	}

	inline void activateBoth()
	{
		m_directionState = BOTH_ACTIVE;
		m_setupTimer();
	}

	inline void deactivate()
	{
		m_directionState = INACTIVE;
	}

	void inline timerCallbackCOMPA()
	{
		// turn LEDs on if required
		if (m_brightnessState != OFF)
		{
			switch(m_directionState)
			{
				case LEFT_ACTIVE:
					digitalWrite(m_leftPin, HIGH);
					m_advanceTimer();
					break;
					
				case RIGHT_ACTIVE:
					digitalWrite(m_rightPin, HIGH);
					m_advanceTimer();
					break;
					
				case BOTH_ACTIVE:
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
		if (m_brightnessState != ON)
		{
			digitalWrite(m_leftPin, LOW);
			digitalWrite(m_rightPin, LOW);
		}
	}
	
	// direction states
	static const uint8_t INACTIVE = 0;
	static const uint8_t LEFT_ACTIVE = 1;
	static const uint8_t RIGHT_ACTIVE = 2;
	static const uint8_t BOTH_ACTIVE = 3;
	static const uint8_t MIN_VAL = 0x1A;
	
	// brightness states
	static const uint8_t OFF = 0;
	static const uint8_t UP = 1;
	static const uint8_t DOWN = 2;
	static const uint8_t ON = 3;
	
	
	private:
#if defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PA__)
	inline void m_setupTimer()
	{
		m_currVal = MIN_VAL;
		m_currTimerDirUp = true;

		// interrupts for COMPA and COMPB
		TIMSK1 |= (1 << OCIE1A) | (1 << OCIE1B);

		// prescaler: 1
		TCCR1B = (1 << CS10);// | (1 << CS10);

		// CTC mode
		TCCR1A = 0x0000;
		TCCR1B |= (1 << WGM12);

		// CTC Limit
		OCR1A = m_maxOCR1A;
	}

	inline void m_advanceTimer()
	{
		// set timer value for current state
		OCR1B = pgm_read_word_near(fadingblinker_data + m_currVal);
		
		// calculate next state
		switch(m_brightnessState)
		{
			case UP:
				if (m_currVal == 0xFF)
				{
					// this implies that ON holding time is at least 1 cycle
					m_brightnessState = ON;
					m_holdCounter = m_holdOn;
				}
				break;
			
			case ON:
				if (m_currVal == 0xFF)
				{
					// always decrement counter: one cycle has already passed since the transition from UP
					m_holdCounter--;
					if (m_holdCounter == 0)
					{
						m_brightnessState = DOWN;
					}
				}
				break;
				
			case DOWN:
				if (m_currVal == 0x00)
				{
					// this implies that OFF holding time is at least 1 cycle
					m_brightnessState = OFF;
					m_holdCounter = m_holdOff;
				}
			case OFF:
				if (m_currVal == 0xFF)
				{
					// always decrement counter: one cycle has already passed since the transition from DOWN
					m_holdCounter--;
					if (m_holdCounter == 0)
					{
						m_brightnessState = UP;
					}
				}
				break;
			
		}
		
		// calculate next value
		switch(m_brightnessState)
		{
			case UP:
				m_currVal++;
				break;
			case DOWN:
				m_currVal--;
				break;
		}	
	}
#endif
	
	// private members
	uint8_t m_leftPin;
	uint8_t m_rightPin;
	
	// direction state
	// TODO: use an enum
	uint8_t m_directionState;
	
	// brightness state
	// TODO: use an enum
	uint8_t m_brightnessState;
	
	uint8_t m_holdCounter;
	
	// constants
	
	bool m_currTimerDirUp;
	uint8_t m_currVal;
	uint16_t m_maxOCR1A;
	uint8_t m_holdOn;
	uint8_t m_holdOff;
	

};
#endif
