/*KODE INI BERJALAN SESUAI KEINGINAN UNTUK READ DAN MENDECODE DATA DARI KARTU
 * KODE INI MASIH EKSPERIMENTAL
*/
#include <SPI.h>
#include <MFRC522.h>
#define SS_PIN D8
#define RST_PIN D4
MFRC522 mfrc522(SS_PIN, RST_PIN); //Create MFRC522 Instance
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#define PUSH_BUTTON D1
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x3F,16,2);


int waitForWifiConnect;
int wifiLastStatus;
int presensiAvailable = 0;
int buttonState;

WiFiClient client;
HTTPClient http;

String noCacahKramaMipil, uid = "test123";

//const char* host = "192.168.1.5";
//const uint16_t port = 1234;


MFRC522::MIFARE_Key key;
String readData = "";
byte count = 0;
bool flag = false;

byte blockAddr;

void setup() {
  lcd.init();   // initialize the lcd 
  lcd.backlight();
  Serial.begin(115200);
  while (!Serial); //Do nothing if no serial port is opened
  SPI.begin(); //init SPI bus
  mfrc522.PCD_Init(); //init MFRC522 card

  //Prepare the key (used both as key A and as key B)
  //Using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  
  WiFi.mode(WIFI_STA);
//  WiFi.begin("Laravel", "pEgUYAngAN#?!421060@pEnGUkUH");
  
  WiFi.begin("GAK PUNYA KUOTA 3", "5U54ntha");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Waiting for connection");
    waitForWifiConnect++;
    if (waitForWifiConnect == 20){ //klo dh 20x nge retry, keluar dari loop
      Serial.println("wifi fail");
      break;
    }
  }

  Serial.println(F("Read data in card"));
  Serial.print(F("Using key (for A and B):"));
  dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
  Serial.println();
}

void loop() {
  lcd.clear();
  lcd.setCursor(0,0);  
  lcd.print("Alat Siap");
  if (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.setCursor(0,0);  
    lcd.print("Reconnecting..");
    WiFi.reconnect();
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Waiting for connection");
      waitForWifiConnect++;
      if (waitForWifiConnect == 20){ //klo dh 20x nge retry, keluar dari loop
        Serial.println("wifi fail");
        break;
      }
  }
  }
  else {
    if ( kramaCard() == 1) {
         readDataKrama();  
      } else {
        Serial.println("no card");
      } 
    }
  delay(2000);
}

void fillPresensi(String nick, String uidCard) {
   // Your Domain name with URL path or IP address with path
    http.begin(client, "http://presensi-harry.herokuapp.com/api/presensi/fill-presensi");
    http.addHeader("Content-Type", "application/json");
    // Data to send with HTTP POST
    // Prepare JSON document
    DynamicJsonDocument doc(2048);
    doc["nomor_cacah_krama_mipil"] = nick;
    doc["uid_kartu"] = uidCard;

    String json;
    serializeJson(doc, json);     
    // Send HTTP POST request 
    
    int httpResponseCode = http.POST(json);
    String payload = http.getString();
    Serial.println(nick);
    StaticJsonDocument<500> jsonBuffer;
    deserializeJson(jsonBuffer, payload); // parse body dari hasil post readgate di servernya
    String data = jsonBuffer["message"];
    String kodePresensi = jsonBuffer["kode_presensi"];
    
    if (httpResponseCode == 200) { // pastiin respon nya 200
   
      Serial.println(data);
      if (data == "data presensi sukses") {
          Serial.println("sukses ngisi presensi");
          Serial.print(http.getString()); 
          lcd.clear();
          lcd.setCursor(0,0);  
          lcd.print("Presensi Berhasil");
          lcd.setCursor(0,1);
          lcd.print("Kode: "  + kodePresensi); 
      } else if (data == "presensi telah diisi") {
          Serial.println("presensi telah diisi");
          Serial.print(http.getString());
          lcd.clear();
          lcd.setCursor(0,0);  
          lcd.print("Presensi Telah");
          lcd.setCursor(0,1);
          lcd.print("Terisi");  
      } else if (data == "tidak terdaftar") {
          Serial.println("tidak terdaftar");
          Serial.print(http.getString()); 
          lcd.clear();
          lcd.setCursor(0,0);  
          lcd.print("Tidak terdaftar");
          lcd.setCursor(0,1);
          lcd.print("pada Presensi"); 
      }
    }
    else if (data == "tidak terdaftar") {
      Serial.println("tidak terdaftar");
      Serial.print(httpResponseCode);
      lcd.clear();
      lcd.setCursor(0,0);  
      lcd.print("Tidak terdaftar");
      lcd.setCursor(0,1);
      lcd.print("pada Presensi"); 
    } else {
      lcd.clear();
      lcd.setCursor(0,0);  
      lcd.print("Tidak ada");
      lcd.setCursor(0,1);
      lcd.print("Presensi terbuka"); 
      Serial.println("no presensi open");
      Serial.print(httpResponseCode); 
    }
    noCacahKramaMipil = "";
    delay(1000);
    http.end();
}

