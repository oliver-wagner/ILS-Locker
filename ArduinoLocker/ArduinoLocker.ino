#include <ESP8266WiFi.h>
#include "Servo.h"

#define SERVO_PIN 14

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

Servo servo;
WiFiClient client; 

void connect_wifi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  if( rtcValid ) {
    // The RTC data was good, make a quick connection
    Serial.println("HERE");
    WiFi.begin(ssid, password, rtcValue.channel, rtcValue.ap_mac, true);
  }
  else {
    // The RTC data was not valid, so make a regular connection
  WiFi.begin(ssid, password);
  }
  
  int retries = 0;
  int wifiStatus = WiFi.status();
  while( wifiStatus != WL_CONNECTED ) {
    Serial.println("Failed to connect to WiFi");
    retries++;
    Serial.println("Failed to connect to WiFi");
    if( retries == 100 ) {
      // Quick connect is not working, reset WiFi and try regular connection
      WiFi.disconnect();
      delay( 10 );
      WiFi.forceSleepBegin();
      delay( 10 );
      WiFi.forceSleepWake();
      delay( 10 );
      WiFi.begin(ssid, password);
    }
    if( retries == 600 ) {
      // Giving up after 30 seconds and going back to sleep
      WiFi.disconnect( true );
      delay(1);
      WiFi.mode( WIFI_OFF );
      Serial.println("Failed to connect to WiFi. Please verify credentials: ");
      ESP.deepSleep( 1000 , WAKE_RF_DISABLED );
      return; // Not expecting this to be called, the previous call will never return.
    }
    delay( 500 );
    wifiStatus = WiFi.status();
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
  servo.write(90);

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
          Serial.println("Here");
          servo.write(0);
          delay(5000);
          Serial.println("Here1");
          servo.write(90);
          write_RTC();
          ESP.deepSleep(0);
        } else if (response == "\nLock") {
          Serial.println("Locker is not opening");
          write_RTC();
          ESP.deepSleep(0);
        } else if (response == "\nSleep") {
          Serial.println("Locker is low on battery and going to sleep");
          write_RTC();
          ESP.deepSleep(0);
        } else {
          Serial.println("Unknown response");
          Serial.println("Ask for retransmission");
          response = send_message(url);
        }
      }
   }
}
 
void loop() {
}

