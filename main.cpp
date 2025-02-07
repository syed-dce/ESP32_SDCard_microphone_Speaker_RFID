#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <MFRC522.h>
#include "WiFi.h"
#include "Audio.h"
// RFID pins
#define SS_PIN 34
#define RST_PIN 21
#define RFID_SCK 36
#define RFID_MISO 37
#define RFID_MOSI 35
// SD card pins
#define SD_SCK 12
#define SD_MISO 13
#define SD_MOSI 11
#define SD_CS 10
// MAX 98357A
#define I2S_DOUT      7
#define I2S_BCLK      8
#define I2S_LRC       6
// Microphone INMP441
#define INMP_SD 		  38	
#define INMP_SCK 	    39	
#define INMP_WS		    40	
// #define IMP_LR		GND	x

/*
  If below mp3 file is present in SD card - then play the mp3 from SD Card.
  If below mp3 file is absent from SD card - then stream mp3 directly from cloud.
*/
char *mp3_file_in_sd_card = "/ce.mp3";

/*
  Play mp3 from cloud if above mp3 file not present in SD card
*/
char *mp3_on_cloud = "https://arabian-nights.s3.ap-south-1.amazonaws.com/Alibabaffmpeg.mp3";

/*
  Wifi Credentials
*/
String ssid =     "vivo_Y12s";
String password = "panther476";

/*
  mode = 0 by default
  mode = 1 when playing mp3 from SD card
*/
int app_mode = 0;

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key; 

uint8_t nuidPICC[4];

Audio audio;

void readFile(fs::FS &fs, const char *path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}


void SD_card_invoke() {
  
  SPI.end();
  delay(200);
  
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  // SPI.begin(sck, miso, mosi, cs);
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }

  // Serial.println("======================== Card mount success ========================");

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

     if(SD.exists(mp3_file_in_sd_card)){
      // mp3 present just play it
      Serial.println("\nINFO: mp3 present. Playing from SD Card ... \n");
      audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
      audio.setVolume(21); // default 0...21
      audio.connecttoFS(SD, mp3_file_in_sd_card);
      app_mode = 1;
    }
    else {
      // mp3 absent
      Serial.println("\nINFO: mp3 absent. Streaming from AWS ... \n");
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) delay(1500);
      audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
      audio.connecttohost(mp3_on_cloud); 
      app_mode = 1;
    }

    if ( app_mode == 0 ){
      RFID_init();
    }

}


void RFID_init() {
  
  SPI.end();
  delay(200);
  
  // RFID  initializations
  SPI.begin( RFID_SCK, RFID_MISO, RFID_MOSI, SS_PIN);
  // SPI.begin(14, 12, 13, 21);
  rfid.PCD_Init(); // Init MFRC522 
  for (uint8_t i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  RFID_init();
}

void loop() {

  switch(app_mode){
    case 1:
      audio.loop();
      break;
    case 0:
      // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
      if ( ! rfid.PICC_IsNewCardPresent())
        return;
      // Verify if the NUID has been readed
      if ( ! rfid.PICC_ReadCardSerial())
        return;
      Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
      Serial.println(rfid.PICC_GetTypeName(piccType));

      // Check is the PICC of Classic MIFARE type
      if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
        piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
        piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println(F("Your tag is not of type MIFARE Classic."));
        return;
      }

      if (rfid.uid.uidByte[0] != nuidPICC[0] || 
        rfid.uid.uidByte[1] != nuidPICC[1] || 
        rfid.uid.uidByte[2] != nuidPICC[2] || 
        rfid.uid.uidByte[3] != nuidPICC[3] ) {
        Serial.println(F("A new card has been detected."));

        // Store NUID into nuidPICC array
        for (uint8_t i = 0; i < 4; i++) {
          nuidPICC[i] = rfid.uid.uidByte[i];
        }
      
        Serial.println(F("The NUID tag is:"));
        Serial.print(F("In hex: "));
        printHex(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();
        Serial.print(F("In dec: "));
        printDec(rfid.uid.uidByte, rfid.uid.size);
        Serial.println();

        SD_card_invoke();

      }
      else Serial.println(F("Use a different tag"));
      // Halt PICC
      rfid.PICC_HaltA();
      // Stop encryption on PCD
      rfid.PCD_StopCrypto1();
      break;
  } // switch ends
} // loop ends

void printHex(uint8_t *buffer, uint8_t bufferSize) {
  for (uint8_t i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void printDec(uint8_t *buffer, uint8_t bufferSize) {
  for (uint8_t i = 0; i < bufferSize; i++) {
    Serial.print(' ');
    Serial.print(buffer[i], DEC);
  }
}

void audio_info(const char *info){
    // Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
    RFID_init();            // back to polling for RFID
    app_mode = 0;           // back to polling for RFID
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
}
void audio_showstreamtitle(const char *info){
    Serial.print("streamtitle ");Serial.println(info);
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}
void audio_eof_speech(const char *info){
    Serial.print("eof_speech  ");Serial.println(info);
}