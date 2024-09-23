#include <Arduino.h>
#include <DEV_TMO.h>

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.printf("\n\nThis is the thermostat with MAC Address = %s\n",WiFi.macAddress().c_str());
  
  homeSpan.begin(Category::Thermostats,"JP Thermostat");

  new SpanAccessory();
    new Service::AccessoryInformation();
      new Characteristic::Identify();
      new Characteristic::Name("JP Thermostat");
    new DEV_Thermostat();

  homeSpan.setLogLevel(0);
}

void loop() {
  homeSpan.poll();
}