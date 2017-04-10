EventManager inputEM(EventManager::kNotInterruptSafe);
EventManager outputEM(EventManager::kNotInterruptSafe);
EventManager externalEM(EventManager::kNotInterruptSafe);

bool externalEventsEnabled = false;

void outputEventListener(int event, EventParam *param) {
  OutputEventParam *p = reinterpret_cast<OutputEventParam *>(param);
  ML2Output *output = static_cast<ML2Output *>(p->sender);

  if (!output || !outputList.hasOutput(output))
    return;

  output->action((OutputAction::Action)event, p->param, p->timeout);

#ifdef DEBUG_RULES
  const char *id = output->ID;
  switch (event) {
    case OutputAction::NoAction:
      Serialprint("Output %s NoAction\r\n", id);
      break;
    case OutputAction::On:
      Serialprint("Output %s On\r\n", id);
      break;
    case OutputAction::Off:
      Serialprint("Output %s Off\r\n", id);
      break;
    case OutputAction::Toggle:
      Serialprint("Output %s Toggle\r\n", id);
      break;
    case OutputAction::Value:
      Serialprint("Output %s Value=%d\r\n", id, p->param);
      break;
    case OutputAction::IncValue:
      Serialprint("Output %s IncValue=%d\r\n", id, p->param);
      break;
  }
#endif
}


void inputEventListener( int event, EventParam *param ) {
  ML2Input *input = reinterpret_cast<ML2Input *>(param->sender);

#ifdef DEBUG_RULES
  const char *id = input->ID;
  switch (event) {
    case ButtonEvent::Pressed:
      Serialprint("Button %s Pressed\r\n", id);
      break;
    case ButtonEvent::Released:
      Serialprint("Button %s Released\r\n", id);
      break;
    case ButtonEvent::Repeat:
      Serialprint("Button %s Repeat\r\n", id);
      break;
    case ButtonEvent::Hold:
      Serialprint("Button %s Hold\r\n", id);
      break;
    case ButtonEvent::Click:
      Serialprint("Button %s Click\r\n", id);
      break;
    case ButtonEvent::DoubleClick:
      Serialprint("Button %s DoubleClick\r\n", id);
      break;
    case ButtonEvent::LongClick:
      Serialprint("Button %s LongClick\r\n", id);
      break;
  }
#endif

  for (SimpleList<int>::iterator itr = input->rules.begin(); itr != input->rules.end(); ++itr)
  {
    ML2Rule *rule = new ML2Rule("");
    if (loadRuleEEPROM(rule, *itr, false)) {
      if (rule->processButtonEvent(event, input))
        if (rule->final)
        {
          delete rule;
          break;
        }
    }
    delete rule;
  }
}

EthernetClient client;

void externalEventListener( int event, EventParam *param ) {

  if (event == 0) // output event
  {
    ML2Output *output = reinterpret_cast<ML2Output *>(param->sender);

    saveOutputStateAndValue(output);

#ifdef DEBUG_RULES
    Serialprint("Status output %s State:%s Value:%d\r\n", output->ID, output->on() ? "On" : "Off", output->value());
#endif

    if (!client.connected())
      client.stop();

    if (client.connect(mdHost.c_str(), mdPort))
    {
      Streamprint(client, "GET /objects/?object=ThisComputer&op=m&m=setRelayState");
      Streamprint(client, "&id=%s", output->ID);
      Streamprint(client, "&on=%d", output->on());
      Streamprint(client, "&v=%d", output->value());
      Streamprint(client, " HTTP/1.0\r\n");
      Streamprint(client, "Host: %s\r\n", mdHost.c_str());
      if (mdAuth.length())
        Streamprint(client, "Authorization: Basic %s\r\n", mdAuth.c_str());
      Streamprint(client, "Connection: close\r\n\r\n");
      delay(10);
      while (client.available()) {
        client.read();
      }
      client.stop();
    }
  }
}

GenericCallable<void(int, EventParam*)> callableInputListener(&inputEventListener);
GenericCallable<void(int, EventParam*)> callableOutputListener(&outputEventListener);
GenericCallable<void(int, EventParam*)> callableExtrenalListener(&externalEventListener);

void setupEMs() {
  inputEM.addListener(ButtonEvent::StateChanged, &callableInputListener);
  inputEM.addListener(ButtonEvent::Pressed     , &callableInputListener);
  inputEM.addListener(ButtonEvent::Released    , &callableInputListener);
  inputEM.addListener(ButtonEvent::Repeat      , &callableInputListener);
  inputEM.addListener(ButtonEvent::Hold        , &callableInputListener);
  inputEM.addListener(ButtonEvent::LongClick   , &callableInputListener);
  inputEM.addListener(ButtonEvent::Click       , &callableInputListener);
  inputEM.addListener(ButtonEvent::DoubleClick , &callableInputListener);

  outputEM.addListener(OutputAction::NoAction, &callableOutputListener);
  outputEM.addListener(OutputAction::On,       &callableOutputListener);
  outputEM.addListener(OutputAction::Off,      &callableOutputListener);
  outputEM.addListener(OutputAction::Toggle,   &callableOutputListener);
  outputEM.addListener(OutputAction::Value,    &callableOutputListener);
  outputEM.addListener(OutputAction::IncValue, &callableOutputListener);


  externalEM.setDefaultListener(&callableExtrenalListener);
  externalEventsEnabled = true;
}

