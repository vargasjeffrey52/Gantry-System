#include <Arduino.h>
class StepperMotor {
public:
    // Constructor
    StepperMotor(int stepPin, int dirPin, int enPin, int ms1, int ms2);

    // Core methods
    void setSpeed(float speed);
    void step(bool state); // takes a half step. with no delay
    void step(int step_delay); // takes a single step and updates step delay.
    void MultiStep(int steps); // steps n steps 
    void setDirection(bool dir);
    bool getDirection();
    void enable();
    void disable();

    // Timing and step control
    void setStepMode(int microsteps);
    void setStepDelay(float delay);
    void delayStep(); // pauses motor to delayed time

    // Diagnostics and state tracking
    bool isMoving();
    int getCurrentPosition();
    void resetPosition();

    // Advanced features
    void accelerateTo(int targetSpeed, float acceleration);
    void decelerateToStop();
    void runToPosition(int targetPosition);
    void stop();

    // Helper methods
    bool isEnabled();
    void invertDirection(bool invert);
    void invertEnable(bool invert);

    // Debugging utilities
    void printStatus();

private:
    int _stepPin, _dirPin, _enPin, _ms1,_ms2;
    float _stepDelay;  // Delay between steps
    int _currentPosition; // Tracks current position
    bool _isEnabled;   // Tracks enable state
    bool _direction;   // Current direction
};