#include <Arduino.h>

#ifdef ESP32
    #include <Preferences.h> // CRITICAL FOR ESP32 DEVICES!
#endif

#include <LithiumPowered.hpp>

#define BATTERY_RATED_CAPACITY  900.00 // Set this to the Rated Capacity of your Battery in mAh!

class MyBatteryCallbacks: public BatteryCallbacks {
    public:
        virtual void onBatteryNowCharging() {
            Serial.println("Battery is now Charging!");
        };

        virtual void onBatteryNowDischarging() {
            Serial.println("Battery is no longer Charging!");
        };

        virtual void onBatteryRemainingCapacityChanged() {
            Serial.print("Battery Update: ");
            Serial.print(Battery.getPercentage());
            Serial.print("% ");
            Serial.print(Battery.getCurrentCapacity());
            Serial.print("mAh remaining. ");
            Serial.print(Battery.getIsCharging() ? "Charging at +" : "Discharging at ");
            Serial.print(Battery.getChangeCapacityWithPolarity());
            Serial.print("mAh");
            if (Battery.getIsCharging()) {
            Serial.print(" - Time to full: ");
            Serial.print(Battery.getTimeToChargeInMinutes());
            }
            else {
            Serial.print(" - Time to empty: ");
            Serial.print(Battery.getTimeToDischargeInMinutes());
            }
            Serial.println(" min");
        };

        virtual void onBatteryRecalibrated() {
            Serial.println("Maximum Battery Capacity has been Recalibrated!");
        };
};

//#define USE_CUSTOM_GPIO // Uncomment this line if you need to use Custom GPIO Pins!

#ifdef USE_CUSTOM_GPIO
    class MyBatteryGPIO: public BatteryGPIO {
        public:
            // Override this to provide the GPIO Pin Number for the Coloumb Counter's Polarity Pin
            virtual uint8_t getPinPolarity() {
                return 4;
            }

            // Override this to provide the GPIO Pin Number for the Coloumb Counter's Interrupt Pin
            virtual uint8_t getPinInterrupt() {
                return 35;
            }

            // Override this to provide the GPIO Pin Number for the Coloumb Counter's High State Reference Pin  
            virtual uint8_t getPinRefHigh() {
                return 23;
            }

            // Override this to provide the GPIO Pin Number for the Coloumb Counter's Low State Reference Pin
            virtual uint8_t getPinRefLow() {
                return 5;
            }
    };
#endif

void setup() {
    Serial.begin(115200); // Initialise Serial interface at 115200 Baud

    #ifdef USE_CUSTOM_GPIO
        Battery.setGpio(new MyBatteryGPIO()); // We want to use specific GPIO Pins
    #endif
    Battery.setCallbacks(new MyBatteryCallbacks()); // Use our custom Callbacks for Battery Events
    Battery.setup(BATTERY_RATED_CAPACITY); // Initialise the LithiumPowered Battery System
}

void loop() {
    Battery.loop(); // Required to perform State Updates for the LithiumPowered Battery System.
}
