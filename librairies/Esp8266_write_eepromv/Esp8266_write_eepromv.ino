
#include <EEPROM.h>
#include <C:\Users\jean\Documents\Arduino\libraries\Esp8266JC\Esp8266ReadEeprom.h>
// the current address in the EEPROM (i.e. which byte
uint8_t eepromVersion = 0x01;
#define addrStationLen 4
#define typeStationLen 4
uint8_t addrStation[4] = {0x00, 0x00, 0x00, 0x00}; // used to identify domotic stations
uint8_t typeStation[4] = {0x00, 0x00, 0x00, 0x01};
int addrADDR = 2;
int typeADDR = addrADDR + addrStationLen + 1;
byte valueEeprom;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  int eepromToWrite = (1+1 + addrStationLen + 1 + typeStationLen  );
  Serial.print("size eeprom to write:");
  Serial.println(eepromToWrite);
  EEPROM.begin(eepromToWrite);

  Serial.println(addrStationLen);
  Serial.println(typeStationLen);
  Serial.println(addrADDR);
  Serial.println(typeADDR);
  for (int i = 0; i < addrStationLen; i++)
  {
    Serial.print(addrStation[i], HEX);
  }
  Serial.println();
  AfficheEeprom();

  Serial.println("update eeprom in 20 secondes");
  delay(20000);
  MajEeprom();
  char* ad = GetParam(0);
  for (int i = 1; i < ad[0] + 1; i++)
  {
    Serial.print(ad[i], HEX);
  }
  Serial.print(":");
  Serial.println(sizeof(ad));

  char* ty = GetParam(1);
  for (int i = 1; i < ty[0] + 1; i++)
  {
    Serial.print(ty[i], HEX);
  }
  Serial.print(":");
  Serial.println(sizeof(ty));
 
  //
  /*
  EEPROM.write(addrEeprom, IDToInit);
   valueEeprom = EEPROM.read(addrEeprom);
   Serial.print("ID=");
   Serial.print(valueEeprom, DEC);
   Serial.println();
   */
}

void loop()
{
  AfficheEeprom();
  delay(10000);
}
void AfficheEeprom()
{
  Serial.print("eepromVersion:");
  Serial.println(EEPROM.read(0), HEX);
  Serial.print("addrStationLen:");
  Serial.println(EEPROM.read(addrADDR - 1));
  Serial.print("addrStation:");
  for (int i = 0; i < addrStationLen; i++)
  {
    Serial.print(EEPROM.read(addrADDR + i), HEX);
  }
  Serial.println();
  Serial.print("typeStationLen:");
  Serial.println(EEPROM.read(typeADDR - 1));
  Serial.print("typeStation:");
  for (int i = 0; i < typeStationLen; i++)
  {
    Serial.print(EEPROM.read(typeADDR + i), HEX);
  }
  Serial.println();
  
}
void MajEeprom()
{
  Serial.println("write");

  if ( EEPROM.read(0) !=  eepromVersion)
  {
    Serial.println(eepromVersion);
    EEPROM.write(0,  eepromVersion);

  }
  EEPROM.commit();
  delay(1500);
  if ( EEPROM.read(addrADDR - 1) !=  addrStationLen)
  {
    Serial.println(addrStationLen);
    EEPROM.write(addrADDR - 1,  addrStationLen);

  }
  EEPROM.commit();
  delay(1500);
  if ( EEPROM.read(typeADDR - 1) !=  typeStationLen)
  {
    Serial.println(typeStationLen);
    EEPROM.write(typeADDR - 1,  typeStationLen);

  }
  EEPROM.commit();
  delay(1500);

  for (int i = 0; i < addrStationLen; i++)
  {
    if ( EEPROM.read(addrADDR + i) !=  addrStation[i])
    {
      Serial.print(addrStation[i]);
      EEPROM.write(addrADDR + i,  addrStation[i]);
    }
    EEPROM.commit();
    delay(1500);
  }
  Serial.println();
  for (int i = 0; i < typeStationLen; i++)
  {
    Serial.println(i);
    Serial.println(EEPROM.read(typeADDR + i));
    Serial.println(typeStation[i]);
    if ( EEPROM.read(typeADDR + i) !=  typeStation[i])
    {
      EEPROM.write(typeADDR + i,  typeStation[i]);

    }

  }
  EEPROM.commit();
  delay(1500);
   Serial.println();
}


