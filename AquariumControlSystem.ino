/* this code has been written for ESP8266 device with Adruino 1.6.5 IDE
validated with Olimex ESP8266-EVB microcontroler
control lighting and heating
use DS1820 temperature sensor for monitoring water temperature (automaticaly detected)
use 2 relays one for light one for heating system
light on/off according to 3 different kinds of schedule 
  one weekly based
  two daily based
  can be turn on/off localy with a switch
  communication over Wifi and UDP
monitor and control over Internet via Java Server 

a station identifier has previously been stored in ESP8266 eeprom

for the download phase a 3.3v Serial/USB link must be connected to the ESP8266 module (Gnd,Tx,Rx)
*/
// include PIN and Wifi configuration
// #define debugOn true  // uncomment or comment this #define to swith on off the debug mode on serial link
#include <C:\Users\jean\Documents\ESP8266\librairies\OneWire\OneWire.h>
#include <C:\Users\jean\Documents\ESP8266\librairies\OneWire\OneWire.cpp>
// include schedule and configuration - must be adapted to your configuration
#include <C:\Users\jean\Documents\ESP8266\librairies\PowerTimer\configPower1030.h>
#include <C:\Users\jean\Documents\ESP8266\librairies\PowerTimer\schedullPower1030.h>
#define maxSendDataLenght 250   // maximum lenght of data to be sent to server
String ver = "AquariumControlSystem-V";
uint8_t vers = 0x02;
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <TimeLib.h>          // Time.h replaced by TimeLib.h to deal with IDE 1.6.9
#include <stdio.h>
#include <string.h>
#include <C:\Users\jean\Documents\Arduino\libraries\Esp8266JC\Esp8266ReadEeprom.h>
#define swDuration 1500  // minimal duration (ms) of push on the switch for beeing taken into account
#define tempDelay 60000  // duration (ms) between 2 readings of temperature
#define delayWifi 30000  // duration (ms)between 2 retries wifi
#define delayBetweenAlerts 300000 // duration (ms)between 2 sent of alerts (in case of no ack received)
#define delayBetweenSendMesurment 900000 // duration (ms)between 2 sent of mesurment 900000
String services[] = {"Time", "MajSql",  "Alerts", "Mails"};
char hostUdp[4][100];
#define nbUdpPort 4     // maximum number of udp ports
#define serviceAlert 2  // define alert service number
#define serviceMail 3   // define mail service number
int udpPort[nbUdpPort];  // store udp ports list
int servicesNumber = nbUdpPort;       // maximum services number - one by udp port
unsigned int udpListenPort = 8888;    // udp port the ESP8266 will listen to
char packetBuffer[UDP_TX_PACKET_MAX_SIZE]; //buffer to hold incoming packet,
// define how many clients should be able to connect to this ESP8266 at a time
#define MAX_SRV_CLIENTS 1
uint8_t currentSSID = 0x01;         // current used SSID
unsigned long timeRTC;  // duration between time refresh over network
unsigned long timeInit;  // last time we got updated time from the network
unsigned long timeRecvTCP;  // last time we received a frame from the network
unsigned long timeAlive;  // last time we sent a keep alive frame over the network
unsigned long timeCheckSchedul; // last time we check the lighting schedule
unsigned long timekSched;     // use to upload configuration to the server
unsigned long timeSwitch;     // last time switch has been pushed on
unsigned long timeTemp;       // clast time we read temperature
unsigned long timeDisableInterrupt; // use to disable switch during the power on off of the lighting (to avoid noise)
unsigned long timeAlert;          // last time we sent an alert
unsigned long switchDuration;     // duration of the current push on the switch
unsigned long timeWifi;          // last time we check the wifi conection
unsigned long timeCheckWifi;   // last time we tried to connect wifi
unsigned long timeSendMesurment;   // last time we sent mesurment
uint8_t flagDisableInterrupt = 0x00; // switch enable/disable status
uint8_t timeOk = 0x00;           // flag of the station time uptodate
uint8_t Diag = 0x93;            // internal diagnostic byte (Wifi,NA,NA,TimeUpTodate,NA,NA,schedulUpload,Memory)
uint8_t SavDiag = Diag;         // copy of internal diagnostic byte

