#include <avr/wdt.h>

Scheduler runner;

Task t1(1, TASK_FOREVER, &buttonLoop, &runner);
Task t2(1, TASK_FOREVER, &relayLoop, &runner);
Task t3(1, TASK_FOREVER, &webLoop, &runner);
Task t4(1, TASK_FOREVER, &externalLoop, &runner);

void buttonLoop() {
  inputList.check();
  outputEM.processAllEvents();
}

void relayLoop() {
  inputEM.processAllEvents();
  outputList.check();
}

#define WBSIZE 1024
void webLoop()
{
  char webBuffer[WBSIZE];
  int buflen = WBSIZE;
  webserver.processConnection(webBuffer, &buflen);
}

void externalLoop()
{
  externalEM.processEvent();
}


bool loadConfig() {
  const uint8_t CONFIG_LINE_LENGTH = 127;
  // The open configuration file.
  SDConfigFile cfg;

  // Open the configuration file.
  if (!cfg.begin(CONFIG_FILE, CONFIG_LINE_LENGTH)) {
    Serialprint("Failed to open configuration file: %s\r\n", CONFIG_FILE);
    return false;
  }

  // Read each setting from the file.
  while (cfg.readNextSetting()) {

    if (cfg.nameIs("mac")) {
      parseBytes(cfg.getValue(), '-', mac, 6, 16);
      Serial.print("MAC="); Serial.println(cfg.getValue());

    } else if (cfg.nameIs("ip")) {
      parseBytes(cfg.getValue(), '.', ip, 4, 10);

    } else if (cfg.nameIs("mdHost")) {
      mdHost = cfg.getValue();

    } else if (cfg.nameIs("mdPort")) {
      mdPort = cfg.getIntValue();

    } else if (cfg.nameIs("mdAuth")) {
      mdAuth = cfg.getValue();
    }
  }

  // clean up
  cfg.end();
  return true;
}

bool setupSD() {
  if (!SD.begin(4)) {
    Serialprint("SD unavailable. Trying to load config from EEPROM.\r\n");
    return false;
  }
  Serialprint("SD initialization done.\r\n");
  return true;
}


void setupTasks()
{
  t1.enable();
  t2.enable();
  t3.enable();
  t4.enable();

  runner.init();

  runner.addTask(t1);
  runner.addTask(t2);
  runner.addTask(t3);
  runner.addTask(t4);
}

int setupInputsSD() {
  const uint8_t CONFIG_LINE_LENGTH = 127;

  String configDir = F("/INPUTS");
  File dir = SD.open(configDir);
  if (!dir.isDirectory())
    return 0;

  inputList.clearInputs();
  storageHeader.cntInputs = 0;
  int sz = 0;

  SDConfigFile cfg;

  while (true) {
    File inp = dir.openNextFile();
    if (!inp)
      break;

    if (inp.isDirectory())
    {
      inp.close();
      continue;
    }

    const char *id = inp.name();
    inp.close();

    ML2Input *b = new ML2Input(id);

    // The open configuration file.
    if (!cfg.begin(String(configDir + "/" + String(id)).c_str(), CONFIG_LINE_LENGTH)) {
      Serialprint("Failed to open input file: %s\r\n", id);
      delete b;
      cfg.end();
      continue;
    }


    // Read each setting from the file.
    while (cfg.readNextSetting()) {

      if (cfg.nameIs("pin")) {
        b->setPin(cfg.getIntValue());

      } else if (cfg.nameIs("pullup")) {
        const char *pu = cfg.getValue();
        if (!strcmp(pu, "intup"))
          b->setPullup(InputPullup::IntPullup);
        else if (!strcmp(pu, "extup"))
          b->setPullup(InputPullup::ExtPullup);
        else if (!strcmp(pu, "extdown"))
          b->setPullup(InputPullup::PullDown);

      } else if (cfg.nameIs("bounceint")) {
        b->setBounceInterval(cfg.getIntValue());

      } else if (cfg.nameIs("holdint")) {
        b->setHoldInterval(cfg.getIntValue());

      } else if (cfg.nameIs("repeat")) {
        b->setRepeat(cfg.getBooleanValue());

      } else if (cfg.nameIs("repeatint")) {
        b->setRepeatInterval(cfg.getIntValue());

      } else if (cfg.nameIs("dclickint")) {
        b->setDoubleClickInterval(cfg.getIntValue());

      } else if (cfg.nameIs("prevclick")) {
        b->setPreventClick(cfg.getBooleanValue());

      }
    }

    // clean up
    cfg.end();

    if (!inputList.addInput(b))
    {
      Serialprint("Failed to add input %s\r\n", id);
      delete b;
    }
    else
    {
      Serialprint("Added input %s on pin %d\r\n", id, b->pin());
      sz += saveInputEEPROM(b, storageHeader.addrInputs + sz);
      storageHeader.cntInputs++;
    }
  }

  dir.close();

  return sz;
}

