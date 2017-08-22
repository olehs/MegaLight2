#include <avr/eeprom.h>

#define CONFIG_VERSION "ML2"
#define CONFIG_START 0

// Input flags
#define ML2I_FLAG_INTUP        0x01
#define ML2I_FLAG_EXTDOWN      0x02
#define ML2I_FLAG_EXTUP        ( ML2I_FLAG_INTUP | ML2I_FLAG_EXTDOWN )
#define ML2I_FLAG_PULLUP       ( ML2I_FLAG_INTUP | ML2I_FLAG_EXTDOWN | ML2I_FLAG_EXTUP )
#define ML2I_FLAG_REPEAT       0x04
#define ML2I_FLAG_PREVENTCLICK 0x08

// Output flags
#define ML2O_FLAG_ON           0x01
#define ML2O_FLAG_INVERT       0x02
#define ML2O_FLAG_PWM          0x04
#define ML2O_FLAG_SAVE_STATE   0x08
#define ML2O_FLAG_SAVE_VALUE   0x10
#define ML2O_FLAG_NO_REPORT    0x20
#define ML2O_FLAG_SAVE_BOTH    ( ML2O_FLAG_SAVE_STATE | ML2O_FLAG_SAVE_VALUE )
#define ML2O_FLAG_SAVE         ( ML2O_FLAG_SAVE_STATE | ML2O_FLAG_SAVE_VALUE | ML2O_FLAG_SAVE_BOTH )

// Rules flags
#define ML2R_FLAG_FINAL        0x01


struct ML2StorageConfig {
  byte mac[6];
  byte ip[4];
  uint16_t mdPort;
  byte szMdHost;
  byte szMdAuth;
  char version[4];
} storageConfig;

struct ML2StorageHeader {
  int addrInputs;
  byte cntInputs;

  int addrOutputs;
  byte cntOutputs;

  int addrRules;
  byte cntRules;
} storageHeader;

struct ML2StoreInput {
  byte pin;
  byte flags;
  uint16_t bi;
  uint16_t hi;
  uint16_t ri;
  uint16_t di;
  byte szID;
} storageInput;

struct ML2StoreOutput {
  byte pin;
  byte flags;
  byte value;
  byte szID;
} storageOutput;

struct ML2StoreRule {
  byte flags;
  byte cntInputs;
  byte cntOutputs;
  byte cntEventActions;
  byte szID;
} storageRule;

struct ML2StoreEventAction {
  ButtonEvent::Type event;
  OutputAction::Action action;
  int param;
  uint32_t timeout;
  byte szCondition;
} storageEventAction;

int loadConfigEEPROM(int addr) {
  eeprom_read_block((void*)&storageConfig, (const void*)addr, sizeof(storageConfig));
  if (!(storageConfig.version[0] == CONFIG_VERSION[0] &&
        storageConfig.version[1] == CONFIG_VERSION[1] &&
        storageConfig.version[2] == CONFIG_VERSION[2] &&
        storageConfig.version[3] == CONFIG_VERSION[3])) {
    Serialprint("Config version mismatch %s <=> %s\r\n", storageConfig.version, CONFIG_VERSION);
    return 0;
  }

  memcpy(mac, storageConfig.mac, sizeof(storageConfig.mac));
  memcpy(ip, storageConfig.ip, sizeof(storageConfig.ip));

  Serialprint("Confgured IP: %d.%d.%d.%d\r\n", ip[0], ip[1], ip[2], ip[3]);

  int sz = sizeof(storageConfig);

  if (storageConfig.szMdHost) {
    char id[storageConfig.szMdHost + 1];
    memset(id, 0, sizeof(id));
    eeprom_read_block((void*)&id, (const void*)(addr + sz), storageConfig.szMdHost);
    id[storageConfig.szMdHost] = 0;
    mdHost = id;
    Serialprint("MJD Host %s\r\n", mdHost.c_str());
    sz += storageConfig.szMdHost;
  }

  if (storageConfig.szMdAuth) {
    char id[storageConfig.szMdAuth + 1];
    memset(id, 0, sizeof(id));
    eeprom_read_block((void*)&id, (const void*)(addr + sz), storageConfig.szMdAuth);
    id[storageConfig.szMdAuth] = 0;
    mdAuth = id;
    sz += storageConfig.szMdAuth;
  }

  return sz;
}

