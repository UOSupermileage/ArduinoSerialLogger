/*
  SD Serial Logger

  Log messages incoming on Serial2 to an SD card.
  We use the Arduino UNO, but other arduinos will also work.

  created  Mar 16 2023
  by Jeremy Côté
*/

#include "esp_wpa2.h"
#include <SPI.h>
#include <SD.h>
#include <TimeLib.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define rxPin 16
#define txPin 17

const char* CACertificate =
"-----BEGIN CERTIFICATE-----\n"
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
"rqXRfboQnoZsG4q5WTP468SQvvG5\n"
"-----END CERTIFICATE-----\n";

const char* Certificate =
"-----BEGIN CERTIFICATE-----\n"
"-----END CERTIFICATE-----\n";

const char* ssid = "eduroam";
const char* password = "";
const char* serverName = "https://6a3blrx50f.execute-api.us-east-2.amazonaws.com/Production/ingest";
const char* awsKey = "<aws_api_key>";
const char* eap_identity = "jcote034@uottawa.ca";
const char* eap_username = "jcote034";
const char* eap_password = "<password>";
const char* test_id = "esp32-current-sensor-2";


const int chipSelect = 5;
const int logButton = 13; // Button to enable/disable logging
const int light = 0; // Signal light
const int sdDetect = 36;

File dataFile;
bool fileMounted = false;

String filename;

void setup() {
  // Setup pins
  pinMode(chipSelect, OUTPUT);
  pinMode(logButton, INPUT);
  pinMode(light, OUTPUT);
  pinMode(sdDetect, INPUT);

  // Open serial communications
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, rxPin, txPin);

  Serial.println("UO Supermileage Datalogger");

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)eap_identity, strlen(eap_identity));
  esp_wifi_sta_wpa2_ent_set_username((uint8_t *)eap_username, strlen(eap_username));
  esp_wifi_sta_wpa2_ent_set_password((uint8_t *)eap_password, strlen(eap_password));
  esp_wifi_sta_wpa2_ent_enable();

  WiFi.begin(ssid);

  Serial.println("Connecting");
  int i = 0;
  while(WiFi.status() != WL_CONNECTED && i < 5) {
    delay(500);
    Serial.print(".");
    i++;
  }
}

String createFilename() {
  String filename = "/logs000.csv";
  for (int i = 0; i < 1000; i++) {
    filename[5] = ((i/10) / 10) % 10 + '0';
    filename[6] = (i/10) % 10 + '0';
    filename[7] = i%10 + '0';
    Serial.println(filename);
    if (! SD.exists(filename)) {
      break;  // leave the loop!
    }
  }
  return filename;
}

void mount() {
  digitalWrite(light, HIGH);

  Serial.print("Initializing SD card...");

  if (digitalRead(sdDetect) == LOW) {
    Serial.println("No SD card in slot...");
    return;
  }

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
      digitalWrite(light, LOW);
      fileMounted = false;
      return;
  }

  Serial.println("card initialized.");  

  filename = createFilename();

  Serial.println("Opening file " + filename);
  dataFile = SD.open(filename, FILE_APPEND);
        
  if (dataFile) {
    // Successfully opened file
    Serial.println("Mounted file");
    dataFile.println("timestamp,throttle,speed,rpm,current,voltage,throttleTooHigh,motorInitializing,clockState,lastDeadman");
    fileMounted = true;
  } else {
    Serial.println("Failed to open file");
    fileMounted = false;
    digitalWrite(light, LOW);
  }
}

void unmount() {

  Serial.println("Unmounting SD Card");

  if (fileMounted) {
    dataFile.close();
    SD.end();
  }
  
  fileMounted = false;
  digitalWrite(light, LOW);
}

String items[50];
int itemsSize = 50;

int itemCount = 0;
int timeConfigured = 0;

void loop() {  

  // Display wifi connection and update time
  if (WiFi.status() == WL_CONNECTED && !timeConfigured) {
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    timeConfigured = 1;
  }

  // If switch is on
  if (digitalRead(logButton) == LOW) {
    
    // Mount file if not already done
    if (!fileMounted) {
      // Serial.println("SD Card disabled");
      mount();
    }

    // If messages in Serial2
    if (Serial2.available() > 0) {
      // Get line from serial
      String line = Serial2.readStringUntil('\n');
      line.trim();

      Serial.println(line);

      if (fileMounted) {
        // if the file is available, write to it:
        if (dataFile) {
            // Write to SD Card
            Serial.println("Writing to SD Card");
            dataFile.println(line);  
            dataFile.flush();
        }
        // if the file isn't open, pop up an error:
        else {
          Serial.println("error opening csv file...");
        }
      } else {
        Serial.println("SD Card not mounted...");
      }

      items[itemCount] = String();
      items[itemCount] += line;

      itemCount++;

      // If batch of itemsSize items is ready, post data to AWS
      if (itemCount >= itemsSize) {
        Serial.println("Creating output JSON string");
        
        String out = "{ \"messages\": \"";
        for (int i = 0; i < itemCount; i++) {
          out += items[i];
          if (i < itemCount - 1) {
            out += "|";
          }
        }
        out += "\", ";
        out += "\"test_id\": \"";
        out += test_id;
        out += "\"}";

        if(WiFi.status()== WL_CONNECTED){
          WiFiClientSecure *client = new WiFiClientSecure;

          if (client) {
            client->setCertificate(Certificate);
            client->setCACert(CACertificate);
            HTTPClient https;

            // Your Domain name with URL path or IP address with path
            https.begin(*client, serverName);
            
            // Specify content-type header
            https.addHeader("Content-Type", "application/json");
            https.addHeader("x-api-key", "3XIwCcPINm4djI3bCyT9j5rXcJ1m5IIk6O68R4uU", false, true);

            Serial.print("Output: ");
            Serial.println(out);

            // Data to send with HTTP POST
            int httpResponseCode = https.POST(out);

            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
              
            // Free resources
            https.end();
          } else {
            Serial.println("Failed to create client");
          }

          delete client;
        }
        else {
          Serial.println("WiFi Not Connected");
        }

        itemCount = 0;
      }
    }
  } else if (fileMounted) {
      unmount();
  }
}









