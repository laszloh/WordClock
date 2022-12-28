#pragma once

#include <functional>

class Button {
public:
    explicit Button() { }

    void begin(uint8_t pin, bool activeLow = true, bool intPullup = true);

    void loop();

    bool read();
    bool pressed();
    bool released();

    void setDebounceMs(uint16_t ms) { _debounceTime = ms; }
    void setClickMs(uint16_t ms) { _clickTime = ms; }
    void setPressMs(uint16_t ms) { _pressTime = ms; }
    void setMaxClicks(uint8_t clicks) { _maxClicks = std::min(clicks, maxNrClicks); }

    using CallbackFunction = std::function<void(void)>;
    void attachClickCallback(CallbackFunction func) { _clickCb = func; }
    void attachDoubleClickCallback(CallbackFunction func) {
        _doubleClickCb = func;
        _maxClicks = std::max(_maxClicks, uint8_t(2));
    }
    void attachMultiClickCallback(CallbackFunction func, uint8_t maxClicks = 3) {
        _multiClickCb = func;
        _maxClicks = std::min(maxClicks, maxNrClicks);
    }
    void attachLogPressStart(CallbackFunction func) { _longPressStartCb = func; }
    void attachLogPressStop(CallbackFunction func) { _longPressStopCb = func; }

private:
    static constexpr uint8_t maxNrClicks = 100;
    
    static void interruptHandler(void *button);

    enum class StateMachine : uint8_t { init = 0, down, up, count, press, pressEnd };

    void tick(bool active);

    constexpr void reset() {
        _state = _lastState = StateMachine::init;
        _nClicks = 0;
        _startTime = 0;
    }

    constexpr void fsmUpdate(StateMachine newState) {
        _lastState = _state;
        _state = newState;
    }

    int _pin;
    bool _buttonPressed{false};
    bool _debouncedState;
    bool _interruptTrigger{false};

    uint16_t _debounceTime{50};
    uint16_t _clickTime{300};
    uint16_t _pressTime{900};

    CallbackFunction _clickCb{nullptr};
    CallbackFunction _doubleClickCb{nullptr};
    CallbackFunction _multiClickCb{nullptr};
    CallbackFunction _longPressStartCb{nullptr};
    CallbackFunction _longPressStopCb{nullptr};

    StateMachine _lastState{StateMachine::init};
    StateMachine _state{StateMachine::init};
    uint32_t _startTime{0};

    uint8_t _nClicks{0};
    uint8_t _maxClicks{1};
};
