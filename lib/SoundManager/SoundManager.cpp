#include "SoundManager.h"

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
    pinMode(_buzzerPin, OUTPUT);
    noTone(_buzzerPin);
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
            tone(_buzzerPin, frequency);
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

        noTone(_buzzerPin);
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
    if (note.frequency == 0 || note.durationMs == 0) {
        ++_noteIndex;
        if (_noteIndex >= _noteCount) {
            _notes = nullptr;
            _noteCount = 0;
            _noteIndex = 0;
        }
        return;
    }

    tone(_buzzerPin, note.frequency);
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

void SoundManager::playStartupMelody() {
    static const Note startupNotes[] = {
        {523, 110, 25}, {659, 110, 25}, {784, 180, 40}, {1047, 240, 0}
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
        tone(_buzzerPin, _stepSoundFrequency);
        return;
    }

    if (now - _lastStepSoundUpdateAt < 20) {
        return;
    }

    _stepSoundFrequency = targetFrequency;
    _lastStepSoundUpdateAt = now;
    tone(_buzzerPin, _stepSoundFrequency);
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
        noTone(_buzzerPin);
    }
    _notes = nullptr;
    _noteCount = 0;
    _noteIndex = 0;
    _noteActive = false;
}

void SoundManager::stopStepSound() {
    if (_stepSoundActive && _begun) {
        noTone(_buzzerPin);
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