int maxUDPResp = 100;           // maximum UDP response size
char resp[100] = "";
String Sresp = "                                                                                                       ";
String Srequest = "                                                                                                                                                                                                                                                                                                                     ";
// respTypeArray define response that could be sent to the server
String respTypeArray[] = {"respTime", "respGpio", "respAlive", "respStatus", "respAlert"};
#define nbrespTypeArray 5
// requestTypeArray define request that are supported coming from the server
String requestTypeArray[] = {"gpio/", "requestStatus", "requestAuto", "requestManu", "requestVa", "requestCa", "information/service", "requestSched/Info", "requestSched/Upd", "requestHg"};
#define nbrequestTypeArray 10
char stationId[5];
int kSched = sizeof(Schedul) - 1; // number of schedul entities
unsigned int freeMemory = 0;
unsigned long currentTemp;
WiFiUDP Udp;                     // start wifi UDP
WiFiServer server(888);          //  listening on 888 port
WiFiClient serverClients[MAX_SRV_CLIENTS]; //  
WiFiClient client;
char ModeListe[19] = "DiLuMaMeJeVeSaVaAb";    // week day
// below internal flags
uint8_t indCal = 0x00;          
uint8_t alertIdentifier = 0;
boolean alertOn = false;
boolean alertAck = false;
boolean sentMail = false;
int stAdd;
int val;
String ModeConsigne = "--";
int8_t IndForce = 0x00;            
bool Onoff = false;
uint8_t switchInt = 0x00;
byte DSaddr[8];
byte type_DS;
boolean tempSensor = false;  // temperatur sensor available or not
OneWire  ds(DS1820_PIN);  // on pin gpio04 (a 4.7K resistor is necessary)
void setup() {
  Serial.begin(38400);    // 
  EEPROM.begin(200);
  setTime(00, 00, 0, 01, 01, 2000);
  AffTime();
  Serial.print(ver);
  Serial.println(vers);
  // printf("%p\n", (void*) ssid);
  //  Pssid = &ssid[0];
  char* ad = GetParam(0);
  Serial.print("address:");
  for (int i = 1; i < ad[0] + 1; i++)
  {
    Serial.print(ad[i], HEX);
    Serial.print(":");
  }
  int a = ad[3] * 256 + ad[4];
  stAdd = a;               // set station address according to eeprom
  itoa(a, stationId, 10);   //  means 10 base
  Serial.println(stationId);

  Serial.print("type:");
  char* ty = GetParam(1);  
  for (int i = 1; i < ty[0] + 1; i++)
  {
    Serial.print(ty[i], HEX);
    Serial.print(":");
  }
  Serial.println();
  strcpy (hostUdp[0] , defaultHostUdp); // default server could be updated by services
  strcpy (hostUdp[1] , defaultHostUdp); // default server could be updated by services
  udpPort[0] = defaultudpPort0; // default server could be updated by services
  udpPort[1] = defaultudpPort1; // default server could be updated by services
  ConnectWifi();
  Serial.println();
  pinMode(light_PIN, OUTPUT);
  pinMode(thermostat_PIN, OUTPUT);
  digitalWrite(thermostat_PIN, false);
  digitalWrite(light_PIN, false);
  pinMode(switchPin, INPUT_PULLUP);
  timeInit = millis();
  attachInterrupt(switchPin, SwitchInterrupt, FALLING);
  LookForDS1820();  
  FreeMemory();
}

