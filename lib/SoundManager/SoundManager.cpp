#include "SoundManager.h"

#include <esp_arduino_version.h>

namespace {
constexpr uint32_t PWM_FREQUENCY_HZ = 1000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr uint8_t PWM_MAX_DUTY = (1U << PWM_RESOLUTION_BITS) - 1;
#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
constexpr uint8_t PWM_CHANNEL = 7;
#endif
}

SoundManager::SoundManager(int buzzerPin)
    : _buzzerPin(buzzerPin),
      _notes(nullptr),
      _queuedNotes(nullptr),
      _noteCount(0),
      _queuedNoteCount(0),
      _noteIndex(0),
      _noteStartedAt(0),
      _nextNoteAt(0),
      _lastStepSoundUpdateAt(0),
      _stepSoundStopStartedAt(0),
      _stepSoundFrequency(450),
      _stepSoundStopFrequency(450),
      _volumePercent(30),
      _oneShotNote{0, 0, 0},
      _enabled(true),
      _stepSoundEnabled(false),
      _noteActive(false),
      _stepSoundActive(false),
      _stepSoundStopping(false),
      _begun(false)
{
}

void SoundManager::begin() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    if (!ledcAttach(_buzzerPin, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS)) {
        Serial.println("Error: could not attach buzzer PWM");
        return;
    }
#else
    if (ledcSetup(PWM_CHANNEL, PWM_FREQUENCY_HZ, PWM_RESOLUTION_BITS) == 0) {
        Serial.println("Error: could not configure buzzer PWM");
        return;
    }
    ledcAttachPin(_buzzerPin, PWM_CHANNEL);
#endif
    stopTone();
    _begun = true;
}

void SoundManager::update() {
    if (!_begun || !_enabled) {
        return;
    }

    const uint32_t now = millis();
    if (_stepSoundStopping && _notes == nullptr) {
        const uint32_t elapsed = now - _stepSoundStopStartedAt;
        const uint32_t fadeDurationMs = 180;
        if (elapsed >= fadeDurationMs) {
            stopStepSound();
            startQueuedMelody();
            return;
        }

        if (now - _lastStepSoundUpdateAt >= 20) {
            const float progress = static_cast<float>(elapsed) / fadeDurationMs;
            const uint16_t frequency = static_cast<uint16_t>(
                _stepSoundStopFrequency -
                (_stepSoundStopFrequency - 300) * progress);
            startTone(frequency);
            _lastStepSoundUpdateAt = now;
        }
        return;
    }

    if (_notes == nullptr) {
        return;
    }

    if (_noteActive) {
        const Note& note = _notes[_noteIndex];
        if (now - _noteStartedAt < note.durationMs) {
            return;
        }

        stopTone();
        _noteActive = false;
        ++_noteIndex;
        if (_noteIndex >= _noteCount) {
            _notes = nullptr;
            _noteCount = 0;
            _noteIndex = 0;
            return;
        }

        _nextNoteAt = now + note.pauseMs;
        return;
    }

    if (static_cast<int32_t>(now - _nextNoteAt) < 0) {
        return;
    }

    const Note& note = _notes[_noteIndex];
    if (note.durationMs == 0) {
        ++_noteIndex;
        if (_noteIndex >= _noteCount) {
            _notes = nullptr;
            _noteCount = 0;
            _noteIndex = 0;
        }
        return;
    }

    if (note.frequency == 0) {
        stopTone(); // A rest still consumes its duration without blocking.
    } else {
        startTone(note.frequency);
    }
    _noteStartedAt = now;
    _noteActive = true;
}

void SoundManager::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!enabled) {
        stopPlayback();
        stopStepSound();
        _queuedNotes = nullptr;
        _queuedNoteCount = 0;
    }
}

void SoundManager::setStepSoundEnabled(bool enabled) {
    _stepSoundEnabled = enabled;
    if (!enabled && _stepSoundActive) {
        stopStepSound();
        startQueuedMelody();
    }
}

