#include <ESP8266WiFi.h>

extern "C" {
  #include "user_interface.h"
}

const char* ssid     = "yourssid";
const char* password = "yourpassword";
const char* host = "wifitest.adafruit.com";
byte bootCount = 0;
int batteryLevel;

WiFiClientSecure wifiClient; 


void connect_wifi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WiFi. Please verify credentials: ");
      delay(10000);
    }
    delay(500);
    Serial.println("...");
    if (millis() - wifiConnectStart > 15000) {
      Serial.println("Failed to connect to WiFi");
      return;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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


void setup() {
  
  Serial.begin(115200);
  delay(100);

  connect_wifi();

  batteryLevel = battery_level();

  if (batteryLevel < 620){
    //add to next transmission that locker needs to be shut down
  }
  
  
  ++bootCount;
  ESP.rtcUserMemoryRead(0, bootCount, sizeof(bootCount));
  Serial.println("Boot number: " + String(bootCount));
  
  if ((int) bootCount == 1){
    // Do http get with mac address until valid response from pi registering to locker
    
  }

  
  //Locker button was pressed, send get, parse, do i open?

  //turn servo, wait 5 seconds, turn servo back
  //stay closed
  
 



 


 ESP.rtcUserMemoryWrite(0, bootCount, sizeof(bootCount))
 //ESP.deepSleep(0);
}
 




























 delay(5000);
  ++value;
 
  Serial.print("connecting to ");
  Serial.println(host);
  
  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }


  // We now create a URI for the request
  String url = "/testwifi/index.html";
  Serial.print("Requesting URL: ");
  Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
  delay(500);
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }
  
  Serial.println();
  Serial.println("closing connection");








 
void loop() {
}

