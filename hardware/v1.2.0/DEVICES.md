# Hardware v1.2.0 devices

## Changes since v1.1.0

The changes are:

- a JTAG connector has been added
- the RESET button has been exposed on the front of the board
- there is an improved power path to ensure that current to the peripherals is steady independently of whether power is from battery or usb
- there are changes to the board (reduced copper plane; air holes) to increase air flow and reduce the chance of heat from the board affecting the temperature sensor


## Board

Custom circuit board, see Schematic in separate file in this directory

* Integrated button connected to pin A1 of the esp32
* Integrated power regulator

## Microcontroller

Adafruit feather esp32 "HUZZAH32" (https://www.adafruit.com/product/3406)

* esp32-wroom-32 MoC, https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf
* onboard wifi for communication
* onboard usb serial for power and development
* onboard reset button
* onboard configured i2c for communication with peripherals
* onboard connectivity to external rechargeable LiPoly battery, charging from onboard usb

## JTAG connector

* The JTAG connector is connected to pins D8 (TCK), D7 (TDI), D4 (TDO), and D2 (TMS).

## Peripherals

### Screen

0.91" 128x32 OLED display @ I2C 0x3C (hardwired)

* https://protosupplies.com/product/oled-0-91-128x32-i2c-white-display/

### Air/gas sensor

DFRobot SKU:SEN0514 air/gas sensor @ I2C 0x53 (alternate 0x52, supposedly)

* https://wiki.dfrobot.com/SKU_SEN0514_Gravity_ENS160_Air_Quality_Sensor


### Environmental sensor

DFRobot SKU:SEN0500 environmental sensor @ I2C 0x22 (undocumented)

* https://wiki.dfrobot.com/SKU_SEN0500_Fermion_Multifunctional_Environmental_Sensor

### Movement sensor

DFRobot SKU:SEN0171 Digital passive IR sensor @ pin (see below)

* signal reads analog high after / while movement is detected, low when no movement is detected, no buffering of value.  Reading it digital would be fine too.
* https://www.dfrobot.com/product-1140.html
* https://wiki.dfrobot.com/PIR_Motion_Sensor_V1.0_SKU_SEN0171
* HW 1.1.0 and HW 1.2.0: The PIR is connected to pin A2.
* HW 1.0.0: The PIR is connected to pin A4.

### Sound sensor

DFRobot SKU:SEN0487 MEMS microphone, analog signal (see below)

* unit and range of signal not documented beyond `analogRead()` returning an `uint16_t`.  From testing, it looks like it returns a reading mostly in the range 1500-2500, corresponding to 1.5V-2.5V, presumably the actual upper limit is around 3V somewhere.
* https://www.dfrobot.com/product-2357.html
* https://wiki.dfrobot.com/Fermion_MEMS_Microphone_Sensor_SKU_SEN0487
* TODO: Document the use of the specific ADC on the ESP32 for this.
* HW 1.1.0 and HW 1.2.0: The MEMS is connected to pin A3.
* HW 1.0.0: The MEMS is connected to pin A5, but this is a bug because the ADC2 configured for the MEMS conflicts with its hardwired use for WiFi, as a consequence, they can't be used at the same time.  In practice the Mic functionality is unavailable in this rev.

### Buzzer

TODO: Piezo element

* The PWM used to drive the element is here: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html
