#ifndef FLOWDUINO_LITHIUMPOWERED_HPP

    #define FLOWDUINO_LITHIUMPOWERED_HPP

    #ifdef ARDUINO
        #include <Arduino.h>
    #endif

    #include <LithiumStorage.hpp>

    /*
        BatteryState
        - Enum describing whether the Battery is presently Discharging or Charging
    */
   enum BatteryState {
       Discharging, // The Battery is Discharging
       Charging // The Battery is Charging
   };

    /*
        BatteryCallbacks
        - This is an Abstract Base Class you can implement to hook Battery Events for your own code.
    */
    class BatteryCallbacks {
        public:
            // Override this to provide a custom behaviour when your Device is connected to an External Supply.
            virtual void onBatteryNowCharging() {};

            // Override this to provide a custom behaviour when your Device is disconnected from an External Supply.
            virtual void onBatteryNowDischarging() {};

            // Override this to provide a custom behaviour when your Device' Battery Gains or Loses Capacity (Charge)
            virtual void onBatteryRemainingCapacityChanged() {};

            // Override this to provide a custom behaviour when your Device' Battery Data has been automatically Recalibrated.
            virtual void onBatteryRecalibrated() {};
    };

    /*
        BatteryGPIO
        - This is a Default Base Class you can implement to define explicit GPIO Pins for your Coulomb Counter
    */
    class BatteryGPIO {
        public:
            // Override this to provide the GPIO Pin Number for the Coloumb Counter's Polarity Pin
            virtual uint8_t getPinPolarity() {
                return 4; // We have chosen GPIO 4 by Default for Polarity
            }

            // Override this to provide the GPIO Pin Number for the Coloumb Counter's Interrupt Pin
            virtual uint8_t getPinInterrupt() {
                #ifdef ESP32
                    return 35; // We have chosen GPIO 35 as the Default for the ESP32
                #else
                    return 2; // For Arduino, we use GPIO 2 because it is a valid Interrupt Pin on most/all Boards
                #endif
            }

            // Override this to provide the GPIO Pin Number for the Coloumb Counter's High State Reference Pin  
            virtual uint8_t getPinRefHigh() {
                #ifdef ESP32
                    return 23; // We have chosen GPIO 23 as the Default for the ESP32
                #elif ARDUINO_AVR
                    return AREF; // We use the AREF pin on Arduino boards
                #else
                    return 0; // if in doubt, we use 0... but this may not work on your board!
                #endif
            }

            // Override this to provide the GPIO Pin Number for the Coloumb Counter's Low State Reference Pin
            virtual uint8_t getPinRefLow() {
                #ifdef ESP32
                    return 5; // We have chosen GPIO 5 as the Default for the ESP32
                #else
                    return 1; // If in dobut, we use 1... but this may not work on your board!
                #endif
            }
    };

    /*
        Battery
        - This is the Battery Manager for Lithium Batteries.
        - It is intended for use with a suitable Coloumb Counter such as the LTC4150
        - It is a SINGLETON! You must call .getInstance() to reference the Single Instance.
    */
    class LithiumBattery {
        private:
            static constexpr double AH_QUANTA = 0.17067759; // mAh for each INT (This is specified by the Coulomb Counter LTC4150)

            // Component Classes
            BatteryCallbacks _defaultCallbacks;
            BatteryGPIO _defaultGpio;
            BatteryCallbacks* _pCallbacks;
            BatteryGPIO* _pGpio;
            LithiumStorageBase* _pStorage;

            // Internal Properties
            volatile bool _isr; // Flag we set each time the Charge State Interrupt fires
            volatile bool _isrPol; // Flag we set each time the Polarity Interrupt fires
            volatile BatteryState _lastState; // We need to keep track of the Previous Charging State so we know when it has changed
            volatile unsigned long _lastTime; // Last time (millis()) the Interrupt fired
            volatile unsigned long _deltaTime; // How many milliseconds elapsed between Interrupt firings
            double _percentage; // Physical Battery Percentage Remaining
            double _percentQuanta; // Quantitative Percentage
            double _mAh; // Current Capacity
            double _mAhMax; // Capacity when Battery is full
            double _mAhRated; // Capacity stated on the Battery's label
            double _mAhChange; // Capacity lost (!_wasCharging) or gained (_wasCharging)
            volatile BatteryState _state; // Is the Battery Charging or Discharging?
            uint8_t _pinInterrupt; // The GPIO Pin to use as the Interrupt
            uint8_t _pinPolarity; // The GPIO Pin to interrogate the Polarity
            uint8_t _pinVio; // The GPIO Pin to use for Reference Voltage (Voltage High)
            uint8_t _pinGnd; // The GPIO Pin to use for Reference Ground (Voltage Low)
            bool _wasInitialised = false;

            void initialiseStorage() {
                #ifdef ARDUINO_AVR
                    _pStorage = new LithiumStorageArduino();
                #elif ESP32
                    _pStorage = new LithiumStorageESP32();
                #else
                    #error There is no Storage class for this hardware yet!
                #endif
            };

            void attachInterrupts();

            inline void refreshCurrentState() { _state = digitalRead(_pGpio->getPinPolarity()) == HIGH ? Charging : Discharging; };

            LithiumBattery() : _defaultCallbacks(), _defaultGpio() {
                _pCallbacks = &_defaultCallbacks;
                _pGpio = &_defaultGpio;
                initialiseStorage();
            };
        public:
            static LithiumBattery& getInstance();

            /*
            * @brief Initialise the Battery System.
            * @param [in] defaultCapacity The Default Battery Capacity (in mAh)
            */
            void setup(double defaultCapacity) {
                _wasInitialised = true;

                // Input Pins
                pinMode(_pGpio->getPinInterrupt(), INPUT);
                pinMode(_pGpio->getPinPolarity(), INPUT);

                // Output Pins
                pinMode(_pGpio->getPinRefHigh(), OUTPUT);
                digitalWrite(_pGpio->getPinRefHigh(), HIGH);
                pinMode(_pGpio->getPinRefLow(), OUTPUT);
                digitalWrite(_pGpio->getPinRefLow(), LOW);

                // Initial Values
                _mAhRated = defaultCapacity;
                _mAh = _pStorage->getLastBatteryCapacity(defaultCapacity);
                _mAhMax = _pStorage->getMaxmimumBatteryCapacity(defaultCapacity);
                _mAhChange = 0.00;  // Default is that there was no change.
                _percentage = (_mAh / _mAhMax) * 100.00;
                _percentQuanta = 1.0 / (_mAhMax / 1000.0 * 5859.0 / 100.0);
                _isr = false;
                refreshCurrentState();

                attachInterrupts();
            }

            inline void loop() {
                if (_isrPol) {
                    _isrPol = false;
                    switch (_state) {
                        case Charging:
                            _pCallbacks->onBatteryNowCharging();
                            break;
                        case Discharging:
                            _pCallbacks->onBatteryNowDischarging();
                            break;
                    }
                }
                if (!_isr) {
                    if (micros() - _lastTime > 120000000 && getIsCharging() && (_mAhMax != _mAhRated || getPercentage() != 100.00)) {
                        _percentage = 100.00;
                        _mAh = _mAhRated;
                        _mAhMax = _mAhRated;
                        _pCallbacks->onBatteryRecalibrated();
                    }

                    return;
                };

                _isr = false;

                if (_lastState == Charging) { // We need to calculate based on whether or not the device WAS charging at the time!
                    _mAh += AH_QUANTA;
                    _percentage += _percentQuanta;
                    if (_mAh > _mAhMax) {// This will ensure the Battery always shows the calibrated max for 100%
                        _mAhMax = _mAh;
                        _percentage = 100.00;
                    } 
                }
                else {
                    _mAh -= AH_QUANTA;
                    _percentage -= _percentQuanta;
                    if (_mAh < 0.00) { // This will calibrate the Max to reflect the real Battery capacity.
                        _mAhMax -= _mAh;
                        _mAh = 0.00;
                        _percentage = 0.00;
                    }
                }

                _mAhChange = 614.4/((_deltaTime)/1000000.0); // Calculate by how much the capacity just changed.

                _pStorage->setLastBatteryCapacity(_mAh);
                _pStorage->setMaximumBatteryCapacity(_mAhMax);

                _pCallbacks->onBatteryRemainingCapacityChanged();
            }

            // WARNING: Do NOT invoke this method in your own code! Only the registered ISR may call this!
            void _interruptChargeChanged() {
                unsigned long refTime = micros();
                _deltaTime = refTime - _lastTime;
                _lastTime = refTime;
                _lastState = _state;
                _isr = true;
            };

            // WARNING: Do NOT invoke this method in your own code! Only the registered ISR may call this!
            void _interruptPolarityChanged() {
                refreshCurrentState();
                _isrPol = true;
            };
            
            // Getters/Setters
            inline BatteryCallbacks* getCallbacks() { return _pCallbacks; };
            // You can only set the Callbacks Class Instance to use PRIOR to calling setup().
            inline void setCallbacks(BatteryCallbacks* pCallbacks) {
                if (_wasInitialised) { return; }
                _pCallbacks = pCallbacks == nullptr ? &_defaultCallbacks : pCallbacks;
            };

            inline BatteryGPIO* getGpio() { return _pGpio; };
            // You can only set the GPIO Class Instance to use PRIOR to calling setup().
            inline void setGpio(BatteryGPIO* pGpio) {
                if (_wasInitialised) { return; }
                _pGpio = pGpio == nullptr ? &_defaultGpio : pGpio;
            };

            inline double getRatedCapacity() { return _mAhRated; };
            inline void setRatedCapacity(double& mAhRated) { _mAhRated = mAhRated; };
            inline void setRatedCapacity(double mAhRated) { _mAhRated = mAhRated; };

            inline double getCurrentCapacity() { return _mAh; };
            inline double getMaximumCapacity() { return _mAhMax; };
            inline double getChangeCapacity() { return _mAhChange; };
            inline double getPercentage() { return _percentage; };
            inline bool getIsCharging() { return _state == Charging; };
            inline bool getIsDischarging() { return _state == Discharging; };
            inline BatteryState getState() { return _state; };

            inline double getChangeCapacityWithPolarity() { return _lastState == Charging ? _mAhChange : _mAhChange * -1.0; }; // This is corrected for negative value on Discharge, positive on Charge.
            inline unsigned long getLastTime() { return _lastTime; };
            inline unsigned long getDeltaTime() { return _deltaTime; };

            // Discharging Times
            inline double getTimeToDischargeInHours() { return getIsCharging() ? __DBL_MAX__ :  _mAh / _mAhChange; };
            inline double getTimeToDischargeInMinutes() { return getIsCharging() ? __DBL_MAX__ : getTimeToDischargeInHours() * 60.00; };
            inline double getTimeToDischargeInSeconds() { return getIsCharging() ? __DBL_MAX__ : getTimeToDischargeInMinutes() * 60.00; };
            inline double getTimeToDischargeInMilliseconds() { return getIsCharging() ? __DBL_MAX__ : getTimeToDischargeInSeconds() * 1000.00; };
            inline double getTimeToDischargeInMicroseconds() { return getIsCharging() ? __DBL_MAX__ : getTimeToDischargeInMilliseconds() * 1000.00; };
            // Charging Times
            inline double getTimeToChargeInHours() { return getIsDischarging() ? __DBL_MAX__ :  (_mAhMax - _mAh) / _mAhChange; };
            inline double getTimeToChargeInMinutes() { return getIsDischarging() ? __DBL_MAX__ : getTimeToChargeInHours() * 60.00; };
            inline double getTimeToChargeInSeconds() { return getIsDischarging() ? __DBL_MAX__ : getTimeToChargeInMinutes() * 60.00; };
            inline double getTimeToChargeInMilliseconds() { return getIsDischarging() ? __DBL_MAX__ : getTimeToChargeInSeconds() * 1000.00; };
            inline double getTimeToChargeInMicroseconds() { return getIsDischarging() ? __DBL_MAX__ : getTimeToChargeInMilliseconds() * 1000.00; };
    };

    extern LithiumBattery& Battery;

#endif