int setupOutputsSD() {
  const uint8_t CONFIG_LINE_LENGTH = 127;

  String configDir = F("/OUTPUTS");
  File dir = SD.open(configDir);
  if (!dir.isDirectory())
    return 0;

  outputList.clearOutputs();
  storageHeader.cntOutputs = 0;
  int sz = 0;

  SDConfigFile cfg;

  while (true) {
    File inp = dir.openNextFile();
    if (!inp)
      break;

    if (inp.isDirectory())
    {
      inp.close();
      continue;
    }

    const char *id = inp.name();
    inp.close();

    ML2Output *b = new ML2Output(id);

    // The open configuration file.
    if (!cfg.begin(String(configDir + "/" + String(id)).c_str(), CONFIG_LINE_LENGTH)) {
      Serialprint("Failed to open output file: %s\r\n", id);
      delete b;
      cfg.end();
      continue;
    }


    // Read each setting from the file.
    while (cfg.readNextSetting()) {

      if (cfg.nameIs("pin")) {
        b->setPin(cfg.getIntValue());

      } else if (cfg.nameIs("pwm")) {
        b->setPWM(cfg.getBooleanValue());

      } else if (cfg.nameIs("invert")) {
        b->setInvert(cfg.getBooleanValue());

      } else if (cfg.nameIs("noreport")) {
        b->setNoreport(cfg.getBooleanValue());

      } else if (cfg.nameIs("on")) {
        cfg.getBooleanValue() ? b->setOn() : b->setOff();

      } else if (cfg.nameIs("value")) {
        b->setValue(cfg.getIntValue());

      } else if (cfg.nameIs("save")) {
        const char *pu = cfg.getValue();
        if (!strcmp(pu, "state"))
          b->setSaveState(OutputStateSave::State);
        else if (!strcmp(pu, "value"))
          b->setSaveState(OutputStateSave::Value);
        else if (!strcmp(pu, "both"))
          b->setSaveState(OutputStateSave::StateAndValue);

      }
    }

    // clean up
    cfg.end();

    if (!outputList.addOutput(b))
    {
      Serialprint("Failed to add output %s\r\n", id);
      delete b;
    }
    else
    {
      Serialprint("Added output %s on pin %d\r\n", id, b->pin());
      sz += saveOutputEEPROM(b, storageHeader.addrOutputs + sz);
      storageHeader.cntOutputs++;
    }
  }

  dir.close();
  return sz;
}


int loadRulesFromFile(File &dir, String path) {
  int sz = 0;
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }

    String npath = path + String("/") + entry.name();
    if (entry.isDirectory()) {
      sz += loadRulesFromFile(entry, npath);
      entry.close();
      continue;
    }
    entry.close();

    ML2Rule *rule = ML2Rule::fromFile(npath);
    if (rule) {
      int s = saveRuleEEPROM(rule, storageHeader.addrRules);
      sz += s;
      storageHeader.addrRules += s;
      storageHeader.cntRules++;
      delete rule;
      Serialprint("Loaded rule: %s\r\n", npath.c_str());
    }
  }
  return sz;
}

int setupRulesSD() {
  storageHeader.cntRules = 0;

  File root = SD.open(RULES_PATH);
  if (root) {
    return loadRulesFromFile(root, "");
  }
}

