#include "ml2classes.h"

extern EventManager inputEM;

inline bool byOrder(String a, String b) {
  return a > b;
}

ML2Input::ML2Input(const String &id)
  : Bounce()
  , m_pin(0)
  , m_pullup(InputPullup::PullDown)
  , m_bounceInterval(BOUNCE_INTERVAL)
  , m_holdInterval(HOLD_INTERVAL)
  , m_repeat(false)
  , m_repeatInterval(REPEAT_INTERVAL)
  , m_doubleClickInterval(0)
  , m_preventClick(false)
  , buttonPressTimeStamp(0)
  , lastRepeatTimeStamp(0)
  , clickCount(0)
  , isHold(false)
{
  id.toCharArray(ID, ID_SIZE);
  this->setPin(m_pin);
  this->setPullup(m_pullup);
}

void ML2Input::setPin(byte pin)
{
  this->m_pin = pin;
  reset();
}

void ML2Input::setRepeat(bool repeat)
{
  this->m_repeat = repeat;
}

void ML2Input::setHoldInterval(uint16_t holdInterval)
{
  this->m_holdInterval = holdInterval;
}

void ML2Input::setDoubleClickInterval(uint16_t dClickInterval)
{
  this->m_doubleClickInterval = dClickInterval;
}

void ML2Input::setPreventClick(bool preventClick)
{
  this->m_preventClick = preventClick;
}

void ML2Input::setRepeatInterval(uint16_t repeatInterval)
{
  this->m_repeatInterval = repeatInterval;
}

void ML2Input::setDoubleClick(uint16_t dClickInterval, bool preventClick)
{
  this->m_doubleClickInterval = dClickInterval;
  this->m_preventClick = preventClick;
}

void ML2Input::setPullup(InputPullup::PullupType pullup)
{
  this->m_pullup = pullup;
  pinMode(this->m_pin, m_pullup == InputPullup::IntPullup ? INPUT_PULLUP : INPUT);
  reset();
}

void ML2Input::setBounceInterval(uint16_t bounceInterval)
{
  this->m_bounceInterval = bounceInterval;
  this->interval(bounceInterval);
}

bool ML2Input::pressed()
{
  return (isPullup() && fell()) || (!isPullup() && rose());
}

bool ML2Input::released()
{
  return (!isPullup() && fell()) || (isPullup() && rose());
}

bool ML2Input::down()
{
  return (!isPullup() && read()) || (isPullup() && !read());
}

inline bool ML2Input::up()
{
  return !down();
}

inline void ML2Input::queueEvent(int event)
{
  inputEM.queueEvent(event, new EventParam(this));
}

void ML2Input::reset()
{
  this->attach(m_pin);
  this->bState = up() ? ButtonState::Up : ButtonState::Down;
}

void ML2Input::check(uint32_t millisec)
{
  if (!millisec)
    millisec = millis();

  if (this->update())
    queueEvent(ButtonEvent::StateChanged);

  if (up() && (clickCount > 0) && (millisec - buttonPressTimeStamp) >= m_doubleClickInterval)
  {
    if (clickCount == 1 && m_preventClick)
      queueEvent(ButtonEvent::Click);

    clickCount = 0;
  }

  if (this->pressed())
  {
    buttonPressTimeStamp = millisec;
    bState = ButtonState::Down;
    queueEvent(ButtonEvent::Pressed);
  }

  if (this->released())
  {
    queueEvent(ButtonEvent::Released);

    if (m_holdInterval && ((millisec - buttonPressTimeStamp) >= m_holdInterval))
    {
      queueEvent(ButtonEvent::LongClick);
    }
    else
    {
      if (this->m_doubleClickInterval > 0)
      {
        if (++clickCount == 1 && !m_preventClick)
          queueEvent(ButtonEvent::Click);

        if (clickCount == 2)
          queueEvent(ButtonEvent::DoubleClick);
      }
      else
      {
        queueEvent(isHold ? ButtonEvent::LongClick : ButtonEvent::Click);
      }
    }

    isHold = false;
    bState = ButtonState::Up;
  }

  if (down())
  {
    if (!buttonPressTimeStamp)
      buttonPressTimeStamp = millisec;

    if (m_repeat && isHold && (millisec - lastRepeatTimeStamp) >= m_repeatInterval)
    {
      lastRepeatTimeStamp = millisec;
      queueEvent(ButtonEvent::Repeat);
    }

    if (!isHold && m_holdInterval && (millisec - buttonPressTimeStamp) >= m_holdInterval)
    {
      queueEvent(ButtonEvent::Hold);
      isHold = true;
      bState |= ButtonState::HoldState;

      if (m_repeat && (millisec - buttonPressTimeStamp) >= m_holdInterval)
      {
        lastRepeatTimeStamp = millisec;
        queueEvent(ButtonEvent::Repeat);
      }

    }
  }
}

InputList::InputList() : SimpleList<ML2Input * >()
{

}

bool InputList::addInput(ML2Input *input)
{
  if (!input || hasInput(input) || find(input->ID))
    return false;

  this->push_back(input);
  return true;
}

bool InputList::removeInput(ML2Input *input)
{
  if (input)
  {
    for (InputList::iterator itr = begin(); itr != end(); ++itr)
    {
      if ((*itr) == input)
      {
        erase(itr);
        delete (*itr);
        return true;
      }
    }
  }
  return false;
}

bool InputList::hasInput(ML2Input *input)
{
  if (input)
  {
    for (InputList::iterator itr = begin(); itr != end(); ++itr)
    {
      if ((*itr) == input)
        return true;
    }

  }
  return false;
}

ML2Input *InputList::find(const char *id)
{
  if (strlen(id))
  {
    for (InputList::iterator itr = begin(); itr != end(); ++itr)
    {
      if (strcmp((*itr)->ID, id) == 0)
        return (*itr);
    }
  }
  return 0;
}

void InputList::check()
{
  for (InputList::iterator itr = begin(); itr != end(); ++itr)
  {
    (*itr)->check();
  }
}

void InputList::clearInputs()
{
  for (InputList::iterator itr = this->begin(); itr != this->end(); ++itr)
    delete (*itr);
  this->clear();
}

bool InputList::addInputRule(const char *inputId, int addrRule) {
  ML2Input *input = find(inputId);
  if (!input)
    return false;
    
  input->rules.push_back(addrRule);
//  input->rules.sort(byOrder);
  return true;
}