void SoundManager::stop() {
    _queuedNotes = nullptr;
    _queuedNoteCount = 0;
    stopPlayback();
    stopStepSound();
}

bool SoundManager::isEnabled() const {
    return _enabled;
}

void SoundManager::setVolume(uint8_t volumePercent) {
    _volumePercent = (volumePercent > 100) ? 100 : volumePercent;
    if ((_noteActive && _notes != nullptr &&
         _notes[_noteIndex].frequency != 0) || _stepSoundActive) {
        applyVolume();
    }
}

bool SoundManager::isPlaying() const {
    return _notes != nullptr || _noteActive || _stepSoundActive ||
           _queuedNotes != nullptr;
}

void SoundManager::playStartupMelody() {
    static const Note startupNotes[] = {
        {1319, 120, 25}, {1568, 120, 25}, {2637, 120, 25},
        {2093, 120, 25}, {2349, 120, 25}, {3136, 120, 175},
        {988, 50, 10}, {1319, 400, 50}
    };
    startMelody(startupNotes, sizeof(startupNotes) / sizeof(startupNotes[0]));
}

void SoundManager::playCompletionMelody() {
    static const Note completionNotes[] = {
        {784, 100, 25}, {988, 100, 25}, {1175, 200, 0}
    };
    startMelody(completionNotes, sizeof(completionNotes) / sizeof(completionNotes[0]));
}

void SoundManager::playSingleTone() {
    _oneShotNote = {880, 120, 0};
    startMelody(&_oneShotNote, 1);
}

void SoundManager::playRandomTone() {
    _oneShotNote = {
        static_cast<uint16_t>(random(220, 1321)),
        static_cast<uint16_t>(random(80, 241)),
        0
    };
    startMelody(&_oneShotNote, 1);
}

void SoundManager::playCrashMelody() {
    static const Note crashNotes[] = {
        {440, 180, 30}, {330, 220, 30}, {220, 400, 0}
    };
    startMelody(crashNotes, sizeof(crashNotes) / sizeof(crashNotes[0]));
}

// NES Mario death melody from the user-provided Arduino sketch.
// Uses the existing non-blocking Note {frequency, onMs, silenceAfterMs}
// scheduler instead of delay(). BPM = 90; gate/articulation = 90%.
// Dotted rhythms are retained using integer durations from the original.
void SoundManager::playDeathMelody() {
    static const Note deathNotes[] = {
        {1047, 74, 9}, // note 1: 83 ms total, 90% gate
        {1047, 74, 9}, // note 2: 83 ms total, 90% gate
        {1047, 74, 9}, // note 3: 83 ms total, 90% gate
        {0, 333, 0}, // rest: 333 ms
        {494, 149, 17}, // note 5: 166 ms total, 90% gate
        {1397, 149, 17}, // note 6: 166 ms total, 90% gate
        {0, 166, 0}, // rest: 166 ms
        {1397, 149, 17}, // note 8: 166 ms total, 90% gate
        {1397, 224, 25}, // note 9: 249 ms total, 90% gate
        {1319, 224, 25}, // note 10: 249 ms total, 90% gate
        {1175, 149, 17}, // note 11: 166 ms total, 90% gate
        {1047, 149, 17}, // note 12: 166 ms total, 90% gate
        {0, 166, 0}, // rest: 166 ms
        {659, 149, 17}, // note 14: 166 ms total, 90% gate
        {0, 166, 0}, // rest: 166 ms
        {523, 149, 17}, // note 16: 166 ms total, 90% gate
    };
    startMelody(deathNotes, sizeof(deathNotes) / sizeof(deathNotes[0]));
}

