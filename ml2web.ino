#include <utility/w5100.h>

#define TCP_RETRANSMISSION_TIME 150
#define ITEMS_PER_PAGE 10

#define NAMELEN 16
#define VALUELEN 128


#define PREFIX ""

WebServer webserver(PREFIX, 80);

const char CONFIG_FILE[] = "config.txt";

void parseBytes(const char* str, char sep, byte* bytes, int maxBytes, int base) {
  for (int i = 0; i < maxBytes; i++) {
    bytes[i] = strtoul(str, NULL, base);  // Convert byte
    str = strchr(str, sep);               // Find next separator
    if (str == NULL || *str == '\0') {
      break;                            // No more separators, exit
    }
    str++;                                // Point to next character after separator
  }
}

void defaultCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  server.httpSuccess();
}

void stateCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  server.httpSuccess("text/plain");

  if (type != WebServer::GET)
  {
    if (type != WebServer::HEAD)
      server.httpFail();
    return;
  }

  URLPARAM_RESULT rc;
  char name[NAMELEN];
  char value[VALUELEN];
  char c[NAMELEN];
  char id[ID_SIZE];
  int state = -1;
  int on = -1;
  int inc = 0;
  uint32_t timeout = 0;

  if (!strlen(url_tail))
    return;

  while (strlen(url_tail))
  {
    rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
    if (rc == URLPARAM_OK)
    {
      if (name[0] == 'c')
        strcpy(c, value);
      else if (name[0] == 'n')
        strcpy(id , value);
      else if (name[0] == 'o')
        on = atoi(value);
      else if (name[0] == 'v')
        state = strtoul(value, NULL, 10);
      else if (name[0] == 'i')
        inc = atoi(value);
      else if (name[0] == 't')
        timeout = strtoul(value, NULL, 10);
    }
  }

  if (!strlen(id))
    return;

  if (strcmp(c, "get") == 0)
  {
    ML2Output *output = outputList.find(id);
    if (!output)
    {
      server.httpNoContent();
      return;
    }

    server.print(output->on());
    server.print(";");
    server.print(output->value());
  }
  else if (strcmp(c, "set") == 0)
  {
    ML2Output *output = outputList.find(id);
    if (!output)
    {
      server.httpNoContent();
      return;
    }

    if (state >= 0 )
      outputEM.queueEvent(OutputAction::Value, new OutputEventParam(output, state, timeout));
    if (inc != 0 )
      outputEM.queueEvent(OutputAction::IncValue, new OutputEventParam(output, inc, timeout));
    if (on >= 0 )
      outputEM.queueEvent(on ? OutputAction::On : OutputAction::Off, new OutputEventParam(output, on, timeout));
  }
  else if (strcmp(c, "button") == 0)
  {
    ML2Input *button = inputList.find(id);
    if (!button)
    {
      server.httpNoContent();
      return;
    }

    server.print(button->state() & ButtonState::Down);
  }
}

void setupWeb()
{
  if (!ip[0] && !ip[1] && !ip[2] && !ip[3])
    Ethernet.begin(mac);
  else
    Ethernet.begin(mac, ip);

  W5100.setRetransmissionTime(TCP_RETRANSMISSION_TIME);
  W5100.setRetransmissionCount(3);

  webserver.begin();

  Serialprint("Server started at ");
  Serial.println(Ethernet.localIP());

  webserver.setDefaultCommand(&defaultCmd);
  webserver.addCommand("state", &stateCmd);
}

