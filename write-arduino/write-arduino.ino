#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN D8
#define RST_PIN D0

#ifndef STASSID
#define STASSID "GAK PUNYA KUOTA 3"
#define STAPSK "5U54ntha"
//#define STASSID "hiJoy!"
//#define STAPSK "jona9600"
//#define STASSID "Laravel"
//#define STAPSK "pEgUYAngAN#?!421060@pEnGUkUH"
//  WiFi.begin("Laravel", "pEgUYAngAN#?!421060@pEnGUkUH");
#endif

MFRC522 mfrc522(SS_PIN, RST_PIN); //Create MFRC522 Instance

MFRC522::MIFARE_Key key;
char writeData[300];
String readData="";
byte count = 0;
bool flag = false;

byte blockAddr;

const char* ssid     = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);
const int led = 13;

void dump_byte_array(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void writeCard(String text) {
  int data_len = text.length()+1;
  char writeData[data_len];
  text.toCharArray(writeData, data_len);


    //Show some details of the PICC(that is: tje tag/card)
    Serial.print(F("Card UID: "));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();

    Serial.print(F("PICC Type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    Serial.println("Written Data: ");
    Serial.println(writeData);
    Serial.println("Lenght Data: ");
    Serial.println(data_len);
    Serial.println();

    MFRC522::StatusCode status;
    byte buffer[18];
    byte dataBlock[16];
    byte size = sizeof(buffer);
    byte startSector = 1;           //start write from this sector value
    byte blockNo = 0;               //start write from block 0

    byte sector = startSector;

    //Step 1. Authenticate using key A
    byte trailerBlock = (sector*4) + 3; //sector trailer is block 3 in each sector
    Serial.print(F("Authenticating using key A for sector "));
    Serial.println(sector);
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK)
    {
      Serial.print(F("PCD_Authenticate() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return;
    }

    memset(dataBlock,0,sizeof(dataBlock));
    for(int i=0;i<data_len;i++)
    {
      dataBlock[i%16] = writeData[i];

      if(i%16==15 || i==data_len-1)
      {
        if(blockNo < 3)
          blockAddr = (sector*4) + blockNo; //block 1 in sector 1
        else
        {
          blockNo = 0;
          sector++;
          byte trailerBlock = (sector*4) + 3; //sector trailer is block 3 in each sector
          //Authenticate again in next sector
          Serial.println(F("Authenticating using key A..."));
          status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
          if (status != MFRC522::STATUS_OK)
          {
            Serial.print(F("PCD_Authenticate() failed: "));
            Serial.println(mfrc522.GetStatusCodeName(status));
            return;
          }
          blockAddr = (sector*4) + blockNo; //block 1 in sector 1
        }

        //Write data in blockAddr
        Serial.print(F("Writing data into sector "));
        Serial.print(sector);
        Serial.print(F(" and block "));
        Serial.print(blockAddr); Serial.println(F(" ..."));
        dump_byte_array(dataBlock, 16); Serial.println();
        status - (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
        if (status != MFRC522::STATUS_OK)
        {
          Serial.print(F("MIFARE_Write() failed: "));
          Serial.println(mfrc522.GetStatusCodeName(status));
        }

        blockNo++;
        memset(dataBlock,0,sizeof(dataBlock));
      }
    }
    //Halt PICC
    mfrc522.PICC_HaltA();
    //Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();

}


boolean checkKartu()
{
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
      Serial.println("masuk atas");
      delay(50);
      return false;
    }

    if ( ! mfrc522.PICC_ReadCardSerial()) {
      Serial.println("masuk bawah");
      delay(50);
      return false;
    }
    Serial.println("masuk paling bawah");
    return true;
}


void testKonek() {
  if (server.method() != HTTP_GET) {
    digitalWrite(led, 1);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, 0);
  } else {
    digitalWrite(led, 1);
    DynamicJsonDocument doc(512);
    doc["message"] = "sukses konek";

    String buf;
    serializeJson(doc, buf);
    server.send(200, "application/json", buf);
    digitalWrite(led, 0);
  }
}

void writeDataKrama() {
  if (server.method() != HTTP_POST) {
    digitalWrite(led, 1);
    server.send(405, "text/plain", "Method Not Allowed");
    digitalWrite(led, 0);
  } else {
    digitalWrite(led, 1);
    String message = "POST form was:\n";
    DynamicJsonDocument doc(512);
     message += " " + server.argName(0) + ": " + server.arg(0) + "\n";
      Serial.println(server.arg(0));
      if (checkKartu() == true) {
        writeCard(server.arg(0));
        doc["message"] = "sukses write";
      } else {
        Serial.println("No Kartu");
        doc["message"] = "no kartu";
      }
    String buf;
    serializeJson(doc, buf);
    server.send(200, "application/json", buf);
  }
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  SPI.begin(); //init SPI bus
  mfrc522.PCD_Init(); //init MFRC522 card

  //Prepare the key (used both as key A and as key B)
  //Using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Waiting connection...");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/test/", testKonek);
  server.on("/write/", writeDataKrama);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
}
