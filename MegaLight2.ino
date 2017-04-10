#include <SPI.h>
#include <SD.h>
#include <SDConfigFile.h>
#include <Ethernet.h>
#include <EventManager.h>
#include <TaskScheduler.h> 
#include <Bounce2.h>
#include <SimpleList.h>
#include <ExpressionEvaluator.h>
#include <WebServer.h>

#define DEBUG_RULES

#include "ml2enums.h"
#include "ml2classes.h"

OutputList outputList;
InputList inputList;

byte mac[] = { 0x34, 0xAD, 0xBE, 0x43, 0xFE, 0x68 };
byte ip[] = { 0, 0, 0, 0 };

String mdHost;
String mdAuth;
uint16_t mdPort = 80;



