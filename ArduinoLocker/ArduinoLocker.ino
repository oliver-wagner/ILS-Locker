#include <ESP8266WiFi.h>
#include <Servo.h>

#define SERVO_PIN 14
const char* ssid     = "VP3 Ballers 2.4";
const char* password = "AskHarsh!";
const char* host = "192.168.200.123";
const int httpPort = 8000;
String url;
int batteryLevel;
//unsigned int *bootCount;
boolean lockerAssigned = false;
boolean validResponse = false;
String response;

Servo servo;
WiFiClient client; 

void connect_wifi() {
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  unsigned long wifiConnectStart = millis();
  
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
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());
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
        Serial.print(response);
      }
      
      Serial.println();
      Serial.println("closing connection");

      return response;
}

void setup() {

  
  Serial.begin(115200);
  delay(100);

  connect_wifi();

  servo.attach(SERVO_PIN);

  batteryLevel = 10;

  //ESP.rtcUserMemoryRead(0, (unsigned int*) &bootCount, sizeof(bootCount));
  //++bootCount;
  Serial.print("Boot number: ");
  //Serial.println(*bootCount);


  Serial.print("connecting to ");
  Serial.println(host);

  url = "/ILSA/lockers/arduino/" + WiFi.macAddress() + "/" + batteryLevel + "/";
  
  if (1 == 1){
    while (!lockerAssigned){
      response = send_message(url);
      if (response == "No Locker"){
        delay(10000);    
      } else {
        lockerAssigned = true;
      }
    } 
  } else {
      response = send_message(url);
      Serial.print("response: ");
      Serial.println(response);
      while (!validResponse) {
        if (response == "Unlock"){
          //turn servo, wait 5 seconds, turn servo back
          servo.write(90);
          delay(5000);
          servo.write(0);
          //ESP.rtcUserMemoryWrite(0, (unsigned int*) &bootCount, sizeof(bootCount));
          //ESP.deepSleep(0);
        } else if (response == "Lock") {
          Serial.println("Locker is not opening");
          //ESP.rtcUserMemoryWrite(0, (unsigned int*) &bootCount, sizeof(bootCount));
          //ESP.deepSleep(0);
        } else if (response == "Sleep") {
          Serial.println("Locker is low on battery and going to sleep");
          //ESP.rtcUserMemoryWrite(0, (unsigned int*) &bootCount, sizeof(bootCount));
          //ESP.deepSleep(0);
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

