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

    long showModeDuration = 1500;
    long prevShowModeUntilMillis = 0;

    DEV_Thermostat() {
        thisCurrentHeatingCoolingState = new Characteristic::CurrentHeatingCoolingState();
        thisTargetHeatingCoolingState = new Characteristic::TargetHeatingCoolingState();
        thisCurrentTemperature = new Characteristic::CurrentTemperature();
        thisTargetTemperature = new Characteristic::TargetTemperature ();
        thisTemperatureDisplayUnits = new Characteristic::TemperatureDisplayUnits();

        
        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
            Serial.println(F("SSD1306 allocation failed"));
        }

        thisTargetTemperature->setVal(20);
        thisCurrentTemperature->setVal(30);
        thisTargetHeatingCoolingState->setValidValues(3, 0, 1, 2);
        thisTemperatureDisplayUnits->setVal(1);

        fanController = new SpanPoint(fanControllerMacAddress, sizeof(int), sizeof(int));

        pinMode(upPin, INPUT_PULLUP);
        pinMode(downPin, INPUT_PULLUP);
        pinMode(modePin, INPUT_PULLUP);
        
    }

    virtual void loop() override {
        int receivedData;
        if (fanController->get(&receivedData)) {
            Serial.printf("Received from fan controller: %i\n", receivedData);
        }
        readSwitches();

        updateDisplay();
    }

    boolean update() {
        if (thisTargetHeatingCoolingState->updated()) {
            Serial.print("Updated target heating cooling state to ");
            Serial.println(thisTargetHeatingCoolingState->getNewVal());
            prevShowModeUntilMillis = millis();
        }
        if (thisTargetTemperature->updated()) {
            Serial.print("Updated target temperature to ");
            Serial.println(thisTargetTemperature->getNewVal<float>());
            
            float toSend = thisTargetTemperature->getNewVal<float>();
            boolean success = fanController->send(&toSend);
            if (success) {
                Serial.print("Sent to fan controller float: ");
                Serial.println(toSend);
            } else {
                Serial.println("Failed to send to fan controller!");
            }

            thisCurrentTemperature->setVal<float>(thisTargetTemperature->getNewVal<float>() + 5);
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
            prevShowModeUntilMillis = millis();
            delay(debounceTime);
        } else {
            prevModeState = false;
        }

        if (thisCurrentHeatingCoolingState->getVal() == OFF) {
            return; // do not respond to button presses when system is off
        }

        readValue = digitalRead(upPin);
        if (readValue == LOW) {
            if (!prevUpState) {
                float newTemp = thisTargetTemperature->getVal<float>() + increment;
                thisTargetTemperature->setVal<float>(newTemp);
                Serial.printf("Up button pressed, new temp: %f\n", newTemp);
            }
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
        display.display();
    }
};