int saveConfigEEPROM(int addr, bool saveVersion) {
  storageConfig.version[0] = saveVersion ? CONFIG_VERSION[0] : 0;
  storageConfig.version[1] = saveVersion ? CONFIG_VERSION[1] : 0;
  storageConfig.version[2] = saveVersion ? CONFIG_VERSION[2] : 0;
  storageConfig.version[3] = saveVersion ? CONFIG_VERSION[3] : 0;

  memcpy(storageConfig.mac, mac, sizeof(mac));
  memcpy(storageConfig.ip, ip, sizeof(ip));
  storageConfig.mdPort = mdPort;
  storageConfig.szMdHost = mdHost.length();
  storageConfig.szMdAuth = mdAuth.length();

  eeprom_update_block((const void*)&storageConfig, (void*)addr, sizeof(storageConfig));

  int sz = sizeof(storageConfig);

  if (storageConfig.szMdHost) {
    eeprom_update_block((const void*)mdHost.c_str(), (void*)(addr + sz), storageConfig.szMdHost);
    sz += storageConfig.szMdHost;
  }

  if (storageConfig.szMdAuth) {
    eeprom_update_block((const void*)mdAuth.c_str(), (void*)(addr + sz), storageConfig.szMdAuth);
    sz += storageConfig.szMdAuth;
  }

  return sz;
}

int loadHeaderEEPROM(int addr) {
  eeprom_read_block((void*)&storageHeader, (const void*)addr, sizeof(storageHeader));
  return sizeof(storageHeader);
}

int saveHeaderEEPROM(int addr) {
  eeprom_update_block((const void*)&storageHeader, (void*)addr, sizeof(storageHeader));
  return sizeof(storageHeader);
}

int loadInputEEPROM(ML2Input *input, int addr) {
  eeprom_read_block((void*)&storageInput, (const void*)addr, sizeof(storageInput));

  input->setPin(storageInput.pin);
  switch (storageInput.flags & ML2I_FLAG_PULLUP) {
    case ML2I_FLAG_INTUP:
      input->setPullup(InputPullup::IntPullup);
      break;
    case ML2I_FLAG_EXTUP:
      input->setPullup(InputPullup::ExtPullup);
      break;
    case ML2I_FLAG_EXTDOWN:
      input->setPullup(InputPullup::PullDown);
      break;
    default:
      break;
  }
  input->setRepeat(storageInput.flags & ML2I_FLAG_REPEAT);
  input->setPreventClick(storageInput.flags & ML2I_FLAG_PREVENTCLICK);
  input->setBounceInterval(storageInput.bi);
  input->setHoldInterval(storageInput.hi);
  input->setRepeatInterval(storageInput.ri);
  input->setDoubleClickInterval(storageInput.di);

  memset(input->ID, 0, sizeof(input->ID));
  if (storageInput.szID)
    eeprom_read_block((void*)&input->ID, (const void*)(addr + sizeof(storageInput)), storageInput.szID);

  return sizeof(storageInput) + storageInput.szID;
}

int saveInputEEPROM(ML2Input *input, int addr) {
  storageInput.pin = input->pin();
  storageInput.flags = 0;
  switch (input->pullup()) {
    case InputPullup::IntPullup:
      storageInput.flags |= ML2I_FLAG_INTUP;
      break;
    case InputPullup::ExtPullup:
      storageInput.flags |= ML2I_FLAG_EXTUP;
      break;
    case InputPullup::PullDown:
      storageInput.flags |= ML2I_FLAG_EXTDOWN;
      break;
    default:
      break;
  }
  if (input->repeat())
    storageInput.flags |= ML2I_FLAG_REPEAT;
  if (input->preventClick())
    storageInput.flags |= ML2I_FLAG_PREVENTCLICK;

  storageInput.bi = input->bounceInterval();
  storageInput.hi = input->holdInterval();
  storageInput.ri = input->repeatInterval();
  storageInput.di = input->doubleClickInterval();
  storageInput.szID = strlen(input->ID);

  int sz = sizeof(storageInput);
  if (storageInput.szID) {
    eeprom_update_block((const void*)input->ID, (void*)(addr + sz), storageInput.szID);
    sz += storageInput.szID;
  }

  eeprom_update_block((const void*)&storageInput, (void*)addr, sizeof(storageInput));

  Serialprint("Saved input %s at %d (%d bytes)\r\n", input->ID, addr, sz);
  return sz;
}

