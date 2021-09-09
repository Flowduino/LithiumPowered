[Latest Release - ![Release Version](https://img.shields.io/github/release/Flowduino/LithiumPowered.svg?style=plastic&logo=github)![Release Date](https://img.shields.io/github/release-date/Flowduino/LithiumPowered.svg?style=plastic&logo=github)](https://github.com/Flowduino/LithiumPowered/releases/latest/)  

Need Help? [![Discord](https://img.shields.io/badge/Discuss-on%20Discord-7289d9?style=plastic&logo=discord)](https://discord.gg/jpwBy7VzaG) [![Instagram](https://img.shields.io/badge/Follow-on%20Instagram-c32aa3?style=plastic&logo=instagram)](https://instagram.com/Flowduino)

# Initial Development!
Please be aware that this Library is currently in its initial development stages. It will function on the ESP32 just fine (tested and verified using PlatformIO for /examples/BasicDemo).
The next release will include working support for the Arduino Uno and Nano R3 will be completed, tested, and submitted for 0.0.4.

# LithiumPowered
A (hopefully) Universal Library for Lithium-Powered Projects, designed to be available for both Arduino IDE and PlatformIO Projects.

This Library uses a Coulomb Counter (*such as the LTC4150 circuit*) to accurately keep track of the state of specifically Lithium Ion (*Li-Ion*) and Lithium Polymer (*Li-Po*) Batteries.

What makes this Library particularly beneficial is that it fully-encapsulates the code necessary to automatically and dynamically Calibrate the Battery Capacity (*in mAh*) while the Battery is Charging *and* Discharging. It also leverages persistent Storage on your MCU so that the Battery State is always remembered between power cycles (*switching your device off and on*)

## Cross-Platform
LithiumPowered is designed to function identically for all MCUs* and all Lithium Battery Types & Capacities**.

>* *Must have suitable GPIO pins, support Interrupts (*ISR*), and provide some form of compatible persistent Storage (*e.g. EEPROM*).
>* **You must configure your `Battery` instance by telling it the expected Capacity (*in mAh*) of your Lithium Cell(s)

## Easy Callbacks
The operative Class, `Battery`, can be initialised with an Instance of your own custom decendant of `BatteryCallbacks`. Each method defined in `BatteryCallbacks` that you override in your decendant will be invokved as an *Event* reflecting the name of the respective overriden Method.

e.g.

```c++
class MyBatteryCallbacks: public BatteryCallbacks {
    public:
        void onBatteryNowCharging() {
            Serial.println("Battery is now Charging!");
        }

        void onBatteryNowDischarging() {
            Serial.println("Battery is no longer Charging!");
        }
};
```

## GPIO Settings
The operative Class, `Battery`, can be initialised with an Instance of your own custom decendant of `BatteryGPIO`.
Each method defined in `BatteryGPIO` that you override in your decendant will define an explicit GPIO Pin to be used for the corresponding Coloumb Pin represented by the name of the respective overriden Method.

e.g.

```c++
class MyBatteryGPIO: public BatteryGPIO {
    uint8_t getPinInterrupt() { return 14; };
};
```

The above will tell our `Battery` instance to use GPIO Pin 14 for the main Coloumb Counter Interrupt pin, rather than the default defined in the Base Class, `BatteryGPIO`.

## Initialising Battery

```c++
#include <LithiumPowered.h>

Battery myBattery();

void setup() {
    Serial.begin(115200);

    myBattery.setCallbacks(new MyBatteryCallbacks()); // Use your Custom Callbacks
    myBattery.setGpio(new MyBatteryGPIO()); // Use your Custom GPIO Settings
    // We would now change any Properties of myBattery as required
    myBattery.setup(500); // This will Initialise our Battery instance with the given Property values, where 500 tells it that the rated Battery capacity is 500mAh
};

void loop() {
    myBattery.loop(); // We need to ensure we Loop our Battery Instance to update its State
}
```

With the above in place, we will now see lines appear in the Console Output each time we connect or disconnect (respectively) our Device to an external Power Supply.

**Note: If you are supplying power through the cable used to monitor Serial output, your Device will *always* be Charging!**