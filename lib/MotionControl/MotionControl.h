#ifndef MOTIONCONTROL_H
#define MOTIONCONTROL_H

#include "StepperMotor.h"
#include "SoundManager.h"
#include "RgbStatus.h"
#include <ctype.h> // Include the ctype library for character functions
#include "globals.h"

class MotionControl {
public:
    // Constructor
    MotionControl(StepperMotor& motorX, StepperMotor& motorY1,
                  SoundManager& soundManager, RgbStatus& rgbStatus);

    // Core Methods
    void moveNsteps(int n, char axis);
    //
    
    
    //void moveLine(int x0, int y0, int x1, int y1);
    void moveLine(float x0, float y0, float x1, float y1, float feedrate);
    //void moveTo(float x, float y);
    void moveTo(int x0, int x1, int y0, int y1, int dt, float speedRatio);
    bool moveArc(float startX, float startY, float endX, float endY,
                 float centerX, float centerY, float feedrate, bool clockwise);
    void moveBy(float dx, float dy);
    void home();

    // Movement Utilities
    void setSpeed(float speed);
    void setAcceleration(float acceleration);
    bool isMoving();
    void stop();

    // Position Tracking
    float getCurrentX();
    float getCurrentY();
    void resetPosition(float x = 0, float y = 0);

    // Safety and Limits (Optional)
    void setLimits(float maxX, float maxY);
    bool isWithinLimits(float x, float y);

    // Debugging
    void printStatus();

private:
    void moveHline (float x0, float y0, float x1, float y1,
                    float feedrate, float acceleration);
    void moveVline (float x0, float y0, float x1, float y1,
                    float feedrate, float acceleration);
    int convertToSteps(float x, char axis);
    StepperMotor& _motorX;
    StepperMotor& _motorY1;
    SoundManager& _soundManager;
    RgbStatus& _rgbStatus;

    float _currentX;
    float _currentY;

    float _speed;
    float _acceleration;

    float _maxX;
    bool _isHomed;
};

#endif // MOTIONCONTROL_H