int loadOutputEEPROM(ML2Output *output, int addr) {
  eeprom_read_block((void*)&storageOutput, (const void*)addr, sizeof(storageOutput));

  output->setPin(storageOutput.pin);
  output->setPWM(storageOutput.flags & ML2O_FLAG_PWM);
  output->setInvert(storageOutput.flags & ML2O_FLAG_INVERT);
  output->setNoreport(storageOutput.flags & ML2O_FLAG_NO_REPORT);
  switch (storageOutput.flags & ML2O_FLAG_SAVE) {
    case ML2O_FLAG_SAVE_STATE:
      output->setSaveState(OutputStateSave::State);
      break;
    case ML2O_FLAG_SAVE_VALUE:
      output->setSaveState(OutputStateSave::Value);
      break;
    case ML2O_FLAG_SAVE_BOTH:
      output->setSaveState(OutputStateSave::StateAndValue);
      break;
    default:
      output->setSaveState(OutputStateSave::None);
      break;
  }
  output->setValue(storageOutput.value);
  storageOutput.flags & ML2O_FLAG_ON ? output->setOn() : output->setOff();

  memset(output->ID, 0, ID_SIZE);
  if (storageOutput.szID) {
    eeprom_read_block((void*)&output->ID, (const void*)(addr + sizeof(storageOutput)), storageOutput.szID);
  }

  output->storeAddress = addr;

  return sizeof(storageOutput) + storageOutput.szID;
}

int saveOutputEEPROM(ML2Output *output, int addr) {
  storageOutput.pin = output->pin();
  storageOutput.value = output->value();

  storageOutput.flags = 0;
  if (output->invert())
    storageOutput.flags |= ML2O_FLAG_INVERT;
  if (output->pwm())
    storageOutput.flags |= ML2O_FLAG_PWM;
  if (output->noreport())
    storageOutput.flags |= ML2O_FLAG_NO_REPORT;

  switch (output->saveState()) {
    case OutputStateSave::State:
      storageOutput.flags |= ML2O_FLAG_SAVE_STATE;
      break;
    case OutputStateSave::Value:
      storageOutput.flags |= ML2O_FLAG_SAVE_VALUE;
      break;
    case OutputStateSave::StateAndValue:
      storageOutput.flags |= ML2O_FLAG_SAVE_BOTH;
      break;
    default:
      break;
  }

  if (output->on())
    storageOutput.flags |= ML2O_FLAG_ON;

  storageOutput.szID = strlen(output->ID);

  int sz = sizeof(storageOutput);
  if (storageOutput.szID) {
    eeprom_update_block((const void*)output->ID, (void*)(addr + sz), storageOutput.szID);
    sz += storageOutput.szID;
  }

  eeprom_update_block((const void*)&storageOutput, (void*)addr, sizeof(storageOutput));
  output->storeAddress = addr;

  Serialprint("Saved output %s at %d (%d bytes)\r\n", output->ID, addr, sz);
  return sz;
}

int loadRuleEEPROM(ML2Rule *rule, int addr, bool regInputs) {
  eeprom_read_block((void*)&storageRule, (const void*)addr, sizeof(storageRule));

  int sz = sizeof(storageRule);
  if (storageRule.szID) {
    char id[storageRule.szID + 1];
    memset(id, 0, sizeof(id));
    eeprom_read_block((void*)&id, (const void*)(addr + sz), storageRule.szID);
    id[storageRule.szID] = 0;
    rule->ID = id;
    sz += storageRule.szID;
  }

  rule->final = storageRule.flags & ML2R_FLAG_FINAL;

  for (int i = 0; i < storageRule.cntInputs; i++) {
    byte pos;
    eeprom_read_block((void*)&pos, (const void*)(addr + sz), sizeof(pos));
    ML2Input *input = inputList.at(pos);
    if (regInputs) {
      inputList.addInputRule(input->ID, addr);
      rule->addInput(input->ID);
    }
    sz += sizeof(pos);
  }

  for (int i = 0; i < storageRule.cntOutputs; i++) {
    byte pos;
    eeprom_read_block((void*)&pos, (const void*)(addr + sz), sizeof(pos));
    ML2Output *output = outputList.at(pos);
    rule->addOutput(output->ID);
    sz += sizeof(pos);
  }

  for (int i = 0; i < storageRule.cntEventActions; i++) {
    eeprom_read_block((void*)&storageEventAction, (const void*)(addr + sz), sizeof(storageEventAction));
    rule->setAction(storageEventAction.event, storageEventAction.action, storageEventAction.param, storageEventAction.timeout);
    sz += sizeof(storageEventAction);

    if (storageEventAction.szCondition) {
      char cond[storageEventAction.szCondition];
      memset(cond, 0, sizeof(cond));
      eeprom_read_block((void*)&cond, (const void*)(addr + sz), storageEventAction.szCondition);
      cond[storageEventAction.szCondition] = 0;
      rule->setCondition(storageEventAction.event, cond);
      sz += storageEventAction.szCondition;
    }
  }
  return sz;
}

