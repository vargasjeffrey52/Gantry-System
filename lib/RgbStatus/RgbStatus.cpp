#include "RgbStatus.h"

namespace {
constexpr uint8_t DEFAULT_LED_BRIGHTNESS = 128;
constexpr uint32_t BOOT_BLUE = 0x0000FF;
constexpr uint32_t READY_FLASH_GREEN = 0x00FF00;
constexpr uint32_t EMERALD_GREEN = 0x006000;
constexpr uint32_t READY_FLASH_ON_MS = 250;
constexpr uint32_t MOVING_YELLOW = 0xFFFF00;
constexpr uint32_t CRASH_RED = 0xFF0000;
constexpr uint32_t STANDBY_PURPLE = 0x200080;
constexpr uint32_t STANDBY_PINK = 0xFF1493;
constexpr uint32_t LED_OFF = 0;
}

RgbStatus::RgbStatus(uint8_t dataPin)
    : _pixel(1, dataPin, NEO_GRB + NEO_KHZ800),
      _mode(Mode::Boot),
      _modeStartedAt(0),
      _lastColor(LED_OFF),
      _brightness(DEFAULT_LED_BRIGHTNESS),
      _enabled(true),
      _begun(false)
{
}

void RgbStatus::begin() {
    _pixel.begin();
    _pixel.setBrightness(_brightness);
    _pixel.clear();
    _pixel.show();
    _modeStartedAt = millis();
    _begun = true;
    update();
}

void RgbStatus::update() {
    if (!_begun || !_enabled) {
        return;
    }

    const uint32_t elapsed = millis() - _modeStartedAt;
    uint32_t color = LED_OFF;

    switch (_mode) {
        case Mode::Boot:
            color = (elapsed % 500 < 250) ? BOOT_BLUE : LED_OFF;
            break;
        case Mode::ReadyFlash:
            if (elapsed < READY_FLASH_ON_MS) {
                color = READY_FLASH_GREEN;
            } else {
                _mode = Mode::Ready;
                color = EMERALD_GREEN;
            }
            break;
        case Mode::Ready:
            color = EMERALD_GREEN;
            break;
        case Mode::Moving:
            color = MOVING_YELLOW;
            break;
        case Mode::Crash:
            color = (elapsed % 160 < 80) ? CRASH_RED : LED_OFF;
            break;
        case Mode::Standby: {
            const uint32_t phase = elapsed % 4000;
            color = (phase < 2000) ? STANDBY_PURPLE : STANDBY_PINK;
            break;
        }
    }

    showColor(color);
}

void RgbStatus::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!_begun) {
        return;
    }

    if (enabled) {
        _lastColor = UINT32_MAX;
        update();
    } else {
        showColor(LED_OFF);
    }
}

void RgbStatus::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    if (!_begun) {
        return;
    }

    _pixel.setBrightness(_brightness);
    _lastColor = UINT32_MAX;
    update();
}

void RgbStatus::setMode(Mode mode) {
    if (_mode == mode) {
        return;
    }

    _mode = mode;
    _modeStartedAt = millis();
    _lastColor = UINT32_MAX;
    update();
}

void RgbStatus::showColor(uint32_t color) {
    if (color == _lastColor) {
        return;
    }

    _pixel.setPixelColor(0, color);
    _pixel.show();
    _lastColor = color;
}
