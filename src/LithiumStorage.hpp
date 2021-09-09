#ifndef FLOWDUINO_LITHIUMSTORAGE_HPP

    #define FLOWDUINO_LITHIUMSTORAGE_HPP

    #ifdef ARDUINO
        #include <Arduino.h>
    #endif

    #ifdef ARDUINO_AVR
        #include <EEPROM.h>
    #elif ESP32
        // We use the "Preferences" Library for the ESP32 boards.
        #include <Preferences.h>
        const static char BATTERY[] = "LithiumPowered";
        const static char BATTERY_CURRENT_CAPACITY[] = "mAh";
        const static char BATTERY_MAX_CAPACITY[] = "mAhMax";
    #endif

    /*
        LithiumStorageBase
        - Abstract Base Class (Interface) to define methods for storage and recallection of Lithium Battery Data persistently on your Device.
        - This is necessary to ensure that the Battery Data is accurate after powering-off and powering-on your Device.
        - This Class is entirely Abstract, and cannot be instanciated!
    */
    class LithiumStorageBase {
        public:
            inline virtual double getLastBatteryCapacity(double& defaultValue) = 0; // Abstract
            inline virtual void setLastBatteryCapacity(double& mAh) = 0; // Abstract

            inline virtual double getMaxmimumBatteryCapacity(double& defaultValue) = 0; // Abstract
            inline virtual void setMaximumBatteryCapacity(double &mAhMax) = 0; // Abstract
    };

    #ifdef ARDUINO_AVR
        class LithiumStorageArduino: public LithiumStorageBase {

        };
    #elif ESP32
        class LithiumStorageESP32: public LithiumStorageBase {
            private:
                Preferences _prefs;
            public:
                LithiumStorageESP32() : _prefs() {
                    _prefs.begin(BATTERY, false);
                }

                // Getters/Setters
                inline double getLastBatteryCapacity(double& defaultValue) { return _prefs.isKey(BATTERY_CURRENT_CAPACITY) ? _prefs.getDouble(BATTERY_CURRENT_CAPACITY) : defaultValue; };
                inline double getLastBatteryCapacity(double defaultValue) { return _prefs.isKey(BATTERY_CURRENT_CAPACITY) ? _prefs.getDouble(BATTERY_CURRENT_CAPACITY) : defaultValue; };
                inline void setLastBatteryCapacity(double& mAh) { _prefs.putDouble(BATTERY_CURRENT_CAPACITY, mAh); };

                inline double getMaxmimumBatteryCapacity(double& defaultValue) { return _prefs.isKey(BATTERY_MAX_CAPACITY) ? _prefs.getDouble(BATTERY_MAX_CAPACITY) : defaultValue; };
                inline double getMaxmimumBatteryCapacity(double defaultValue) { return _prefs.isKey(BATTERY_MAX_CAPACITY) ? _prefs.getDouble(BATTERY_MAX_CAPACITY) : defaultValue; };
                inline void setMaximumBatteryCapacity(double &mAhMax) { _prefs.putDouble(BATTERY_MAX_CAPACITY, mAhMax); };
        };
    #else
        #error Your board is not yet officially compatible with LithiumPowered.
    #endif

#endif