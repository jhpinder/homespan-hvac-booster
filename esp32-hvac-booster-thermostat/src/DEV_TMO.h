struct DEV_Thermostat : Service::Thermostat {
    SpanCharacteristic *thisCurrentHeatingCoolingState;
    SpanCharacteristic *thisTargetHeatingCoolingState;
    SpanCharacteristic *thisCurrentTemperature;
    SpanCharacteristic *thisTargetTemperature;
    SpanCharacteristic *thisTemperatureDisplayUnits;

    uint32_t calibrateTime = 0;
    const char* fanControllerMacAddress = "C8:2E:18:26:AE:84";
    SpanPoint *fanController;


    DEV_Thermostat() {
        thisCurrentHeatingCoolingState = new Characteristic::CurrentHeatingCoolingState();
        thisTargetHeatingCoolingState = new Characteristic::TargetHeatingCoolingState();
        thisCurrentTemperature = new Characteristic::CurrentTemperature();
        thisTargetTemperature = new Characteristic::TargetTemperature ();
        thisTemperatureDisplayUnits = new Characteristic::TemperatureDisplayUnits();

        thisTargetHeatingCoolingState->setValidValues(3, 0, 1, 2);
        thisTemperatureDisplayUnits->setVal(1);

        fanController = new SpanPoint(fanControllerMacAddress, sizeof(float), 0);
        
    }

    virtual void loop() override {
        int receivedData;
        if (fanController->get(&receivedData)) {
            Serial.printf("Received from fan controller: %i\n", receivedData);
        }
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
};