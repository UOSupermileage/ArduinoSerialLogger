/*
  SD card datalogger

  This example shows how to log data from three analog sensors
  to an SD card using the SD library.

  The circuit:
   analog sensors on analog ins 0, 1, and 2
   SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4 (for MKRZero SD: SDCARD_SS_PIN)

  created  24 Nov 2010
  modified 9 Apr 2012
  by Tom Igoe

  This example code is in the public domain.

*/

#include <SPI.h>
#include <SD.h>

const int chipSelect = 21;
const int logButton = 6;
const int light = 5;

File dataFile;
bool fileMounted = false;

void setup() {

  pinMode(chipSelect, OUTPUT);
  pinMode(logButton, INPUT);
  pinMode(light, OUTPUT);


  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  digitalWrite(light, HIGH);

  Serial1.begin(115200);

  while (!Serial) {
    ;
  }

  digitalWrite(light, LOW);

  Serial.println("UO Supermileage Datalogger");
}

void loop() {

  if (digitalRead(logButton) == HIGH) {
    if (fileMounted) {
      // if the file is available, write to it:
      if (dataFile) {
        if (Serial1.available() > 0) {
          digitalWrite(light, HIGH);
          String line = Serial1.readStringUntil('\n');
          dataFile.println(line);
          // print to the serial port too:
          Serial.println("\r\nWriting: " +line);
          digitalWrite(light, LOW);
        } else {
          Serial.print("-");
        }
      }
      // if the file isn't open, pop up an error:
      else {
        Serial.println("error opening datalog.csv");
      }
    } else {
      Serial.print("Initializing SD card...");

      // see if the card is present and can be initialized:
      if (!SD.begin(chipSelect)) {
        Serial.println("Card failed, or not present");
        // don't do anything more:
        fileMounted = false;
      } else {
        Serial.println("card initialized.");  
        Serial.println("Mounting file");

        String filename = "logs00.CSV";
        for (uint8_t i = 0; i < 100; i++) {
          filename[4] = i/10 + '0';
          filename[5] = i%10 + '0';
          if (! SD.exists(filename)) {
            break;  // leave the loop!
          }
        }

        Serial.println("Saving to " + filename);
        dataFile = SD.open(filename, FILE_WRITE);
        
        if (! dataFile) {
          Serial.println("Failed to mount file");
        } else {
          Serial.println("Mounted file");
          dataFile.println("id,value");

          fileMounted = true;
        }
      }
    }
  } else if (fileMounted) {
      dataFile.close();
      SD.end();
      fileMounted = false;
  } else {
    Serial.println("Waiting...");
  }
}









