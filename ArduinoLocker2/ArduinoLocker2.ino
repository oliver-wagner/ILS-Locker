#include <ESP8266WiFi.h>
#include "Servo.h"

#define SERVO_PIN 12
#define LED_PIN_RED 13
#define LED_PIN_GREEN 15

typedef struct {
  uint32_t crc32;
  uint32_t bootFlag;
  uint8_t channel;
  uint8_t ap_mac[6];
  uint8_t padding;
} rtcData;

rtcData rtcValue;

const char* ssid     = "ILSANetwork";
const char* password = "SwypeD@ddy1";
const char* host = "192.168.1.1";
const int httpPort = 8000;
String url;
int batteryLevel;
boolean lockerAssigned = false;
boolean validResponse = false;
String response;
bool rtcValid = false;
int pos = 0;

Servo servo;
WiFiClient client; 

void connect_wifi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  if( rtcValid ) {
    // The RTC data was good, make a quick connection
    WiFi.begin(ssid, password, rtcValue.channel, rtcValue.ap_mac, true);
  }
  else {
    // The RTC data was not valid, so make a regular connection
    WiFi.begin(ssid, password);
  }
  
  unsigned long wifiConnectStart = millis();
  while( WiFi.status() != WL_CONNECTED ) {
    Serial.println("Connecting to Wifi");
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WiFi. Please verify credentials: ");
      delay(10000);
      ESP.deepSleep(0);
    }
    delay(500);
    Serial.println("...");
    // Only try for 5 seconds.
    if (millis() - wifiConnectStart > 15000) {
      Serial.println("Failed to connect to WiFi");
      ESP.deepSleep(0);
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC address: ");
  Serial.println(WiFi.macAddress());
}

uint32_t calculateCRC32( const uint8_t *data, size_t length ) {
  uint32_t crc = 0xffffffff;
  while( length-- ) {
    uint8_t c = *data++;
    for( uint32_t i = 0x80; i > 0; i >>= 1 ) {
      bool bit = crc & 0x80000000;
      if( c & i ) {
        bit = !bit;
      }

      crc <<= 1;
      if( bit ) {
        crc ^= 0x04c11db7;
      }
    }
  }

  return crc;
}

void write_RTC() {
  rtcValue.channel = WiFi.channel();
  memcpy( rtcValue.ap_mac, WiFi.BSSID(), 6 ); // Copy 6 bytes of BSSID (AP's MAC address)
  rtcValue.crc32 = calculateCRC32( ((uint8_t*)&rtcValue) + 4, sizeof( rtcValue ) - 4 );
  ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcValue, sizeof( rtcValue ) );
}

int battery_level() {
 
  // read the battery level from the ESP8266 analog in pin.
  // analog read level is 10 bit 0-1023 (0V-1V).
  // our 1M & 220K voltage divider takes the max
  // lipo value of 4.2V and drops it to 0.758V max.
  // this means our min analog read value should be 580 (3.14V)
  // and the max analog read value should be 774 (4.2V).
  int level = analogRead(A0);
 
  // convert battery level to percent
  level = map(level, 580, 774, 0, 100);
  Serial.print("Battery level: "); Serial.print(level); Serial.println("%");
  return level;
}

String send_message(String url) {
  if (!client.connect(host, httpPort)) {
        Serial.println("Connection failed");
        return "Connection Failed";
      }
       // We now create a URI for the request
      Serial.print("Requesting URL: ");
      Serial.println(url);
  
      // This will send the request to the server
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" + 
                  "Connection: close\r\n\r\n");
      delay(500);
  
      // Read all the lines of the reply from server and print them to Serial
      while(client.available()){
        response = client.readStringUntil('\r');
      }
      
      Serial.println();
      Serial.println("closing connection");

      return response;
}

void setup() {

  Serial.begin(115200);
  delay(100);

  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_GREEN, OUTPUT);
  
  // Try to read WiFi settings from RTC memory
  if( ESP.rtcUserMemoryRead( 0, (uint32_t*)&rtcValue, sizeof( rtcValue ) ) ) {
    // Calculate the CRC of what we just read from RTC memory, but skip the first 4 bytes as that's the checksum itself.
    uint32_t crc = calculateCRC32( ((uint8_t*)&rtcValue) + 4, sizeof( rtcValue ) - 4 );
    if( crc == rtcValue.crc32 ) {
      rtcValid = true;
    }
  }

  connect_wifi();

  servo.attach(SERVO_PIN);

  batteryLevel = battery_level();

  Serial.print("Boot flag: ");
  Serial.println(rtcValue.bootFlag);

  Serial.print("Connecting to ");
  Serial.println(host);

  url = "/ILSA/lockers/arduino/" + WiFi.macAddress() + "/" + batteryLevel + "/";
  if (rtcValue.bootFlag != 2723){
    while (!lockerAssigned){
      response = send_message(url);
      Serial.println(response);
      if (response == "\nNo Locker") {
        delay(1200000);    
      } else {
        lockerAssigned = true;
        rtcValue.bootFlag = 2723;
        write_RTC();
        ESP.deepSleep(0);
      }
    } 
  } else {
      response = send_message(url);
      Serial.print("response: ");
      Serial.println(response);
      while (!validResponse) {
        if (response == "\nUnlock") {
          //turn servo, wait 5 seconds, turn servo back
          for (pos = 80; pos <= 180; pos += 1) {
            servo.write(pos);
            delay(15); 
          }
          digitalWrite(LED_PIN_GREEN, HIGH);
          delay(10000);
          digitalWrite(LED_PIN_GREEN, LOW);
          for (pos = 180; pos >= 8 0; pos -= 1) {
            servo.write(pos);
            delay(15); 
          }
          write_RTC();
          ESP.deepSleep(0);
        } else if (response == "\nLock") {
          Serial.println("Locker is not opening");
          digitalWrite(LED_PIN_RED, HIGH);
          delay(3000);
          digitalWrite(LED_PIN_RED, LOW);
          write_RTC();
          ESP.deepSleep(0);
        } else if (response == "\nSleep") {
          Serial.println("Locker is low on battery and going to sleep");
          digitalWrite(LED_PIN_RED, HIGH);
          delay(3000);
          digitalWrite(LED_PIN_RED, LOW);
          write_RTC();
          ESP.deepSleep(0);
        } else if (response == "\nNo Locker") {
          Serial.println("No locker setup in database. Going to sleep");
          for (int i = 0; i < 5; i += 1) {
            digitalWrite(LED_PIN_RED, HIGH);
            delay(1000);
            digitalWrite(LED_PIN_RED, LOW);
          }
          write_RTC();
          ESP.deepSleep(0);
        } else {
          Serial.println("Unknown response");
          Serial.println("Go to sleep");
          for (int i = 0; i < 5; i += 1) {
            digitalWrite(LED_PIN_RED, HIGH);
            delay(250);
            digitalWrite(LED_PIN_RED, LOW);
            delay(250);
          }
          ESP.deepSleep(0);
        }
      }
   }
}

void loop() {
}

