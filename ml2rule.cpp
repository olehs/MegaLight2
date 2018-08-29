#include "ml2classes.h"

#include <SDConfigFile.h>

extern EventManager outputEM;
extern OutputList outputList;
extern InputList inputList;

uint32_t parseTime(const char* v) {
  String s = v;
  s.trim();
  if (!s.length())
    return 0;
  s.toUpperCase();

  uint32_t r = 0;
  byte i = 0;

  do {
    char c = s[i];
    if (!c) {
      break;
      
    } else if (c >= '0' && c <= '9') {
      ++i;
      
    } else {
      if (c == 'D') {
        r += strtoul(s.substring(0, i).c_str(), NULL, 10) * 24 * 60 * 60 * 1000;
      } else if (c == 'H') {
        r += strtoul(s.substring(0, i).c_str(), NULL, 10) * 60 * 60 * 1000;
      } else if (c == 'M') {
        r += strtoul(s.substring(0, i).c_str(), NULL, 10) * 60 * 1000;
      } else if (c == 'S') {
        r += strtoul(s.substring(0, i).c_str(), NULL, 10) * 1000;
      }
      s.remove(0, i + 1);
      i = 0;
    }
  } while (true);

  return r + strtoul(s.c_str(), NULL, 10);
}

int eval_token(char *expr)
{
  String s = expr;
  s.trim();
  if (!s.length())
    return 0;

  s.toUpperCase();

  char c = s[0];
  if ((c == 'R') || (c == 'V'))
  {
    bool needValue = (c == 'V');
    s.remove(0, 1);
    c = s[0];

    int i = 0;
    while (c && ExpressionEvaluator::istoken_char(c))
      c = s[++i];
    s.remove(i);

    ML2Output *output = outputList.find(s.c_str());
    if (output)
      return needValue ? output->value() : (output->on() ? 1 : 0);
  }
  else if (c == 'B')
  {
    s.remove(0, 1);
    ButtonState::State state;
    c = s[0];
    switch (c) {
      case 'U' :
        state = ButtonState::Up;
        s.remove(0, 1);
        c = s[0];
        break;
      case 'H' :
        state = ButtonState::HoldState;
        s.remove(0, 1);
        c = s[0];
        break;
      default:
        state = ButtonState::Down;
    }

    int i = 0;
    while (c && ExpressionEvaluator::istoken_char(c))
      c = s[++i];
    s.remove(i);

    ML2Input *input = inputList.find(s.c_str());
    if (input)
      return input->state() == state ? 1 : 0;
  }
  else
  {
    return s.toInt();
  }

  return 0;
}

GenericTokenEvaluator<int(char*)> tokenEvaluator(&eval_token);
ExpressionEvaluator evaluator(&tokenEvaluator);
bool evalRuleExpr(const String &expr)
{
  int r = evaluator.eval(expr.c_str());
  if (r == EVAL_FAILURE)
  {
    Serialprint("Error evaluating expression: %s", expr.c_str());
    return false;
  }
  return r != 0;
}

ML2Rule::ML2Rule(const String &id)
  : ID(id)
  , final(false)
{
  for (int i = 0; i < ButtonEvent::EventsCount; i++)
  {
    eventActions[i].action = OutputAction::Unassigned;
    eventActions[i].param = 0;
    eventActions[i].timeout = 0;
    eventActions[i].condition = "";
  }
}

ML2Rule::~ML2Rule()
{
  clearOutputs();
}


bool ML2Rule::addInput(const char *id)
{
  ML2Input *input = inputList.find(id);
  if (!input)
    return false;

  return inputlist.addInput(input);
}

bool ML2Rule::hasOutput(ML2Output *output)
{
  for (OutputList::iterator itr = outputlist.begin(); itr != outputlist.end(); ++itr)
  {
    if ((*itr) == output)
      return true;
  }
  return false;
}

bool ML2Rule::addOutput(const char *id)
{
  ML2Output *output = outputList.find(id);
  if (!output)
    return false;

  return outputlist.addOutput(output);
}

bool ML2Rule::removeOutput(const char *id)
{
  ML2Output *output = outputList.find(id);
  if (!output)
    return false;

  for (OutputList::iterator itr = outputlist.begin(); itr != outputlist.end(); ++itr)
  {
    if ((*itr) == output)
    {
      outputlist.erase(itr);
      return true;
    }
  }
  return false;
}

bool ML2Rule::hasOutput(const char *id)
{
  ML2Output *output = outputList.find(id);
  if (!output)
    return false;

  return hasOutput(output);
}

