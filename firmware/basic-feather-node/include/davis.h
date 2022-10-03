#pragma once

#include <Arduino.h>

static volatile uint32_t wind_count = 0;

void davis_isr() {
    wind_count++;
}

class Davis {
private:
    int _directionPin;
    int _speedPin;

public:
    Davis(int directionPin, int speedPin) : _directionPin { directionPin}, _speedPin { speedPin } {
        pinMode(_directionPin, INPUT);
        pinMode(_speedPin, INPUT);
    }

    int getDirectionRaw() {
        return analogRead(_directionPin);
    }

    float getDirectionDegrees() {
        static const float a2dFactor = 360.0f / 1024.0f;
        return getDirectionRaw() * a2dFactor;
    }

    void startSpeedMeasurement() {
        wind_count = 0;
        attachInterrupt(_speedPin, davis_isr, FALLING);
    }

    uint32_t stopSpeedMeasurement() {
        detachInterrupt(_speedPin);
        return wind_count;
    }

    float getSpeedMph(const uint32_t count, const uint32_t ms) {
        // convert to mp/h using the formula V=P(2.25/T)
        return count * 2.25 / (ms / 1000);
    }

    float getSpeedKph(const uint32_t count, const uint32_t ms) {
        return getSpeedMph(count, ms) * 1.60934;
    }
};
