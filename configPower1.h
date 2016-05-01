#define DS1820_PIN 4             // temperature sensor PIN GPIO4 
#define light_PIN 5             // light relay relay must be NO GPIO5
#define thermostat_PIN 12        // heating relay PIN  - relay must be NC GPIO12
#define switchPin 14            // switch PIN to localy power on/off GPIO14
char ssid1[] = "yourssid1";       // first SSID  will be used at first
char pass1[] = "yourkey1";      // first pass
char ssid2[] = "yourssid2";        // second SSID will be used in case on connection failure with the first ssid
char pass2[] = "yourkey2";  // second pass
unsigned long tempMaxAlert = 270;      // over this value an alert will be set on
unsigned long tempMinAlert = 230;       // under this value an alert will be set on
unsigned long tempSwitchOn = 0;      //  under this value the thermostat relay will be set on
unsigned long tempSwitchOff = 255;     //  under this value the thermostat relay will be set off
char defaultHostUdp[100] = "yourserver.home";  // defaut host address that could be change by newtork services
int defaultudpPort0 = 1903; // defaut host port for server time that will receive time request (could be change by newtork services)
int defaultudpPort1 = 1902;  // defaut host port for server SQL that will receive data (could be change by newtork services)