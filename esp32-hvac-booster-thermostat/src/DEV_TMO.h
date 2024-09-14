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

    DEV_Thermostat() {
        thisCurrentHeatingCoolingState = new Characteristic::CurrentHeatingCoolingState();
        thisTargetHeatingCoolingState = new Characteristic::TargetHeatingCoolingState();
        thisCurrentTemperature = new Characteristic::CurrentTemperature();
        thisTargetTemperature = new Characteristic::TargetTemperature ();
        thisTemperatureDisplayUnits = new Characteristic::TemperatureDisplayUnits();

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
    }

    boolean update() {
        if (thisTargetHeatingCoolingState->updated()) {
            Serial.print("Updated target heating cooling state to ");
            Serial.println(thisTargetHeatingCoolingState->getNewVal());
        }
        if (thisTargetTemperature->updated()) {
            Serial.print("Updated target temperature to ");
            Serial.println(thisTargetTemperature->getNewVal());
            
            float toSend = thisTargetTemperature->getNewVal();
            boolean success = fanController->send(&toSend);
            if (success) {
                Serial.print("Sent to fan controller float: ");
                Serial.println(toSend);
            } else {
                Serial.println("Failed to send to fan controller!");
            }

            
            thisCurrentTemperature->setVal(thisTargetTemperature->getNewVal() + 5);
        }
        return true;
    }

    void readSwitches() {
        int readValue = digitalRead(upPin);
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

        readValue = digitalRead(modePin);
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
            delay(debounceTime);
        } else {
            prevModeState = false;
        }
    }
};