// Non-blocking transcription of the supplied myCustomTheme() sequence.
// Each entry stores {frequency, soundingMs, silenceAfterMs}.
// Rests use {0, restDurationMs, 0} and consume time in update().
// BPM = 120 (quarter = 500 ms, eighth = 250 ms, sixteenth = 125 ms).
void SoundManager::playCustomTheme() {
    static const Note customNotes[] = {
        // phraseA, repeat 1
        {294, 162, 88}, // D4, 250 ms, 65% gate
        {523, 162, 88}, // C5, 250 ms, 65% gate
        {523, 137, 113}, // C5, 250 ms, 55% gate
        {523, 162, 88}, // C5, 250 ms, 65% gate
        {349, 81, 44}, // F4, 125 ms, 65% gate
        {440, 81, 44}, // A4, 125 ms, 65% gate
        {523, 212, 38}, // C5, 250 ms, 85% gate
        {0, 250, 0}, // timed rest
        {523, 200, 50}, // C5, 250 ms, 80% gate
        // phraseB, repeat 1
        {262, 162, 88}, // C4, 250 ms, 65% gate
        {494, 162, 88}, // B4, 250 ms, 65% gate
        {494, 137, 113}, // B4, 250 ms, 55% gate
        {494, 162, 88}, // B4, 250 ms, 65% gate
        {330, 81, 44}, // E4, 125 ms, 65% gate
        {392, 81, 44}, // G4, 125 ms, 65% gate
        {494, 212, 38}, // B4, 250 ms, 85% gate
        {0, 250, 0}, // timed rest
        {494, 200, 50}, // B4, 250 ms, 80% gate
        // phraseA, repeat 2
        {294, 162, 88}, // D4, 250 ms, 65% gate
        {523, 162, 88}, // C5, 250 ms, 65% gate
        {523, 137, 113}, // C5, 250 ms, 55% gate
        {523, 162, 88}, // C5, 250 ms, 65% gate
        {349, 81, 44}, // F4, 125 ms, 65% gate
        {440, 81, 44}, // A4, 125 ms, 65% gate
        {523, 212, 38}, // C5, 250 ms, 85% gate
        {0, 250, 0}, // timed rest
        {523, 200, 50}, // C5, 250 ms, 80% gate
        // phraseB, repeat 2
        {262, 162, 88}, // C4, 250 ms, 65% gate
        {494, 162, 88}, // B4, 250 ms, 65% gate
        {494, 137, 113}, // B4, 250 ms, 55% gate
        {494, 162, 88}, // B4, 250 ms, 65% gate
        {330, 81, 44}, // E4, 125 ms, 65% gate
        {392, 81, 44}, // G4, 125 ms, 65% gate
        {494, 212, 38}, // B4, 250 ms, 85% gate
        {0, 250, 0}, // timed rest
        {494, 200, 50}, // B4, 250 ms, 80% gate
        // phraseA, repeat 3
        {294, 162, 88}, // D4, 250 ms, 65% gate
        {523, 162, 88}, // C5, 250 ms, 65% gate
        {523, 137, 113}, // C5, 250 ms, 55% gate
        {523, 162, 88}, // C5, 250 ms, 65% gate
        {349, 81, 44}, // F4, 125 ms, 65% gate
        {440, 81, 44}, // A4, 125 ms, 65% gate
        {523, 212, 38}, // C5, 250 ms, 85% gate
        {0, 250, 0}, // timed rest
        {523, 200, 50}, // C5, 250 ms, 80% gate
        // phraseB, repeat 3
        {262, 162, 88}, // C4, 250 ms, 65% gate
        {494, 162, 88}, // B4, 250 ms, 65% gate
        {494, 137, 113}, // B4, 250 ms, 55% gate
        {494, 162, 88}, // B4, 250 ms, 65% gate
        {330, 81, 44}, // E4, 125 ms, 65% gate
        {392, 81, 44}, // G4, 125 ms, 65% gate
        {494, 212, 38}, // B4, 250 ms, 85% gate
        {0, 250, 0}, // timed rest
        {494, 200, 50}, // B4, 250 ms, 80% gate
        // turnaround
        {440, 187, 63}, // A4, 250 ms, 75% gate
        {466, 187, 63}, // AS4, 250 ms, 75% gate
        {494, 200, 50}, // B4, 250 ms, 80% gate
        {0, 250, 0}, // timed rest
        {349, 162, 88}, // F4, 250 ms, 65% gate
        {440, 162, 88}, // A4, 250 ms, 65% gate
        {523, 425, 75}, // C5, 500 ms, 85% gate
        // ending
        {523, 137, 113}, // C5, 250 ms, 55% gate
        {523, 137, 113}, // C5, 250 ms, 55% gate
        {523, 137, 113}, // C5, 250 ms, 55% gate
        {523, 137, 113}, // C5, 250 ms, 55% gate
        {523, 450, 50}, // C5, 500 ms, 90% gate
        {0, 500, 0}, // timed rest
    };
    startMelody(customNotes, sizeof(customNotes) / sizeof(customNotes[0]));
}

