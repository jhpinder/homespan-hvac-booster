#include <HomeSpan.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>

#define MOCK 0

Adafruit_SSD1306 display(128, 32, &Wire, -1);

enum Modes {
    OFF = 0,
    HEATING = 1,
    COOLING = 2
};

struct DEV_Thermostat : Service::Thermostat {
    SpanCharacteristic *thisCurrentHeatingCoolingState;
    SpanCharacteristic *thisTargetHeatingCoolingState;
    SpanCharacteristic *thisCurrentTemperature;
    SpanCharacteristic *thisTargetTemperature;
    SpanCharacteristic *thisTemperatureDisplayUnits;
    SpanCharacteristic *thisCurrentRelativeHumidity;

    uint32_t calibrateTime = 0;
    const float increment = (float) 5 / (float) 9;
    const char* fanControllerMacAddress = "C8:2E:18:26:AE:84";
    SpanPoint *fanController;

    const int upPin = GPIO_NUM_13;
    const int downPin = GPIO_NUM_12;
    const int modePin = GPIO_NUM_14;
    const int debounceTime = 200;

    bool prevUpState = false;
    bool prevDownState = false;
    bool prevModeState = false;

    unsigned long showModeDuration = 1500;
    unsigned long prevShowModeUntilMillis;

    unsigned long dimTimeout = 30000;
    unsigned long prevDimMillis = 0;

    DHTesp tempSensor;
    uint16_t tempSensorPin = GPIO_NUM_25;
    unsigned long tempDelay = 5000;
    unsigned long lastTempReadTime;
    float maxTempDelta = (float) 3.0;
    const long minFanSpeed = 10;

    float mockTemp = (float) 10;
    float mockTempIncrement = (float) 5 / (float) 9;

    DEV_Thermostat() {
        thisCurrentHeatingCoolingState = new Characteristic::CurrentHeatingCoolingState();
        thisTargetHeatingCoolingState = new Characteristic::TargetHeatingCoolingState();
        thisCurrentTemperature = new Characteristic::CurrentTemperature();
        thisTargetTemperature = new Characteristic::TargetTemperature ();
        thisTemperatureDisplayUnits = new Characteristic::TemperatureDisplayUnits();
        thisCurrentRelativeHumidity = new Characteristic::CurrentRelativeHumidity();
        
        if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            Serial.println(F("SSD1306 allocation failed"));
        }

        thisTargetTemperature->setVal((float) 22.222);
        thisTargetHeatingCoolingState->setValidValues(3, 0, 1, 2);
        thisTemperatureDisplayUnits->setVal(1);

        fanController = new SpanPoint(fanControllerMacAddress, sizeof(float), sizeof(int));

        tempSensor.setup(tempSensorPin, DHTesp::DHT22); 
        pinMode(GPIO_NUM_26, OUTPUT);
        pinMode(GPIO_NUM_33, OUTPUT);
        digitalWrite(GPIO_NUM_26, HIGH);
        digitalWrite(GPIO_NUM_33, LOW);

