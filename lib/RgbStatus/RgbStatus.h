#ifndef RGBSTATUS_H
#define RGBSTATUS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class RgbStatus {
public:
    enum class Mode : uint8_t {
        Boot,
        ReadyFlash,
        Ready,
        Moving,
        Crash,
        Standby
    };

    explicit RgbStatus(uint8_t dataPin);

    void begin();
    void update();
    void setEnabled(bool enabled);
    void setBrightness(uint8_t brightness);
    void setMode(Mode mode);

private:
    void showColor(uint32_t color);

    Adafruit_NeoPixel _pixel;
    Mode _mode;
    uint32_t _modeStartedAt;
    uint32_t _lastColor;
    uint8_t _brightness;
    bool _enabled;
    bool _begun;
};

#endif