void SoundManager::updateStepSound(float speedRatio) {
    if (!_begun || !_enabled || !_stepSoundEnabled || _notes != nullptr) {
        return;
    }

    const uint32_t now = millis();
    _stepSoundStopping = false;
    speedRatio = constrain(speedRatio, 0.0f, 1.0f);
    const uint16_t targetFrequency = static_cast<uint16_t>(450 + 1050 * speedRatio);

    if (!_stepSoundActive) {
        _stepSoundActive = true;
        _stepSoundFrequency = 450;
        _lastStepSoundUpdateAt = now;
        startTone(_stepSoundFrequency);
        return;
    }

    if (now - _lastStepSoundUpdateAt < 20) {
        return;
    }

    _stepSoundFrequency = targetFrequency;
    _lastStepSoundUpdateAt = now;
    startTone(_stepSoundFrequency);
}

void SoundManager::finishStepSound() {
    if (!_stepSoundActive || _stepSoundStopping) {
        return;
    }

    _stepSoundStopping = true;
    _stepSoundStopStartedAt = millis();
    _stepSoundStopFrequency = _stepSoundFrequency;
    _lastStepSoundUpdateAt = _stepSoundStopStartedAt;
}

void SoundManager::startMelody(const Note* notes, size_t noteCount) {
    if (!_begun || !_enabled || notes == nullptr || noteCount == 0) {
        return;
    }

    if (_stepSoundActive) {
        _queuedNotes = notes;
        _queuedNoteCount = noteCount;
        finishStepSound();
        return;
    }

    stopPlayback();
    _queuedNotes = nullptr;
    _queuedNoteCount = 0;
    _notes = notes;
    _noteCount = noteCount;
    _noteIndex = 0;
    _nextNoteAt = millis();
}

void SoundManager::stopPlayback() {
    if (_begun) {
        stopTone();
    }
    _notes = nullptr;
    _noteCount = 0;
    _noteIndex = 0;
    _noteActive = false;
}

void SoundManager::stopStepSound() {
    if (_stepSoundActive && _begun) {
        stopTone();
    }
    _stepSoundActive = false;
    _stepSoundStopping = false;
    _stepSoundFrequency = 450;
}

void SoundManager::startQueuedMelody() {
    if (_queuedNotes == nullptr || _queuedNoteCount == 0 || !_enabled) {
        return;
    }

    const Note* notes = _queuedNotes;
    const size_t noteCount = _queuedNoteCount;
    _queuedNotes = nullptr;
    _queuedNoteCount = 0;
    startMelody(notes, noteCount);
}

void SoundManager::startTone(uint16_t frequency) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWriteTone(_buzzerPin, frequency);
    applyVolume();
#else
    ledcWriteTone(PWM_CHANNEL, frequency);
    applyVolume();
#endif
}

void SoundManager::stopTone() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(_buzzerPin, 0);
#else
    ledcWrite(PWM_CHANNEL, 0);
#endif
}

void SoundManager::applyVolume() {
    const uint32_t duty = static_cast<uint32_t>(_volumePercent) *
                          (PWM_MAX_DUTY / 2) / 100;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
    ledcWrite(_buzzerPin, duty);
#else
    ledcWrite(PWM_CHANNEL, duty);
#endif
}