void loop() {
  int n = 0;
  delay(100);
  InputUDP();
  if (flagDisableInterrupt == 0x01 && (millis() - timeDisableInterrupt) >= 5000)
  {
#if defined(debugOn)
    Serial.println("interrupt On");
#endif
    attachInterrupt(switchPin, SwitchInterrupt, FALLING);
    flagDisableInterrupt = 0x00;

  }
  if (SavDiag != Diag) {
    Event();
  }
  if (bitRead(Diag, 7) == 1 && (millis() - timeCheckWifi) >= (120000 + random(0, 500)))
  {
    ConnectWifi();
  }
  if (timeOk != 0x01 && (millis() - timeInit) >= (30000 + random(0, 500))) //
  {
    timeInit = millis();
    ReqTimeUdp();
    delay(100);

  }
  if (  WiFi.status() != WL_CONNECTED && (millis() - timeWifi) >= delayWifi) //
  {
    timeWifi = millis();
    Serial.print("wifi status:");
    Serial.println(WiFi.status());
    FreeMemory();
    //   timeCheckWifi = millis();
    bitWrite(Diag, 7, 1);       // position bit diag
  }

  if (timeOk == 0x01 && (millis() - timeRTC) >= (900000 + random(0, 10000))) //
  {
    timeRTC = millis();
    ReqTimeUdp();
  }
  if ((millis() - timeRecvTCP) >= (1000 + random(0, 100))) //
  {
    timeRecvTCP = millis();
    //   ReadTcp();
  }
  if ((millis() - timeAlive) >= (300000 + random(0, 1000))) //
  {
    timeAlive = millis();
    KeepAlive();
  }
  if ((millis() - timeSendMesurment) >= (delayBetweenSendMesurment + random(0, 1000))) //
  {
    timeSendMesurment = millis();
    SendMesurment();
  }
  if ((millis() - timeRecvTCP) >= (1000 + random(0, 100))) //
  {
    timeRecvTCP = millis();
  }
  if (kSched < (sizeof(Schedul) - 1) && (millis() - timekSched) >= (10000 + random(0, 1000))) // il reste des infos a remonter
  {
    timekSched = millis();
    RespSchedulUdp();
  }
  if ((millis() - timeCheckSchedul) >= (15000 + random(0, 1000)) && timeOk == 0x01) //
  {
    timeCheckSchedul = millis();
    Onoff = CalcConsigne();
    if (IndForce == 0x00)
    {

      if (digitalRead(light_PIN) != Onoff)
      {
#if defined(debugOn)
        Serial.println("interrupt disable");
#endif
        TempDisableInterrupt(); // pour eviter parasites
        digitalWrite(light_PIN, Onoff);
        delay(100);
        SendMesurment();
      }

    }
    int currentMemory = ESP.getFreeHeap();
    if (currentMemory < freeMemory && freeMemory - currentMemory >= freeMemory * 0.1) {
      bitWrite(Diag, 0, 1);       // position bit diag
#if defined(debugOn)
      Serial.print("free memory:");
      Serial.println(currentMemory);
#endif
    }
    else {
      bitWrite(Diag, 0, 0);       // position bit diag
    }
  }
  if (Diag == 0x02 && millis() > 120000) // upload till to be done since reboot
  {
    kSched = 0; // force remontee infos temporairement supprime le 21/07/15
    bitWrite(Diag, 1, 0);
  }
  if ( switchInt == 0xff) // switch a confirmer par appui long
  {
    if ((millis() - switchDuration) < swDuration)
    {
      if ( digitalRead(switchPin) != 0)
      {
#if defined(debugOn)
        Serial.println("cancel switch");
#endif
        switchInt = 0x00;  // annulation
        attachInterrupt(switchPin, SwitchInterrupt, FALLING);
      }
    }
    else
    {
      switchInt = 0x0e;
    }
  }
  if ( switchInt == 0x0e)
  {
    //  attachInterrupt(switchPin, SwitchInterrupt, FALLING);
#if defined(debugOn)
    Serial.println("switch");
#endif
    timeSwitch = millis();
    switchInt = 0x01;
    IndForce = 0x01;
    digitalWrite(light_PIN, !digitalRead(light_PIN));
    delay(100);
    SendMesurment();
  }
  if ((millis() - timeSwitch) >= 5000 && switchInt == 0x01)
  {
    attachInterrupt(switchPin, SwitchInterrupt, FALLING);
    switchInt = 0x00;
  }
  uint8_t i;

  // check if there are any new clients
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
#if defined(debugOn)
        Serial.print("New client: ");
        Serial.println(i);
#endif
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }

  //check clients for data
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      uint8_t buf[256];
      int j = 0;
      if (serverClients[i].available()) {

#if defined(debugOn)
        Serial.print("received:");
#endif
        while (serverClients[i].available()) {
          //        Serial.println(sizeof(buf));
          if (j < sizeof(buf)) {
            buf[j] = (serverClients[i].read());
            Srequest[j] = buf[j];
#if defined(debugOn)
            Serial.print(buf[j], HEX);
            Serial.print("-");
#endif
            j = j + 1;
          }
        }
#if defined(debugOn)
        Serial.println();
#endif
        int requestNumber = find_requestType();
        int gpio = 0;
        int val = 0;
#if defined(debugOn)
        Serial.print("request:");
        Serial.println(requestNumber);
#endif
        if (requestNumber == 0)
        {

          if (buf[7] == 0x3f) // is it ? that means value is requested and gpio is one digit long
          {
            gpio = buf[5] - 0x30;
          }
          if (buf[8] == 0x3f) // is it ? that means value is requested and gpio is two digit long
          {
            gpio = (buf[5] - 0x30) * 10 + (buf[6] - 0x30);
          }
          if (buf[6] == 0x3d) // is it = that means requested is to set gpio and gpio is one digit long
          {
            if (buf[5] >= 0x30 && buf[5] <= 0x39 && (buf[7] == 0x30 || buf[7] == 0x31))  // check numerique
            {
              gpio = buf[5] - 0x30;
              val = buf[7] - 0x30;
            }
#if defined(debugOn)
            else {
              Serial.println("gpio received value not valid");
            }
#endif
          }

          if (buf[7] == 0x3d)  // is it = that means requested is to set gpio and gpio is two digit long
          {
            if (buf[5] >= 0x30 && buf[5] <= 0x39 && buf[6] >= 0x30 && (buf[6] <= 0x39 || buf[8] == 0x30 && buf[8] == 0x31)) // check numerique
            {
              gpio = (buf[5] - 0x30) * 10 + (buf[6] - 0x30);
              val = buf[8] - 0x30;
            }
#if defined(debugOn)
            else {
              Serial.println("gpio received value not valid");
            }
#endif
          }
#if defined(debugOn)
          Serial.print("gpio:");
          Serial.print(gpio);
          Serial.print(" value:");
          Serial.println(val);
#endif
          pinMode(gpio, OUTPUT);
          TempDisableInterrupt(); // pour eviter parasites
#if defined(debugOn)
          Serial.println("interrupt disa parasite");
#endif
          digitalWrite(gpio, val);
          delay(100);
          SendMesurment();
#if defined(debugOn)
          Serial.print("gpio:");
          Serial.print(gpio, HEX);
          Serial.println(val, HEX);
#endif
          IndForce = 0x01;
          RespStatus();
          //       }
        }
        if (requestNumber == 1)
        {
          RespStatus();
        }
        if (requestNumber == 2)
        {
          IndForce = 0x00;
          RespStatus();
        }
        if (requestNumber == 3)
        {
          IndForce = 0x01;
          RespStatus();

        }
        if (requestNumber == 4)
        {
          ModeConsigne = "Va";
          IndForce = 0x00;
          RespStatus();
        }
        if (requestNumber == 9)
        {
          ModeConsigne = "Hg";
          IndForce = 0x00;
          RespStatus();
        }
        if (requestNumber == 5)
        {
          ModeConsigne = "--";
          IndForce = 0x00;
          RespStatus();
        }
        if (requestNumber == 6)   // services
        {

          int posDNS = find_DNS();
          //        Serial.println(posDNS);
          int posPort = find_Port();
          //        Serial.println(posPort);
          int posEnd = find_requestEnd();
          //      Serial.println(posEnd);
          int posService = find_service();
          int serviceReceived = 0;
          for (int j = posDNS - 1; j > posService + 7; j--)
          {
            serviceReceived = (Srequest[j] - 0x30) * (pow(10, (posDNS - 1 - j))) + serviceReceived;
          }
          if (serviceReceived >= 1 || serviceReceived <= 4 )
          {
            if (posDNS != -1 && posPort > posDNS && posEnd > posPort)
            {
#if defined(debugOn)
              for (int j = posDNS + 1; j < posPort; j++)
              {
                Serial.print(Srequest[j]);
                Serial.print(" ");
              }
              Serial.println("");
#endif

              int sizeHost = sizeof(hostUdp[serviceReceived - 1]);
              if (posPort - posDNS + 6 < sizeHost)
              {
                int k = 0;
                for (int i = posDNS + 5; i < posPort; i++)
                {
                  hostUdp[serviceReceived - 1][k] = Srequest[i];
                  k = k + 1;
                }
                for (int i = k; i < sizeHost; i++)
                {
                  hostUdp[serviceReceived - 1][i] = 0x00;
                }
              }

            }

#if defined(debugOn)
            Serial.print("port:");
            for (int j = posPort + 1; j < posEnd; j++)
            {
              Serial.print(Srequest[j]);
            }
            Serial.println();
#endif
            int portReceived = 0;

            for (int j = posEnd - 1; j > posPort + 5; j--)
            {
              //             portReceived = (Srequest[j] - 0x30) * (10 ^ (posEnd - 1 - j)) + portReceived;
              portReceived = (Srequest[j] - 0x30) * (pow(10, (posEnd - 1 - j))) + portReceived;
            }
#if defined(debugOn)
            Serial.print("service:");
            Serial.println(serviceReceived);
#endif
            udpPort[serviceReceived - 1] = portReceived;
          }
#if defined(debugOn)
          PrintUdpConfig();
#endif
        }

        if (requestNumber == 7)  // request Schedul info
        {
          kSched = 0; // force remontee infos
        }
        if (requestNumber == 8)  // request maj schedul
        {
          uint8_t Value = 0x00;   // on byte value for schedule
          int IValue = 0;         // int value
          uint8_t indIndex = 0x00;
          int posID = find_SchedID() + 3;
          int posStringValue = find_SchedValue();
          int posValue = posStringValue + 6;
          int posEnd = find_requestEnd();
          boolean negative = false;
#if defined(debugOn)
          Serial.print("posID:");
          Serial.print(posID);
          Serial.print(" posStringValue:");
          Serial.print(posStringValue);
          Serial.print(" posValue:");
          Serial.print(posValue);
          Serial.print(" posEnd:");
          Serial.println(posEnd);
#endif
          unsigned int idx = 0;
          if (posID != 2 && posValue > posID && posEnd > posID && Srequest[posID] != 0x2d)
          {
            //                        Serial.println(posID);
            int lenValue = posEnd - posValue ;
            if (Srequest[posValue] == 0x2d)     // start with minus sign
            {
              lenValue = lenValue - 1;
              posValue = posValue + 1;
              negative = true;
            }
            int lenInd = posStringValue - posID ;
            for (int j = posEnd - 1; j >= posValue ; j--)
            {
              //             portReceived = (Srequest[j] - 0x30) * (10 ^ (posEnd - 1 - j)) + portReceived;
              Value = uint8_t((Srequest[j] - 0x30) * (pow(10, (posEnd - 1 - j))) + Value);
              IValue = ((Srequest[j] - 0x30) * (pow(10, (posEnd - 1 - j))) + IValue);
#if defined(debugOn)
              Serial.print(" Value:");
              Serial.print(Value);
              Serial.print(" IValue:");
              Serial.print(IValue);
#endif
            }
#if defined(debugOn)
            Serial.println();

#endif
            if (negative)
            {
              Value = uint8_t(256 - Value);
              IValue = -IValue;
            }
            for (int j = posStringValue - 1; j >= posID ; j--)
            {
              //             portReceived = (Srequest[j] - 0x30) * (10 ^ (posEnd - 1 - j)) + portReceived;
              int tmp = (Srequest[j] - 0x30) * (pow(10, (posStringValue - 1 - j)));
              idx = tmp + idx;
            }
          }
          if (idx >= 50 && idx <= 265)
          {
#if defined(debugOn)
            Serial.print("MajSchecd:");
            Serial.print(idx);
            Serial.print(">");
            Serial.println(Value, HEX);
#endif
            Schedul[idx - 50] = uint8_t(Value);
            RespSchedulUnit(idx - 50);
          }
          else {
#if defined(debugOn)
            Serial.print("Majinf:");
            Serial.print(idx);
            Serial.print(">");
            Serial.println(IValue);
#endif
          }
          if (idx == 10)
          {
            tempMaxAlert = IValue;
          }
          if (idx == 11)
          {
            tempMinAlert = IValue;
          }
          if (idx == 12)
          {
            tempSwitchOn = IValue;
          }
          if (idx == 13)
          {
            tempSwitchOff = IValue;
          }
          RespStatus();
        }

      }
    }
  }

  if (tempSensor == true && (millis() - timeTemp) >= tempDelay)
  {
    ReadDS1820();
    timeTemp = millis();
  }
}


