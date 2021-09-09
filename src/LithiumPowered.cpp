#include <LithiumPowered.hpp>

LithiumBattery& LithiumBattery::getInstance() {
    static LithiumBattery battery;
    return battery;
}

LithiumBattery &Battery = Battery.getInstance();

void ISR_Battery_ChargeChanged() {
    Battery._interruptChargeChanged();
}

void ISR_Battery_PolarityChanged() {
    Battery._interruptPolarityChanged();
}

void LithiumBattery::attachInterrupts() {
    attachInterrupt(digitalPinToInterrupt(Battery._pGpio->getPinInterrupt()), ISR_Battery_ChargeChanged, FALLING);
    attachInterrupt(digitalPinToInterrupt(Battery._pGpio->getPinPolarity()), ISR_Battery_PolarityChanged, CHANGE);
}