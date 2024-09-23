# ESP32 HVAC Booster Fan Control System

### Problem:
Many houses built in the 1960s were designed without air conditioning but included ducts for central heat. Some of these ducts run through the exterior walls to supply second story rooms. The result is less CFM is supplied to the second story than the first story. In the winter, this is not an issue due to the nature of heating two stories. However in the summer this becomes a large issue with cooling. Left untreated this can result in a temperature difference of over 4 degrees F on hot days.

### Solution:

Without running new ducts, there are two solutions:

1. Decrease airflow to the lower level registers
2. Increase airflow to the upper level registers

Option 1 is simpler but it can cause the air handler blower to overheat and run at reduced capacity. It also decreases efficiency overall and increases costs.

Option 2 is more complicated but it allows the air handler blower to function at full capacity without any issues. For each of the four ducts that run to bedrooms on the second story, an inline booster fan is installed. The booster fans must be able to increase airflow when called for and also not restrict airflow from the air handler.


## Thermostat
### Hardware

- ESP-WROOM-32
- DHT22/AM2302
- 0.91 Inch I2C 128x32 OLED

### Implementation

The thermostat is based on the HomeSpan library for the ESP32. This enables the thermostat to connect to HomeKit for automation and remote access. Using the DHT22/AM2302 is optimal due to its simplicity as well as its inclusion of a humidity sensor in addition to a temperature sensor.

Status is displayed on a 0.91 Inch I2C 128x32 OLED display, which dims after 30 seconds of no button presses. It displays current temperature, set temperature, and mode.

HomeSpan has a `SpanPoint` object for communication between ESP32 boards without the requirement for a centralized wifi network. The thermostat receives a heartbeat/sync int message from the fan controller to determine the correct wireless channel to use for reliability. The thermostat sends a float message to the fan controller every time the thermometer is updated (5 second interval);

## Fan Controller
### Hardware

- AC Infinity CLOUDLINE PRO S6
- ESP-WROOM-32
- L298N

### Implementation

AC Infinity CLOUDLINE PRO S6 is controlled by a 10VDC PWM signal. The ESP32 supplies 3.3VDC so an L298N is required to pull up and down the signal to the 10VDC level. When a duty cycle of 100% is applied, the fan is turned off. When a 0% duty cycle is applied, the fan runs at 100% speed.

The ESP32 is a `HomeSpan` `SpanPoint` node and receives a float from the thermostat every time the fan speed is changed. It constrains the values from 0 to the max PWM value based on the bit depth and reverses the duty cycle.