void SetTime(char *input) {
  uint8_t jj = ((input[0] - 0x30) * 10 + (input[1] - 0x30));
  uint8_t mm = ((input[3] - 0x30) * 10 + (input[4] - 0x30));
  uint8_t aa = ((input[6] - 0x30) * 10 + (input[7] - 0x30));

  uint8_t hh = ((input[9] - 0x30) * 10 + (input[10] - 0x30));
  uint8_t mn = ((input[12] - 0x30) * 10 + (input[13] - 0x30));
  uint8_t ss = ((input[15] - 0x30) * 10 + (input[16] - 0x30));

  setTime(hh, mn, ss, jj, mm, 2000 + aa);
  AffTime();
  timeOk = 0x01;
  timeRTC = millis();
  bitWrite(Diag, 4, 0);       // position bit diag
}

void AffTime()
{

  Serial.println();
  Serial.print(hour());
  Serial.print(":");
  Serial.print(minute());
  Serial.print(":");
  Serial.print(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print("/");
  Serial.print(month());
  Serial.print("/");
  Serial.print(year());
  Serial.print(" ");
  Serial.println(weekday());

}

int find_respType() {
  int foundpos = -1;
  String needle = "               ";
  for (int r = 0; r < nbrespTypeArray; r++) {
    needle = respTypeArray[r];

    if (Sresp.length() - needle.length() >= 0) {
      for (int i = 0; (i < Sresp.length() - needle.length() + 1); i++) {
        //      Serial.println((haystack.substring(i, needle.length() + i)));
        if (Sresp.substring(i, needle.length() + i) == needle) {
          foundpos = r;
          return foundpos;
        }
      }
    }
  }
  return foundpos;
}
int find_requestType() {
  int foundpos = -1;
  String needle = "               ";
  for (int r = 0; r < nbrequestTypeArray; r++) {
    needle = requestTypeArray[r];

    if (Srequest.length() - needle.length() >= 0) {
      for (int i = 0; (i < Srequest.length() - needle.length() + 1); i++) {
        //      Serial.println((haystack.substring(i, needle.length() + i)));
        if (Srequest.substring(i, needle.length() + i) == needle) {
          foundpos = r;
          return foundpos;
        }
      }
    }
  }

  return foundpos;
}
int find_respEnd() {
  int foundpos = -1;
  String needle = "\\End";

  if (Sresp.length() - needle.length() >= 0) {
    for (int i = 0; (i < Sresp.length() - needle.length() + 1); i++) {
      //      Serial.println((haystack.substring(i, needle.length() + i)));
      if (Sresp.substring(i, needle.length() + i) == needle) {
        foundpos = i;
        return foundpos;
      }
    }
  }
  return foundpos;
}
int find_requestEnd() {
  int foundpos = -1;
  String needle = "\\End";


  if (Srequest.length() - needle.length() >= 0) {
    for (int i = 0; (i < Srequest.length() - needle.length() + 1); i++) {
      //      Serial.println((haystack.substring(i, needle.length() + i)));
      if (Srequest.substring(i, needle.length() + i) == needle) {
        foundpos = i;
        return foundpos;
      }
    }

  }
  return foundpos;
}
void RespStatus() {
  String req = "";
  req += stationId;
  req += "/status";
  req += "/ind_id/";
  req += 1;
  req += "=";
  req += digitalRead(light_PIN);
  req += "/ind_id/";
  req += 2;
  req += "=";
  req += uint8_t(Diag);
  req += "/ind_id/";
  req += 3;
  req += "=";
  req += uint8_t(IndForce);
  req += "/ind_id/";
  req += 4;
  req += "=";
  if (ModeConsigne == "Va" || ModeConsigne == "Hg" )
  {
    if (ModeConsigne == "Va")
    {
      req += 0x01;
    }

    if (ModeConsigne == "Hg")
    {
      req += 0x02;
    }
  }
  else
  {
    req += 0x00;
  }
  req += "/ind_id/";
  req += 5;
  req += "=";
  req += uint8_t(indCal);
  req += "/ind_id/";
  req += 6;
  req += "=";
  req += uint8_t(vers);
  req += "/ind_id/";
  int timeSinceBoot = millis() / 60000;
  req += 7;
  req += "=";
  req += timeSinceBoot;
  req += "/ind_id/";
  req += 8;
  req += "=";
  req += currentTemp;
  req += "/ind_id/";
  req += 9;
  req += "=";
  req += !digitalRead(thermostat_PIN); // relay NC true means power off false means power on
  req += "/ind_id/";
  req += 10;
  req += "=";
  req += tempMaxAlert;
  req += "/ind_id/";
  req += 11;
  req += "=";
  req += tempMinAlert;
  req += "/ind_id/";
  req += 12;
  req += "=";
  req += tempSwitchOn;
  req += "/ind_id/";
  req += 13;
  req += "=";
  req += tempSwitchOff;
  req += "\\End"; // required end off frame
  req += "\r\n";  // marqueur de fin indispensable
  req += 0x00;    // marqueur de fin indispensable

  {
#if defined(debugOn)
    Serial.print("Status: ");
    Serial.println(req);
#endif
  }
  PrepareSendToUdp(req, 1);


  // This will send the request to the server

  delay(10);
  // client.print(0x10);
  //   client.flush();
}
void SendMesurment() {
  String req = "";
  req += stationId;
  req += "/mesurment";
  req += "/ind_id/";
  req += 1;
  req += "=";
  req += currentTemp;
  req += "/ind_id/";
  req += 2;
  req += "=";
  req += !digitalRead(thermostat_PIN); // relay NC true >>  means power off
  req += "/ind_id/";
  req += 3;
  req += "=";
  req += digitalRead(light_PIN); // relay NO true >>  means power on
  req += "\\End"; // required end off frame
  req += "\r\n";  // marqueur de fin indispensable
  req += 0x00;    // marqueur de fin indispensable

  {
#if defined(debugOn)
    Serial.print("Mesurment: ");
    Serial.println(req);
#endif
  }
  PrepareSendToUdp(req, 1);
  // This will send the request to the server

  delay(10);
  // client.print(0x10);
  //   client.flush();
}
void KeepAlive() {
  String req = "";
  req += stationId;
  req += "/";
  req += "information";
  req += "/";
  req += "alive/";
  req += "\\End"; // marqueur de fin indispensable
  req += "\r\n"; // marqueur de fin indispensable
  req += 0x00; // marqueur de fin indispensable
#if defined(debugOn)
  Serial.print("Send: ");
  Serial.println(req);
#endif
  PrepareSendToUdp(req, 1);
  delay(10);

}
bool CalcConsigne() {
  int i = 0;
  int f = 0;
  uint8_t DemiH = 0x00;
  bool retOnOff = 0x00;
  if (ModeConsigne == "Va" || ModeConsigne == "Hg" ) {
    //   i=7;
    i = (7 * 24 + hour()); //
    if (ModeConsigne == "Hg" )
    {
      i = (8 * 24 + hour());
    }
    f = ((weekday()) - 1 * 24 + hour());
  }

  else {
    ModeConsigne[0] = ModeListe[(weekday() - 1) * 2];
    ModeConsigne[1] = ModeListe[(weekday() - 1) * 2 + 1];
    i = ((weekday() - 1) * 24 + hour());
    //   f = ((weekday()-1) * 24 + hour());
  }

  int k = minute();
#if defined(debugOn)
  Serial.print("i: ");
  Serial.print(i);
  Serial.print(" sched: ");
  Serial.print(Schedul[i], BIN);
  Serial.print(" minute: ");
  Serial.print(k);
#endif
  k = k / 8;
#if defined(debugOn)
  Serial.print(" k: ");
  Serial.println(k);
#endif
  retOnOff = bitRead(Schedul[i], 7 - k);
  indCal = uint8_t(i + 50);

  return retOnOff;


}


void ConnectWifi()
{
  timeCheckWifi = millis();
  if (currentSSID == 0x01) {
    WiFi.begin(ssid1, pass1);
    Serial.print("\nConnecting to "); Serial.println(ssid1);
  }
  else
  {
    WiFi.begin(ssid2, pass2);
    Serial.print("\nConnecting to "); Serial.println(ssid2);
  }
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 5) delay(2000);
  bitWrite(Diag, 7, 0);       // position bit diag
  if (i == 5) {
    Serial.print("Could not connect to"); Serial.println(pass2);
    while (1) delay(500);
    bitWrite(Diag, 7, 1);       // position bit diag
    Udp.stop();
    if (currentSSID == 0x01) {
      currentSSID = 0x02;
    }
    else
    {
      currentSSID = 0x01;
    }
  }
  else
  {
    WiFi.mode(WIFI_STA);
    server.begin();
    server.setNoDelay(true);
    Serial.print("Ready! Use ");
    Serial.print(WiFi.localIP());
    Serial.println(" 888' to connect");
    PrintUdpConfig();

    Udp.begin(udpListenPort);
    delay (5000);
    WiFi.printDiag(Serial);
  }
}
void Event() {
  SavDiag = Diag;       // sauvegarde Diag precedent

  RespStatus();
}
int find_DNS() {
  int foundpos = -1;
  String needle = "/DNS=";

  if (Srequest.length() - needle.length() >= 0) {
    for (int i = 0; (i < Srequest.length() - needle.length() + 1); i++) {
      //      Serial.println((haystack.substring(i, needle.length() + i)));
      if (Srequest.substring(i, needle.length() + i) == needle) {
        foundpos = i;
        return foundpos;
      }
    }
  }

}
int find_Port() {
  int foundpos = -1;
  String needle = "/port=";



  if (Srequest.length() - needle.length() >= 0) {
    for (int i = 0; (i < Srequest.length() - needle.length() + 1); i++) {
      //      Serial.println((haystack.substring(i, needle.length() + i)));
      if (Srequest.substring(i, needle.length() + i) == needle) {
        foundpos = i;
        return foundpos;
      }
    }
  }

}
int find_service() {
  int foundpos = -1;
  String needle = "/service";

  if (Srequest.length() - needle.length() >= 0) {
    for (int i = 0; (i < Srequest.length() - needle.length() + 1); i++) {
      //      Serial.println((haystack.substring(i, needle.length() + i)));
      if (Srequest.substring(i, needle.length() + i) == needle) {
        foundpos = i;
        return foundpos;
      }
    }
  }

}
int find_SchedID() {
  int foundpos = -1;
  String needle = "ID=";



  if (Srequest.length() - needle.length() >= 0) {
    for (int i = 0; (i < Srequest.length() - needle.length() + 1); i++) {
      //      Serial.println((haystack.substring(i, needle.length() + i)));
      if (Srequest.substring(i, needle.length() + i) == needle) {
        foundpos = i;
        return foundpos;
      }
    }
  }

}