void readDataKrama() {
  MFRC522::StatusCode status;
  byte buffer[18] = {0};
  byte dataBlock[16];
  byte size = sizeof(buffer);
  byte startSector = 1;           //start write from this sector value
  byte blockNo = 0;               //start write from block 0
  byte endSector = 15;
  byte sector = startSector;
  readData = "";

  Serial.println("\nSTATUS : START READING....\n");
  Serial.println(readData);
  lcd.clear();
  lcd.setCursor(0,0);  
  lcd.print("Sedang Membaca");
  lcd.setCursor(0,1);
  lcd.print("Kartu Pintar");

  //Step 1. Authenticate using key A merupakan proses autentikasi key di sector trailer
  byte trailerBlock = (sector * 4) + 3; //rumus untuk mencari sector trailer karena sector trailer terdapat pada block 3 in each sector
  Serial.print(F("Authenticating using key A for sector "));
  Serial.println(sector);
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  memset(dataBlock, 0, sizeof(dataBlock));


  for (int i = startSector; i <= endSector; i++)
  {
    byte trailerBlock = (i * 4) + 3;
    Serial.print(F("Authenticating using key A for sector "));
    Serial.println(i);
    status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

    Serial.println(F("Current data in sector: "));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, i);
    Serial.println();

    //memset(buffer, 0 , strlen(buffer));
    for (int j = 0; j < 3; j++)
    {
      blockAddr = (i * 4) + j;
      status - (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
      String sTemp = String((char*)buffer);
      readData += sTemp.substring(0, 16);
    }
  }

  int readData_len = readData.length() + 1;
  char char_array[readData_len];
  readData.toCharArray(char_array, readData_len);

  Serial.println("Ini data char array: ");
  Serial.println(char_array);

  Serial.println("======== Identitas Krama =========");

  if(readData != "") {
    char delim[] = "#";
    char *pointer = strtok(char_array, delim);

    if (pointer != NULL) {
      Serial.print("No Cacah Krama Mipil: ");
      Serial.println(pointer);
      noCacahKramaMipil = pointer;
      Serial.println(noCacahKramaMipil);
      pointer = strtok(NULL, delim);
    }
    if (pointer != NULL) {
      Serial.print("NIK: ");
      Serial.println(pointer);
      pointer = strtok(NULL, delim);
    }
    if (pointer != NULL) {
      Serial.print("Nama Krama: ");
      Serial.println(pointer);
      pointer = strtok(NULL, delim);
    }
    if (pointer != NULL) {
      Serial.print("Jenis Kelamin: ");
      Serial.println(pointer);
      pointer = strtok(NULL, delim);
    }
    if (pointer != NULL) {
      Serial.print("Tempat Lahir: ");
      Serial.println(pointer);
      pointer = strtok(NULL, delim);
    }
    if (pointer != NULL) {
      Serial.print("Tanggal Lahir: ");
      Serial.println(pointer);
      pointer = strtok(NULL, delim);
    }
    if (pointer != NULL) {
      Serial.print("Alamat: ");
      Serial.println(pointer);
      pointer = strtok(NULL, delim);
    }
    fillPresensi(noCacahKramaMipil, "uidmisalnya");
  }
  else {
    lcd.clear();
    lcd.setCursor(0,0);  
    lcd.print("Kartu Kosong");
    Serial.println("Kartu Belum terisi data Krama");
  }  
  Serial.println("\n========= SELESAI ==========");


  //Halt PICC
  mfrc522.PICC_HaltA();
  //Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();

  count = 0;
}

int kramaCard() {
  //put your main code here, to run repeatedly
    if ( ! mfrc522.PICC_IsNewCardPresent()) {
      Serial.println("masuk atas");
      delay(50);
      return 0;
    }
      

    //select one of the card
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      Serial.println("masuk bawah");
      delay(50);
      return 0;
    }
      

    //Show some details of the PICC(that is: tje tag/card)
    Serial.print(F("Card UID: "));
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();

    Serial.print(F("PICC Type: "));
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.println(mfrc522.PICC_GetTypeName(piccType));
    return 1;
}

void dump_byte_array(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
