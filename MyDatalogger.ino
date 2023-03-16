/*
  SD Serial Logger

  Log messages incoming on Serial1 to an SD card.
  We use the Arduino MKR GSM 1400, but other arduinos will also work.

  created  Mar 16 2023
  by Jeremy Côté
*/

#include <SPI.h>
#include <SD.h>

const int chipSelect = 21;
const int logButton = 6; // Button to enable/disable logging
const int light = 5; // Signal light

File dataFile;
bool fileMounted = false;

void setup() {
  // Setup pins
  pinMode(chipSelect, OUTPUT);
  pinMode(logButton, INPUT);
  pinMode(light, OUTPUT);

  // Open serial communications
  Serial.begin(115200);
  Serial1.begin(115200);

  Serial.println("UO Supermileage Datalogger");
}

void mount() {
  digitalWrite(light, HIGH);

  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
      digitalWrite(light, LOW);
      fileMounted = false;
      return;
  }

  Serial.println("card initialized.");  

  String filename = createFilename();

  Serial.println("Opening file " + filename);
  dataFile = SD.open(filename, FILE_WRITE);
        
  if (dataFile) {
    // Successfully opened file
    Serial.println("Mounted file");

    // Create header
    dataFile.println("id,value");
    fileMounted = true;
  } else {
    Serial.println("Failed to open file");
    fileMounted = false;
    digitalWrite(light, LOW);
  }    
}

void unmount() {

  if (fileMounted) {
    dataFile.close();
    SD.end();
  }
  
  fileMounted = false;
  digitalWrite(light, LOW);
}

String createFilename() {
  String filename = "logs00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[4] = i/10 + '0';
    filename[5] = i%10 + '0';
    if (! SD.exists(filename)) {
      break;  // leave the loop!
    }
  }
  return filename;
}

void loop() {
  // If button pressed (We use a switch)
  if (digitalRead(logButton) == HIGH) {
    if (fileMounted) {
      // if the file is available, write to it:
      if (dataFile) {
        // If messages in Serial1
        if (Serial1.available() > 0) {
          
          // Turn on light to signal write in progress
          digitalWrite(light, HIGH);
          
          // Get line from serial
          String line = Serial1.readStringUntil('\n');
          
          // Write to SD Card
          dataFile.println(line);

          // Print to Serial too
          Serial.println("\r\nWriting: " +line);

          // Turn off light to signal write complete
          digitalWrite(light, LOW);
        }
      }
      // if the file isn't open, pop up an error:
      else {
        Serial.println("error opening csv file...");
      }
    } else {
      mount();  
    }
  } else if (fileMounted) {
      unmount();
  }
}