int find_SchedValue() {
  int foundpos = -1;
  String needle = "value=";

  if (Srequest.length() - needle.length() >= 0) {
    for (int i = 0; (i < Srequest.length() - needle.length() + 1); i++) {
      //      Serial.println((haystack.substring(i, needle.length() + i)));
      if (Srequest.substring(i, needle.length() + i) == needle) {
        foundpos = i;
        return foundpos;
      }
    }
  }



}
void RespSchedulUnit(int id) {



  //  int nbLoop = (48);
  //  for (int i = 0; i < nbLoop; i++)
  //  {
  String req = "";
  req += stationId;
  req += "/status";
  req += "/ind_id/";
  req += id + 50;
  req += "=";
  req += Schedul[id];
  req += "\\End"; // marqueur de fin indispensable
  req += "\r\n"; // marqueur de fin indispensable
  req += 0x00; // marqueur de fin indispensable
  PrepareSendToUdp(req, 1);

  // This will send the request to the server

  delay(1000);
}

void RespSchedulUdp() {
  int nbLoop = (54);
  String req = "";
  req += stationId;
  req += "/status";
  req += "/ind_id/";
  req += kSched + 50;
  req += "=";
  req += Schedul[kSched];
  req += "/ind_id/";
  req += kSched + 51;
  req += "=";
  req += Schedul[kSched + 1];
  req += "/ind_id/";
  req += kSched + 52;
  req += "=";
  req += Schedul[kSched + 2];
  req += "/ind_id/";
  req += kSched + 53;
  req += "=";
  req += Schedul[kSched + 3];
  req += "\\End"; // marqueur de fin indispensable
  req += "\r\n"; // marqueur de fin indispensable
  req += 0x00; // marqueur de fin indispensable
  PrepareSendToUdp(req, 1);
  kSched = kSched + 4;


  delay(1000);

}
void ReqTimeUdp() {

  String req = "";
  req += stationId;
  req += "/";
  req += "request";
  req += "/";
  req += "time";
  req += "\r\n";
  req += 0x00; // marqueur de fin indispensable
#if defined(debugOn)
  Serial.print("Requesting: ");
  Serial.println(req);
#endif
  PrepareSendToUdp(req, 0);
}
void PrepareSendToUdp(String req, uint8_t serviceId)
{ byte dataBin[maxSendDataLenght + 1];
  int len = 0;
  for (int i = 0; i <= maxSendDataLenght; i++)
  {
    if (req[i] == 0x00)
    {
      len = i - 3;
      break;
    }
    dataBin[i] = req[i];

  }
  SendToUdp(dataBin, len, serviceId);
}
void SendToUdp(byte * msg2, int mlen, uint8_t serviceId) {
  Udp.beginPacket(hostUdp[serviceId], udpPort[serviceId]);
  Udp.write(msg2, mlen);
  Udp.endPacket();
}

