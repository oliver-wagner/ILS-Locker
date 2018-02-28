#include <ESP8266WiFi.h>

typedef struct {
  uint32_t crc32;
  uint32_t bootFlag;
  uint8_t channel;
  uint8_t ap_mac[6];
  uint8_t padding;
} rtcData;

rtcData rtcValue;

void setup() {
  Serial.begin(115200);
  delay(100);
  
  rtcValue.crc32 = 0;
  rtcValue.bootFlag = 0;
  rtcValue.channel = 0;
  memcpy(rtcValue.ap_mac, 0, 6 );
  rtcValue.padding = 0;
  ESP.rtcUserMemoryWrite( 0, (uint32_t*)&rtcValue, sizeof( rtcValue ) );
  ESP.rtcUserMemoryRead( 0, (uint32_t*)&rtcValue, sizeof( rtcValue ));
  Serial.println(rtcValue.bootFlag);
  Serial.println(rtcValue.crc32);
  Serial.println(rtcValue.channel);
  //Serial.println(rtcValue.ap_mac);
  Serial.println(rtcValue.padding);
}



void loop() {
  
}