int saveRuleEEPROM(ML2Rule *rule, int addr) {
  int sz = sizeof(storageRule);

  storageRule.szID = rule->ID.length();
  if (storageRule.szID) {
    eeprom_update_block((const void*)rule->ID.c_str(), (void*)(addr + sz), storageRule.szID);
    sz += storageRule.szID;
  }

  storageRule.flags = 0;
  if (rule->final)
    storageRule.flags |= ML2R_FLAG_FINAL;

  storageRule.cntInputs = 0;
  for (InputList::iterator itr = rule->inputs()->begin(); itr != rule->inputs()->end(); ++itr) {
    int8_t pos = inputList.indexOf((*itr));
    if (pos == -1)
      continue;

    eeprom_update_block((const void*)&pos, (void*)(addr + sz), sizeof(pos));

    inputList.addInputRule((*itr)->ID, addr);

    storageRule.cntInputs++;
    sz += sizeof(pos);
  }

  storageRule.cntOutputs = 0;
  for (OutputList::iterator itr = rule->outputs()->begin(); itr != rule->outputs()->end(); ++itr) {
    int8_t pos = outputList.indexOf((*itr));
    if (pos == -1)
      continue;

    eeprom_update_block((const void*)&pos, (void*)(addr + sz), sizeof(pos));
    storageRule.cntOutputs++;
    sz += sizeof(pos);
  }

  storageRule.cntEventActions = 0;
  for (int i = 0; i < ButtonEvent::EventsCount; i++)
  {
    ML2Rule::EventAction &ea = rule->eventAction((ButtonEvent::Type)i);
    if (ea.action == OutputAction::Unassigned)
      continue;

    storageEventAction.event = (ButtonEvent::Type)i;
    storageEventAction.action = ea.action;
    storageEventAction.param = ea.param;
    storageEventAction.timeout = ea.timeout;
    storageEventAction.szCondition = ea.condition.length();

    eeprom_update_block((const void*)&storageEventAction, (void*)(addr + sz), sizeof(storageEventAction));
    storageRule.cntEventActions++;
    sz += sizeof(storageEventAction);

    if (storageEventAction.szCondition) {
      eeprom_update_block((const void*)ea.condition.c_str(), (void*)(addr + sz), storageEventAction.szCondition);
      sz += storageEventAction.szCondition;
    }
  }

  eeprom_update_block((const void*)&storageRule, (void*)addr, sizeof(storageRule));

  Serialprint("Saved rule %s at %d (%d bytes)\r\n", rule->ID.c_str(), addr, sz);
  return sz;
}

void saveOutputStateAndValue(ML2Output *output) {
  if (output->saveState() == OutputStateSave::None)
    return;

  bool saveState = (output->saveState() == OutputStateSave::State) || (output->saveState() == OutputStateSave::StateAndValue);
  bool saveValue = (output->saveState() == OutputStateSave::Value) || (output->saveState() == OutputStateSave::StateAndValue);

  if (saveState) {
    byte flags;
    int addr = output->storeAddress + offsetof(ML2StoreOutput, flags);
    eeprom_read_block((void *)&flags, (const void *)addr, sizeof(flags));
    output->on() ? flags |= ML2O_FLAG_ON : flags &= ~ML2O_FLAG_ON;
    eeprom_update_block((const void *)&flags, (void *)addr, sizeof(flags));
  }

  if (saveValue) {
    int addr = output->storeAddress + offsetof(ML2StoreOutput, value);
    byte value = output->value();
    eeprom_update_block((const void *)&value, (void *)addr, sizeof(storageOutput.value));
  }

}