void SwitchInterrupt()
{
#if defined(debugOn)

  Serial.println("interrupt ");
#endif
  detachInterrupt(switchPin);
  switchInt = 0xff;
  timeSwitch = millis();
  switchDuration = millis();
}

void TempDisableInterrupt()
{
  detachInterrupt(switchPin);
  timeDisableInterrupt = millis();
  flagDisableInterrupt = 0x01;
}
void InputUDP() {
  int packetSize = Udp.parsePacket();  // message from UDP

  if (packetSize)
  {
    IPAddress remote = Udp.remoteIP();
#if defined(debugOn)
    Serial.print("UDPRSize ");
    Serial.print(packetSize);
    Serial.print(": ");

    for (int i = 0; i < 4; i++)
    {
      Serial.print(remote[i], DEC);
      if (i < 3)
      {
        Serial.print(".");
      }
    }
    Serial.print(",port ");
    Serial.print(Udp.remotePort());
    Serial.print(" ");
#endif
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    int packetBufferLen = packetSize;
#if defined(debugOn)
    Serial.print("awnser:");
    Serial.println(packetBufferLen);
#endif
    int lf = 10;
    for (int i = 0; i < maxUDPResp; i++)
    {
      resp[i] = 0x20;
    }
    for (int i = 0; i < maxUDPResp; i++)
    {

      Sresp[i] = packetBuffer[i];
    }

    // Serial.println();
#if defined(debugOn)
    Serial.println(Sresp);
    Serial.println();
#endif
    if (find_respEnd() >= 0)  // fin de trame trouvee
    {


      int f = find_respType();


      if (f == 0) { // resptime
        //       Serial.println("time");
        char param[20] ;
        for (int i = 0; i < 17; i++)
        {
          param[i] = packetBuffer[i + 9];
        }
        char *ptr = NULL;
        ptr = param;
        SetTime(ptr);
      }
      if (f == 4) { //
        Serial.println("ack alert");
        alertAck = true;
      }
    }
  }
}
void PrintUdpConfig() {
  for (int i = 0; i < (servicesNumber); i++)
  {
    Serial.print(services[i]);
    Serial.print(" Udp");
    Serial.print(i);
    Serial.print(" ");
    Serial.print(hostUdp[i]);
    Serial.print(" port:");
    Serial.println(udpPort[i]);

  }
}

