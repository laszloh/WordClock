
#include <Arduino.h>
#include <Schedule.h>

#include <c++23.h>

#include "Button.h"
#include "esp-hal-log.h"

void Button::begin(uint8_t pin, bool activeLow, bool intPullup) {
    _pin = pin;
    _buttonPressed = !activeLow;

    pinMode(pin, (intPullup) ? INPUT_PULLUP : INPUT);
}

void Button::loop() {
    if(_pin)
        tick(digitalRead(_pin) == _buttonPressed);
}

void IRAM_ATTR Button::levelInterruptHandler(void *button) {
    Button *btn = static_cast<Button *>(button);
    if(btn == nullptr)
        return;

    detachInterrupt(btn->_pin);
    schedule_function([]() { log_d("GPIO wakeup IRQ"); });
}

void Button::armWakeup() {
    attachInterruptArg(_pin, Button::levelInterruptHandler, this, ONLOW_WE);
}

void Button::tick(bool activeLevel) {
    const auto now = millis();
    const auto waitTime = now - _startTime;

    // log_d("state: %d, activeLevel: %c, waitTime: %d", std::to_underlying(_state), (activeLevel) ? '1' : '0', waitTime);
    // log_d("debounce: %c, startTime: %d", (_debouncedState) ? '1' : '0', _startTime);

    switch(_state) {
        case StateMachine::init:
            if(activeLevel) {
                fsmUpdate(StateMachine::down);
                _startTime = now;
                _nClicks = 0;
            }
            break;

        case StateMachine::down:
            // button is pressed...

            if(!activeLevel) {
                // and was released
                if((waitTime < _debounceTime))
                    fsmUpdate(_lastState);
                else {
                    // button was released
                    fsmUpdate(StateMachine::up);
                    _debouncedState = activeLevel;
                    _startTime = now;
                }
            } else {
                // check for a long press
                if(waitTime > _pressTime) {
                    if(_longPressStartCb)
                        _longPressStartCb();
                    _longPressState = activeLevel;
                    _debouncedState = activeLevel;
                    fsmUpdate(StateMachine::press);
                }
            }
            break;

        case StateMachine::up:
            // button is released...
            if(activeLevel && waitTime < _debounceTime) {
                fsmUpdate(_lastState);
            } else if(waitTime >= _debounceTime) {
                // this is a click
                _nClicks++;
                _debouncedState = activeLevel;
                fsmUpdate(StateMachine::count);
            }
            break;

        case StateMachine::count:
            // debounce is over, count clicks
            if(activeLevel) {
                // button was pressed ... again
                fsmUpdate(StateMachine::down);
                _startTime = now;
            } else if(waitTime > _clickTime || _nClicks >= _maxClicks) {
                // we have collected all the clicks
                if(_nClicks == 1) {
                    if(_clickCb)
                        _clickCb();
                } else if(_nClicks == 2) {
                    if(_doubleClickCb)
                        _doubleClickCb();
                } else {
                    if(_multiClickCb)
                        _multiClickCb();
                }
                reset();
            }
            break;

        case StateMachine::press:
            // button is pressed
            if(!activeLevel) {
                fsmUpdate(StateMachine::pressEnd);
                _startTime = now;
            }
            break;

        case StateMachine::pressEnd:
            // button was released
            if(activeLevel && (waitTime < _debounceTime)) {
                // debounce
                fsmUpdate(_lastState);
            } else if(waitTime >= _debounceTime) {
                if(_longPressStopCb)
                    _longPressStopCb();
                _longPressState = !activeLevel;
                reset();
            }

            break;
    }
}