int ML2Rule::outputCount()
{
  return outputlist.size();
}

void ML2Rule::clearOutputs()
{
  outputlist.clear();
}

void ML2Rule::setAction(ButtonEvent::Type event, OutputAction::Action action, int param, uint32_t timeout)
{
  eventActions[event].action = action;
  eventActions[event].param = param;
  eventActions[event].timeout = timeout;
}

void ML2Rule::unsetAction(ButtonEvent::Type event)
{
  eventActions[event].action = OutputAction::Unassigned;
}

void ML2Rule::setCondition(ButtonEvent::Type event, const char *condition)
{
  eventActions[event].condition = condition;
}

bool ML2Rule::processButtonEvent(int event, ML2Input *input)
{
  OutputAction::Action action = eventActions[event].action;
  if (action == OutputAction::Unassigned)
    return false;

  if (eventActions[event].condition.length() && !evalRuleExpr(eventActions[event].condition))
    return false;

  for (OutputList::iterator itr = outputlist.begin(); itr != outputlist.end(); ++itr)
    outputEM.queueEvent(action, new OutputEventParam((*itr), eventActions[event].param, eventActions[event].timeout));

  return true;
}

ML2Rule *ML2Rule::fromFile(const String &path) {
  const uint8_t CONFIG_LINE_LENGTH = 127;

  SDConfigFile cfg;

  String fullPath = String(RULES_PATH) + path;
  if (!cfg.begin(fullPath.c_str(), CONFIG_LINE_LENGTH)) {
    Serialprint("Failed to open rule file: %s", path.c_str());
    return 0;
  }

  ML2Rule *rule = new ML2Rule(path);

  ButtonEvent::Type currEvent = ButtonEvent::EventsCount;

  // Read each setting from the file.
  while (cfg.readNextSetting()) {

    if (cfg.nameIs("final")) {
      rule->final = cfg.getBooleanValue();

    } else if (cfg.nameIs("input")) {
      String inputId = cfg.getValue();
      inputId.toUpperCase();
      if (!rule->addInput(inputId.c_str()))
        Serialprint("Could not add input %s to rule %s\r\n", inputId.c_str(), path.c_str());

    } else if (cfg.nameIs("output")) {
      String outputId = cfg.getValue();
      outputId.toUpperCase();

      if (!rule->addOutput(outputId.c_str()))
        Serialprint("Could not add output %s to rule %s\r\n", outputId.c_str(), path.c_str());

    } else if (cfg.nameIs("event")) {
      String pu = cfg.getValue();
      if (pu == "change")
        currEvent = ButtonEvent::StateChanged;
      else if (pu == "press")
        currEvent = ButtonEvent::Pressed;
      else if (pu == "release")
        currEvent = ButtonEvent::Released;
      else if (pu == "repeat")
        currEvent = ButtonEvent::Repeat;
      else if (pu == "hold")
        currEvent = ButtonEvent::Hold;
      else if (pu == "lclick")
        currEvent = ButtonEvent::LongClick;
      else if (pu == "click")
        currEvent = ButtonEvent::Click;
      else if (pu == "dclick")
        currEvent = ButtonEvent::DoubleClick;

    } else if (cfg.nameIs("action") && (currEvent != ButtonEvent::EventsCount)) {
      String pu = cfg.getValue();
      if (pu == "no")
        rule->eventAction(currEvent).action = OutputAction::NoAction;
      else if (pu == "on")
        rule->eventAction(currEvent).action = OutputAction::On;
      else if (pu == "off")
        rule->eventAction(currEvent).action = OutputAction::Off;
      else if (pu == "toggle")
        rule->eventAction(currEvent).action = OutputAction::Toggle;
      else if (pu == "value")
        rule->eventAction(currEvent).action = OutputAction::Value;
      else if (pu == "incvalue")
        rule->eventAction(currEvent).action = OutputAction::IncValue;

    } else if (cfg.nameIs("param") && (currEvent != ButtonEvent::EventsCount)) {
      rule->eventAction(currEvent).param = cfg.getIntValue();

    } else if (cfg.nameIs("timeout") && (currEvent != ButtonEvent::EventsCount)) {
      rule->eventAction(currEvent).timeout = parseTime(cfg.getValue());

    } else if (cfg.nameIs("condition") && (currEvent != ButtonEvent::EventsCount)) {
      rule->eventAction(currEvent).condition = cfg.getValue();

    }
  }

  // clean up
  cfg.end();

  return rule;
}


