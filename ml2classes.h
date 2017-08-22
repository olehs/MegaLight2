#ifndef ML2CLASSES_H
#define ML2CLASSES_H

#include <SimpleList.h>
#include <EventManager.h>
#include <ExpressionEvaluator.h>

#include "ml2enums.h"

#define ID_SIZE 13
#define PWM_HIGH 255

uint32_t parseTime(const char* v);

class OutputEventParam : public EventParam
{
  public:
    OutputEventParam(void *sender, int param, uint32_t timeout = 0) : EventParam(sender, param), timeout(timeout) {}
    uint32_t timeout;
};

class ML2Output;

class OutputList : public SimpleList<ML2Output *>
{
  public:
    OutputList();

    bool addOutput(ML2Output *output);
    bool removeOutput(ML2Output *output);
    bool hasOutput(ML2Output *output);
    ML2Output *find(const char *id);

    void check();
    void clearOutputs();
  private:
};

class ML2Output
{
  public:
    ML2Output(const String &id);

    char ID[ID_SIZE];
    int storeAddress;

    inline byte pin() {
      return m_pin;
    }
    inline bool pwm() {
      return m_pwm;
    }
    inline bool noreport() {
      return m_noreport;
    }

    void setPin(byte pin);
    void setPWM(bool on);
    void setInvert(bool inv);
    void setNoreport(bool no);

    inline byte value() {
      return m_value;
    }
    inline bool on() {
      return m_on;
    }
    inline bool off() {
      return !m_on;
    }

    void setOn(uint32_t timeout = 0);
    void setOff(uint32_t timeout = 0);
    bool toggle(uint32_t timeout = 0);
    void setValue(byte value, uint32_t timeout = 0);
    void incValue(int val, uint32_t timeout = 0);

    uint32_t timeout();
    bool invert();

    bool action(OutputAction::Action action, int param = 0, uint32_t timeout = 0);
    void check(uint32_t millisec = 0);

    OutputStateSave::Save saveState();

    void setSaveState(OutputStateSave::Save saveState);


  protected:
    byte m_pin;
    bool m_pwm;
    byte m_value;
    bool m_on;
    bool m_invert;
    bool m_noreport;

    OutputStateSave::Save m_saveState;
    uint32_t m_timeout;
    uint32_t m_dim_t0, m_dim_t1;
    byte m_dim_v0, m_dim_v1;

    void setupPin();
    void updatePin(uint32_t timeout = 0, bool doEmit = true);
    void emitState();

};

#define DOUBLE_CLICK_INTERVAL 400
#define HOLD_INTERVAL 500
#define REPEAT_INTERVAL 250
#define BOUNCE_INTERVAL 1

#include "ml2enums.h"
#include <Bounce2.h>
#include <SimpleList.h>
#include <EventManager.h>

class ML2Input;

class InputList : public SimpleList<ML2Input *>
{
  public:
    InputList();

    bool addInput(ML2Input *input);
    bool removeInput(ML2Input *input);
    bool hasInput(ML2Input *input);
    ML2Input *find(const char *id);
    bool addInputRule(const char *inputId, int addrRule);

    void check();
    void clearInputs();

  private:
};

class ML2Input : public Bounce
{
  public:
    ML2Input(const String &id);

    char ID[ID_SIZE];
    SimpleList<int> rules;

    inline byte pin() {
      return m_pin;
    }
    inline InputPullup::PullupType pullup() {
      return m_pullup;
    }
    inline uint16_t bounceInterval() {
      return m_bounceInterval;
    }
    inline uint16_t holdInterval() {
      return m_holdInterval;
    }
    inline bool repeat() {
      return m_repeat;
    }
    inline uint16_t repeatInterval() {
      return m_repeatInterval;
    }
    inline uint16_t doubleClickInterval() {
      return m_doubleClickInterval;
    }
    inline bool preventClick() {
      return m_preventClick;
    }

    void setPin(byte pin);
    void setPullup(InputPullup::PullupType pullup = InputPullup::IntPullup);
    void setBounceInterval(uint16_t bounceInterval);

    void setRepeat(bool repeat = true);
    void setRepeatInterval(uint16_t repeatInterval);
    void setHoldInterval(uint16_t holdInterval);
    void setDoubleClickInterval(uint16_t dClickInterval);
    void setPreventClick(bool preventClick = true);

    void setDoubleClick(uint16_t dClickInterval = DOUBLE_CLICK_INTERVAL, bool preventClick = true);

    void check(uint32_t millisec = 0);
    bool pressed();
    bool released();

    inline bool isPullup() {
      return m_pullup != InputPullup::PullDown;
    }

    inline bool hold() {
      return isHold;
    }
    bool down();
    bool up();
    inline byte state() {
      return bState;
    }


  protected:
    void queueEvent(int event);
    void reset();

  protected:
    int m_pin;
    InputPullup::PullupType m_pullup;
    uint16_t m_bounceInterval;

    bool m_repeat;
    uint16_t m_holdInterval;
    uint16_t m_repeatInterval;

    uint16_t m_doubleClickInterval;
    bool m_preventClick;

    uint32_t buttonPressTimeStamp;
    uint32_t lastRepeatTimeStamp;
    bool isHold;
    byte clickCount;
    byte bState;

};

#define RULES_PATH "/RULES"

class ML2Rule
{
  public:
    struct EventAction {
      OutputAction::Action action;
      int param;
      uint32_t timeout;
      String condition;
    };

    ML2Rule(const String &id);
    ~ML2Rule();

    static ML2Rule *fromFile(const String &path);

    String ID;
    bool final;

    bool addInput(const char *id);

    bool addOutput(const char *id);
    bool removeOutput(const char *id);
    bool hasOutput(const char *id);
    bool hasOutput(ML2Output *output);
    int outputCount();
    void clearOutputs();

    inline InputList *inputs() {
      return &inputlist;
    }

    inline OutputList *outputs() {
      return &outputlist;
    }

    EventAction &eventAction(ButtonEvent::Type event) {
      return eventActions[event];
    }

    void setAction(ButtonEvent::Type event, OutputAction::Action action, int param = 0, uint32_t timeout = 0);
    void unsetAction(ButtonEvent::Type event);
    void setCondition(ButtonEvent::Type event, const char *condition);

    bool processButtonEvent(int event, ML2Input *input);

  private:
    InputList inputlist;
    OutputList outputlist;
    EventAction eventActions[ButtonEvent::EventsCount];
};



#endif //ML2CLASSES_H
