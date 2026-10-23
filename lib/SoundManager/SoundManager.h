#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <Arduino.h>

class SoundManager {
public:
    explicit SoundManager(int buzzerPin);

    void begin();
    void update();
    void setEnabled(bool enabled);
    void setStepSoundEnabled(bool enabled);
    void playStartupMelody();
    void playCompletionMelody();
    void playSingleTone();
    void playRandomTone();
    void playCrashMelody();
    void updateStepSound(float speedRatio);
    void finishStepSound();

private:
    struct Note {
        uint16_t frequency;
        uint16_t durationMs;
        uint16_t pauseMs;
    };

    void startMelody(const Note* notes, size_t noteCount);
    void stopPlayback();
    void stopStepSound();
    void startQueuedMelody();

    int _buzzerPin;
    const Note* _notes;
    const Note* _queuedNotes;
    size_t _noteCount;
    size_t _queuedNoteCount;
    size_t _noteIndex;
    uint32_t _noteStartedAt;
    uint32_t _nextNoteAt;
    uint32_t _lastStepSoundUpdateAt;
    uint32_t _stepSoundStopStartedAt;
    uint16_t _stepSoundFrequency;
    uint16_t _stepSoundStopFrequency;
    Note _oneShotNote;
    bool _enabled;
    bool _stepSoundEnabled;
    bool _noteActive;
    bool _stepSoundActive;
    bool _stepSoundStopping;
    bool _begun;
};

#endif