void LookForDS1820()
{

  if ( !ds.search(DSaddr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }

  Serial.print("ROM =");
  for ( int i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(DSaddr[i], HEX);
  }

  if (OneWire::crc8(DSaddr, 7) != DSaddr[7]) {
    Serial.println("CRC is not valid!");
    return;
  }
  Serial.println();

  // the first ROM byte indicates which chip
  switch (DSaddr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_DS = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_DS = 0;
      tempSensor = true;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_DS = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
      ds.reset();
      ds.select(DSaddr);
      ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  }

  if (tempSensor == true)
  {
    ds.reset();
    ds.select(DSaddr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  }
}

void ReadDS1820()
{
  ds.reset();
  ds.select(DSaddr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  delay(750);
  byte present = 0;
  byte data[12];
  present = ds.reset();
  ds.select(DSaddr);
  ds.write(0xBE);         // Read Scratchpad
#if defined(debugOn)
  Serial.print("  Data = ");
  Serial.print(present, HEX);
  Serial.print(" ");
#endif
  for ( int i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
#if defined(debugOn)
    Serial.print(data[i], HEX);
    Serial.print(" ");
#endif
  }
#if defined(debugOn)
  Serial.print(" CRC=");
  Serial.print(OneWire::crc8(data, 8), HEX);
  Serial.println();
#endif
  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_DS) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  float celsius = ((float)raw / 16.0);
  currentTemp = celsius * 10;
#if defined(debugOn)
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.println(currentTemp);

#endif
  if (currentTemp >= (tempMinAlert + 0.1) && currentTemp <= (tempMaxAlert - 0.1))
  {
    if (alertOn == true && udpPort[serviceAlert] != 0)
    {
      SendAlert(3, 3);
    }
    alertOn = false;
    alertAck = true;
    sentMail = false;
  }

  if (alertOn == false && (currentTemp < tempMinAlert || currentTemp > tempMaxAlert)  )
  {
    alertIdentifier++;
    alertOn = true;
    alertAck = false;
  }
  if (alertOn == true && sentMail == false  )
  {
    if ( (currentTemp > tempMaxAlert) )
    {
      SendMail(1);
      sentMail = true;
    }
    else
    {
      SendMail(2);
      sentMail = true;
    }
  }

  if (millis() - timeAlert > delayBetweenAlerts && alertAck == false && udpPort[serviceAlert] != 0)
  {
    timeAlert = millis();
    SendAlert(1, 1);
  }

  if (currentTemp < tempSwitchOn && digitalRead(thermostat_PIN) == true)  // relay NC true means power off
  {
    digitalWrite(thermostat_PIN, false);
    delay(100);
    SendMesurment();
    //   RespStatus();
  }
  if (currentTemp > tempSwitchOff && digitalRead(thermostat_PIN) == false) // relay NC false means power on
  {
    digitalWrite(thermostat_PIN, true);
    delay(100);
    SendMesurment();
    //   RespStatus();
  }
  if (currentTemp >= tempSwitchOn && currentTemp <= tempSwitchOff && digitalRead(thermostat_PIN) == true) // relay NC true means power off
  {
    digitalWrite(thermostat_PIN, false);
    delay(100);
    SendMesurment();
    //   RespStatus();
  }
}
void FreeMemory()
{
  freeMemory = ESP.getFreeHeap();
  Serial.print("free memory:");
  Serial.println(freeMemory);
  if (freeMemory >= 10000)
  {
    bitWrite(Diag, 0, 0);       // position bit diag
  }
}
void SendAlert(uint8_t messType, uint8_t alertStatus) {    // (0x01 alert on 0x03 alert off , 0x01 sever 0x03 minor)
#if defined(debugOn)
  Serial.println("sendAlert");
#endif
#define alertLen 15
  uint8_t dataBin[alertLen + 1];
  //  alertIdentifier=alertIdentifier+1;
  dataBin[0] = 0x00;
  dataBin[1] = uint8_t(stAdd / 256);
  dataBin[2] = uint8_t(stAdd);
  dataBin[3] = 0x3B;
  dataBin[4] = 0x4D; // M master
  dataBin[5] = 0x08; // len
  dataBin[6] = 0x01;
  dataBin[7] = 0x00; // identifier force a 0 pour compatibilite cpt sur 2 octets
  dataBin[8] = alertIdentifier; // identifier max 255
  dataBin[9] = alertStatus;
  dataBin[10] = messType; // msgType alert
  dataBin[11] = 0x04; // category security
  dataBin[12] = 0x01; // urgency immediate
  dataBin[13] = 0x03; // severity moderate
  dataBin[14] = 0x00; // responseType NA
  SendToUdp( dataBin, alertLen, serviceAlert);

}

void SendMail(uint8_t mailId) {  // envoi trame vers batch listenMail
#if defined(debugOn)
  Serial.println("sendMail");
#endif
#define mailLen 8
  uint8_t dataBin[mailLen + 1];
  dataBin[0] = 0x00;
  dataBin[1] = uint8_t(stAdd / 256);
  dataBin[2] = uint8_t(stAdd);
  dataBin[3] = 0x3B;
  dataBin[4] = 0x4D; // M master
  dataBin[5] = uint8_t(mailLen); // len
  dataBin[6] = 0x3b;
  dataBin[7] = mailId; // selection de l id du mail
  SendToUdp( dataBin, mailLen, serviceMail);
}

