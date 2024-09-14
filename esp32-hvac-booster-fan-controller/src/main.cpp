#include <Arduino.h>
#include <HomeSpan.h>

#define CALIBRATION_PERIOD   10

const char* thermostatMacAddress = "C8:2E:18:26:BE:00";
SpanPoint *thermostat;
uint32_t calibrateTime=0;

void setup() {
  delay(5000);
  Serial.begin(115200);
  Serial.printf("\n\nThis is the fan controller with MAC Address = %s\n",WiFi.macAddress().c_str());

  thermostat = new SpanPoint(thermostatMacAddress, sizeof(int), sizeof(int));

  homeSpan.setLogLevel(0);
}

void loop() {
  int fanSpeed;
  if(thermostat->get(&fanSpeed)){
    Serial.printf("Received float=%f\n",fanSpeed);
  }
  if(millis()>calibrateTime){
    int toSend=rand();
    boolean success = thermostat->send(&toSend);
    Serial.printf("Send %s\n",success?"Succeded":"Failed");
    calibrateTime=millis()+CALIBRATION_PERIOD*1000;
  }
}