        pinMode(upPin, INPUT_PULLUP);
        pinMode(downPin, INPUT_PULLUP);
        pinMode(modePin, INPUT_PULLUP);
        
    }

    virtual void loop() override {
        int receivedData;
        if (fanController->get(&receivedData)) {
            Serial.printf("Received from fan controller: %i\n", receivedData);
        }
        readTempSensor();
        readSwitches();
        updateDisplay();
    }

    boolean update() {
        if (thisTargetHeatingCoolingState->updated()) {
            prevShowModeUntilMillis = millis();
        }
        if (thisTargetTemperature->updated()) {
            Serial.print("Updated target temperature to ");
            Serial.println(thisTargetTemperature->getNewVal<float>());

            if (MOCK) {
                thisCurrentTemperature->setVal<float>(thisTargetTemperature->getNewVal<float>() + 5);
            }
        }
        return true;
    }

    void readSwitches() {
        int readValue = digitalRead(modePin);
        if (readValue == LOW) {
            if (!prevModeState) {
                Serial.println("Mode button pressed");
                if (thisTargetHeatingCoolingState->getVal() == 2) {
                    thisTargetHeatingCoolingState->setVal(0);
                } else {
                    thisTargetHeatingCoolingState->setVal(thisTargetHeatingCoolingState->getVal() + 1);
                }            
            }
            prevModeState = true;
            prevShowModeUntilMillis = prevDimMillis = millis();
            delay(debounceTime);
        } else {
            prevModeState = false;
        }

        if (thisTargetHeatingCoolingState->getVal() == OFF) {
            return; // do not respond to temperature set button presses when system is off
        }

        readValue = digitalRead(upPin);
        if (readValue == LOW) {
            if (!prevUpState) {
                float newTemp = thisTargetTemperature->getVal<float>() + increment;
                thisTargetTemperature->setVal<float>(newTemp);
                Serial.printf("Up button pressed, new temp: %f\n", newTemp);
            }
            prevDimMillis = millis();
            prevUpState = true;
            delay(debounceTime);
        } else {
            prevUpState = false;
        }

        readValue = digitalRead(downPin);
        if (readValue == LOW) {
            if (!prevDownState) {
                float newTemp = thisTargetTemperature->getVal<float>() - increment;
                Serial.printf("Down button pressed, new temp: %f\n", newTemp);
                thisTargetTemperature->setVal<float>(newTemp);
            }
            prevDimMillis = millis();
            prevDownState = true;
            delay(debounceTime);
        } else {
            prevDownState = false;
        }
    }

    void updateDisplay() {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);

        if (millis() - prevShowModeUntilMillis < showModeDuration) {
            display.setTextSize(2);
            display.setCursor(25,14);
            const char* toDisplay;
            switch (thisTargetHeatingCoolingState->getVal()) {
            case OFF:
            default:
                toDisplay = "FAN OFF";
                break;
            case HEATING:
                toDisplay = "HEATING";
                break;
            case COOLING:
                toDisplay = "COOLING";
                break;
            }
            display.print(toDisplay);
            display.display();
            return;
        }

        int currTemp = round(thisCurrentTemperature->getVal<float>() * 1.8 + 32);
        int setTemp = round(thisTargetTemperature->getVal<float>() * 1.8 + 32);
        display.setTextSize(4);
        display.setCursor(0, 4);
        display.print(currTemp);
        display.drawCircle(52, 5, 3, SSD1306_WHITE);

        if (thisTargetHeatingCoolingState->getVal() == OFF) {
            display.setTextSize(2);
            display.setCursor(85, 2);     
            display.print(F("FAN"));
            display.setCursor(85, 18);     
            display.print(F("OFF"));
            display.display();
        } else {
            display.setCursor(80, 4);
            display.setTextSize(1);
            display.print(F("Set Temp"));
            display.setTextSize(2);
            display.setCursor(90, 16);
            display.print(setTemp);
            display.setTextSize(1);
            display.print(F("o"));
        }

        display.dim(millis() - prevDimMillis > dimTimeout) ;
        display.display();
    }

    void readTempSensor() {
        if (millis() - lastTempReadTime > tempDelay) {
            Serial.println("Reading temp and humidity");

            TempAndHumidity tempAndHumidity = tempSensor.getTempAndHumidity();
            if (isnan(tempAndHumidity.humidity) || isnan(tempAndHumidity.temperature)) {
                Serial.println("Failed to read from DHT sensor!");
            } else {
                float temp = tempAndHumidity.temperature * (float) 9.0 / (float) 5.0 + (float) 32.0;
                Serial.printf("Temp: %fF\nHumidity: %f%\n", temp, tempAndHumidity.humidity);
                thisCurrentTemperature->setVal(tempAndHumidity.temperature);
                thisCurrentRelativeHumidity->setVal(tempAndHumidity.humidity);
            }

            lastTempReadTime = millis();
            sendFanSpeed();

            if (MOCK) {
                if (mockTemp > 30) {
                    mockTemp = 10;
                } else {
                    mockTemp += mockTempIncrement;
                }   
                thisCurrentTemperature->setVal(mockTemp);     
            }
        }
    }

    void sendFanSpeed() {
        float toSend;
        switch (thisTargetHeatingCoolingState->getVal()) {

            case OFF:
            default:
                toSend = 0;
                break;

            case COOLING:

                toSend = mapFloat(
                    thisCurrentTemperature->getVal(),
                    thisTargetTemperature->getVal(),
                    thisTargetTemperature->getVal() + maxTempDelta,
                    minFanSpeed,
                    100
                    );

                break;

            case HEATING:

                toSend = mapFloat(
                    thisCurrentTemperature->getVal(),
                    thisTargetTemperature->getVal(),
                    thisTargetTemperature->getVal() - maxTempDelta,
                    minFanSpeed,
                    100
                    );
 
                break;
        }

        Serial.printf("Sending fan speed %f to fan controller...\n", toSend);
        boolean success = fanController->send(&toSend);
        if (success) {
            Serial.printf("Sent to fan controller fan speed: %f\n", toSend);
        } else {
            Serial.println("Failed to send to fan controller!");
        }
    }

    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
        const float run = in_max - in_min;
        const float rise = out_max - out_min;
        const float delta = x - in_min;
        return (delta * rise) / run + out_min;
    }
};