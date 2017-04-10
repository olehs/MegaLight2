#include "ml2classes.h"

extern bool externalEventsEnabled;
extern EventManager externalEM;
extern OutputList outputList;

ML2Output::ML2Output(const String &id)
  : m_pin(0)
  , m_timeout(0)
  , m_dim_t0(0)
  , m_dim_t1(0)
  , m_dim_v0(0)
  , m_dim_v1(0)
  , m_pwm(false)
  , m_on(false)
  , m_invert(false)
  , m_saveState(OutputStateSave::None)
  , m_value(0)
{
  id.toCharArray(ID, ID_SIZE);
  setupPin();
}

void ML2Output::setPin(byte pin)
{
  m_pin = pin;
  setupPin();
}

void ML2Output::setPWM(bool on)
{
  m_pwm = on;
  setupPin();
}

void ML2Output::setValue(byte value, uint32_t timeout)
{
  if (timeout)
  {
    m_dim_v0 = m_value;
    m_dim_v1 = value;
    m_dim_t0 = millis();
    m_dim_t1 = m_dim_t0 + timeout;
    check();
  }
  else
  {
    m_value = value;
    m_dim_t1 = 0;
    updatePin(this->timeout());
  }
}

void ML2Output::setOn(uint32_t timeout)
{
  m_on = true;
  updatePin(timeout);
}

void ML2Output::setOff(uint32_t timeout)
{
  m_on = false;
  updatePin(timeout);
}

bool ML2Output::toggle(uint32_t timeout)
{
  on() ? setOff(timeout) : setOn(timeout);
}

void ML2Output::incValue(int val, uint32_t timeout)
{
  val += (int)m_value;

  if (val < 0)
    val = 0;
  else if (val > PWM_HIGH)
    val = PWM_HIGH;

  setValue(val, timeout);
}

bool ML2Output::action(OutputAction::Action action, int param, uint32_t timeout)
{
  switch (action) {
    case OutputAction::NoAction:
      updatePin();
      break;
    case OutputAction::On:
      setOn(timeout);
      break;
    case OutputAction::Off:
      setOff(timeout);
      break;
    case OutputAction::Toggle:
      toggle(timeout);
      break;
    case OutputAction::Value:
      setValue(param, timeout);
      break;
    case OutputAction::IncValue:
      incValue(param, timeout);
      break;
  }

  return value();
}

void ML2Output::check(uint32_t millisec)
{
  if (!millisec)
    millisec = millis();

  byte nv;
  if (m_dim_t1)
  {
    if (millisec >= m_dim_t1)
    {
      nv = m_dim_v1;
      m_dim_t1 = 0;
      emitState();
    }
    else
    {
      int dV = m_dim_v1 - m_dim_v0;
      int32_t dT = (m_dim_t1 - m_dim_t0);
      int32_t t = millisec - m_dim_t0;

      double v = t * dV / dT + m_dim_v0;
      if (v < 0)
        nv = 0;
      else if (v > PWM_HIGH)
        nv = PWM_HIGH;
      else
        nv = (byte)v;
    }
    if (nv != m_value)
    {
      m_value = nv;
      updatePin(timeout(), false);    }
  }

  if (m_timeout)
  {
    if (millisec > m_timeout)
      toggle();
  }
}

uint32_t ML2Output::timeout()
{
  if (!m_timeout)
    return 0;
  return max(0, m_timeout - millis());
}

OutputStateSave::Save ML2Output::saveState()
{
  return m_saveState;
}

void ML2Output::setSaveState(OutputStateSave::Save saveState)
{
  m_saveState = saveState;
}

bool ML2Output::invert()
{
  return m_invert;
}

void ML2Output::setInvert(bool inv)
{
  m_invert = inv;
}

void ML2Output::setupPin()
{
  if (m_pin)
  {
    pinMode(m_pin, OUTPUT);
  }
  updatePin();
}

void ML2Output::updatePin(uint32_t timeout, bool doEmit)
{
  if (timeout)
    m_timeout = millis() + timeout;
  else
    m_timeout = 0;

  if (m_pin)
  {
    if (m_pwm)
    {
      byte onVal  = m_invert ? PWM_HIGH - m_value : m_value;
      byte offVal = m_invert ? PWM_HIGH : 0;
      analogWrite(m_pin, m_on ? onVal : offVal);
    }
    else
    {
      int onState  = m_invert ? LOW : HIGH;
      int offState = m_invert ? HIGH : LOW;
      digitalWrite(m_pin, m_on ? onState : offState);
    }
  }

  if (doEmit)
    emitState();
}

void ML2Output::emitState()
{
  if (externalEventsEnabled)
    externalEM.queueEvent(0, new EventParam(this));
}

OutputList::OutputList() : SimpleList<ML2Output * >()
{
}

bool OutputList::addOutput(ML2Output *output)
{
  if (!output || hasOutput(output) || find(output->ID))
    return false;

  this->push_back(output);
  return true;
}

bool OutputList::removeOutput(ML2Output *output)
{
  if (output)
  {
    for (OutputList::iterator itr = begin(); itr != end(); ++itr)
    {
      if ((*itr) == output)
      {
        erase(itr);
        delete (*itr);
        return true;
      }
    }
  }
  return false;
}

bool OutputList::hasOutput(ML2Output *output)
{
  if (output)
  {
    for (OutputList::iterator itr = begin(); itr != end(); ++itr)
    {
      if ((*itr) == output)
        return true;
    }

  }
  return false;
}

ML2Output *OutputList::find(const char* id)
{
  if (strlen(id))
  {
    for (OutputList::iterator itr = begin(); itr != end(); ++itr)
    {
      if (strcmp((*itr)->ID, id) == 0)
        return (*itr);
    }
  }
  return 0;
}


void OutputList::check()
{
  for (OutputList::iterator itr = begin(); itr != end(); ++itr)
    (*itr)->check();
}

void OutputList::clearOutputs()
{
  for (OutputList::iterator itr = this->begin(); itr != this->end(); ++itr)
    delete (*itr);
  this->clear();
}