bool loadAllFromEEPROM() {
  int addr = CONFIG_START;
  int sz = loadConfigEEPROM(addr);
  if (!sz)
    return false;

  addr += sz;
  sz = loadHeaderEEPROM(addr);
  addr += sz;

  for (int i = 0; i < storageHeader.cntInputs; i++) {
    ML2Input *input = new ML2Input("");
    sz = loadInputEEPROM(input, addr);
    if (inputList.addInput(input))
      Serialprint("Added input %s on pin %d\r\n", input->ID, input->pin());

    addr += sz;
  }

  for (int i = 0; i < storageHeader.cntOutputs; i++) {
    ML2Output *output = new ML2Output("");
    sz = loadOutputEEPROM(output, addr);
    if (outputList.addOutput(output))
      Serialprint("Added output %s on pin %d\r\n", output->ID, output->pin());

    addr += sz;
  }

  for (int i = 0; i < storageHeader.cntRules; i++) {
    ML2Rule *rule = new ML2Rule("");
    sz = loadRuleEEPROM(rule, addr, true);
    Serialprint("Added rule %s\r\n", rule->ID.c_str());
    addr += sz;
    delete rule;
  }

  return true;
}

int setupConfigSD() {
  const uint8_t CONFIG_LINE_LENGTH = 127;
  SDConfigFile cfg;

  if (!cfg.begin(CONFIG_FILE, CONFIG_LINE_LENGTH)) {
    Serialprint("Failed to open configuration file: %s\r\n", CONFIG_FILE);
    return 0;
  }

  while (cfg.readNextSetting()) {
    if (cfg.nameIs("mac")) {
      parseBytes(cfg.getValue(), '-', mac, 6, 16);
      Serial.print("MAC="); Serial.println(cfg.getValue());

    } else if (cfg.nameIs("ip")) {
      parseBytes(cfg.getValue(), '.', ip, 4, 10);

    } else if (cfg.nameIs("mdHost")) {
      mdHost = cfg.getValue();

    } else if (cfg.nameIs("mdPort")) {
      mdPort = cfg.getIntValue();

    } else if (cfg.nameIs("mdAuth")) {
      mdAuth = cfg.getValue();
    }
  }

  // clean up
  cfg.end();

  return saveConfigEEPROM(CONFIG_START, false);
}

void saveAllToEEPROM() {
  int sz = setupConfigSD();
  if (!sz) {
    Serialprint("Failed to store config\r\n");
    return;
  }

  int addr = CONFIG_START + sz;
  int addrHeader = addr;
  addr += sizeof(storageHeader);

  storageHeader.addrInputs = addr;
  sz = setupInputsSD();
  addr += sz;
  Serialprint("Stored %d inputs (%d bytes)\r\n\r\n", storageHeader.cntInputs, sz);

  storageHeader.addrOutputs = addr;
  sz = setupOutputsSD();
  addr += sz;
  Serialprint("Stored %d outputs (%d bytes)\r\n\r\n", storageHeader.cntOutputs, sz);

  storageHeader.addrRules = addr;
  sz = setupRulesSD();
  addr += sz;
  Serialprint("Stored %d rules (%d bytes)\r\n\r\n", storageHeader.cntRules, sz);

  saveHeaderEEPROM(addrHeader);
  saveConfigEEPROM(CONFIG_START, true);

  Serialprint("Stored config to EEPROM (%d bytes)\r\n\r\n", addr - CONFIG_START);
}


int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void setup() {
  wdt_disable();

  Serial.begin(115200);
  Serialprint("Starting...\r\n");

  if (setupSD()) {
    saveAllToEEPROM();
  } else {
    loadAllFromEEPROM();
  }

  setupWeb();
  setupEMs();
  setupTasks();

  Serialprint("Started (free RAM: %d)\r\n", freeRam());

  wdt_enable(WDTO_4S);
}

void loop() {
  wdt_reset();
  runner.execute();
}

void reset() {
  wdt_enable(WDTO_1S);
  while (1);
}

