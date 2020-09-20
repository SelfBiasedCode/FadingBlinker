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
    enum class OperationState : uint8_t;
      enum class BrightnessState : uint8_t;

    public:
      FadingBlinker(uint8_t ledLeftPin, uint8_t ledRightPin, uint8_t buzzerPin) : m_leftPin(ledLeftPin), m_rightPin(ledRightPin), m_buzzerPin(buzzerPin), m_OperationState(OperationState::Inactive), m_brightnessState(BrightnessState::Up)
    {

      // set direction registers
      pinMode(m_leftPin, OUTPUT);
      pinMode(m_rightPin, OUTPUT);

      // set initial state
      digitalWrite(m_leftPin, HIGH);
      digitalWrite(m_rightPin, HIGH);

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
            //m_advanceTimer();
            break;

          case OperationState::BothActive:
          case OperationState::Flash:
            digitalWrite(m_leftPin, HIGH);
            digitalWrite(m_rightPin, HIGH);
            //m_advanceTimer();
            break;

          default:
            break;
        }
      }
       m_advanceTimer();
    }

    void inline timerCallbackCOMPB()
    {
      // turn LEDs off
      //if (m_brightnessState != BrightnessState::On)
      //{
        digitalWrite(m_leftPin, LOW);
        digitalWrite(m_rightPin, LOW);
      //}
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
      OCR1A = fadingblinker_data.timerTop;
    }

    inline void m_setCompareValue()
    {
      OCR1B = fadingblinker_data.pwmData[m_currTableIndex];
    }

#elif defined (__AVR_ATmega4809__)

    inline void m_setupTimer()
    {
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
    inline void m_startOperation(OperationState newState)
    {
      // do not restart already running programs
      if (newState == m_OperationState)
        return;

      m_OperationState = newState;

      // reset dimming index
      m_currTableIndex = 0x00;

      // initialize state machine depending on operation mode
      if (m_OperationState == OperationState::Flash)
      {
        m_brightnessState = BrightnessState::On;

        // the hold counter has to be preloaded as it would usually be set during state transition
        m_holdCounter = fadingblinker_data.flashCycles;
      }
      else
      {
        m_brightnessState = BrightnessState::Up;
      }
    }

    inline void m_advanceTimer()
    {
      if (m_OperationState == OperationState::Inactive)
        return;

      
      // set timer value for current state
      m_setCompareValue();

      Serial.print("Index ");
      Serial.print(m_currTableIndex);
      Serial.print(", Value ");
      Serial.print(TCA0.SINGLE.CMP0);
      Serial.print(", OpState ");
      Serial.print((uint8_t)m_OperationState);
      Serial.print(", BrightState ");
      Serial.println((uint8_t)m_brightnessState);

      // calculate next state and control buzzer
      switch (m_brightnessState)
      {
        case BrightnessState::Up:
          if (m_currTableIndex == 0xFF)
          {
            // this implies that ON holding time is at least 1 cycle
            m_brightnessState = BrightnessState::On;
            m_holdCounter = fadingblinker_data.holdOnCycles;
            tone(m_buzzerPin, fadingblinker_data.buzzerFreq);
          }
          break;

        case BrightnessState::On:

          // always decrement counter: one cycle has already passed since the transition from UP
          m_holdCounter--;
          if (m_holdCounter == 0)
          {
            // turn off buzzer
            noTone(m_buzzerPin);

            // go to next state depending on operation mode
            if (m_OperationState == OperationState::Flash)
            {
              m_brightnessState = BrightnessState::Off;
              m_holdCounter = fadingblinker_data.flashCycles;
            }
            else
            {
              m_brightnessState = BrightnessState::Down;
            }
          }

          break;

        case BrightnessState::Down:
          if (m_currTableIndex == 0x00)
          {
            // this implies that OFF holding time is at least 1 cycle
            m_brightnessState = BrightnessState::Off;
            m_holdCounter = fadingblinker_data.holdOffCycles;
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
              m_brightnessState = BrightnessState::On;
              m_holdCounter = fadingblinker_data.flashCycles;
            }
            else
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
