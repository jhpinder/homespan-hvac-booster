#include <Arduino.h>
#include <HomeSpan.h>

#define CALIBRATION_PERIOD 10

const float frequency = 1250.0f;
const int channel = 0;
const int bitDepth = 8;
const int maxPwmVal = (1 << bitDepth) - 1;
const uint8_t pinToUse = 15;

int dutyCycle = 100;

const char* thermostatMacAddress = "C8:2E:18:26:BE:00";
SpanPoint *thermostat;
uint32_t calibrateTime = 0;

void setup() {
  delay(2000);
  Serial.begin(115200);
  Serial.printf("\n\nThis is the fan controller with MAC Address = %s\n",WiFi.macAddress().c_str());

  thermostat = new SpanPoint(thermostatMacAddress, sizeof(int), sizeof(float));

  ledcSetup(channel, frequency, bitDepth);
  ledcAttachPin(pinToUse, channel);

  homeSpan.setLogLevel(0);
}

void loop() {
  float fanSpeed;
  if (thermostat->get(&fanSpeed)) {
    Serial.printf("Received float: %f\n", fanSpeed);
    dutyCycle = constrain(map(fanSpeed, 100, 0, 0, maxPwmVal), 0, maxPwmVal);
    ledcWrite(channel, dutyCycle);
    Serial.printf("Set duty cycle to: %i\n", dutyCycle);
  }

  if (millis() > calibrateTime) {
    int toSend = rand();
    boolean success = thermostat->send(&toSend);
    Serial.printf("Send int %d %s\n", toSend, success ? "Succeded" : "Failed");
    calibrateTime = millis() + CALIBRATION_PERIOD * 1000;
  }
}
