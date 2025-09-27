#ifndef MOTIONCONTROL_H
#define MOTIONCONTROL_H

#include "StepperMotor.h"
#include <ctype.h> // Include the ctype library for character functions
#include "globals.h"

class MotionControl {
public:
    // Constructor
    MotionControl(StepperMotor& motorX, StepperMotor& motorY1, StepperMotor& motorY2);

    // Core Methods
    void moveNsteps(int n, char axis);
    //
    
    
    //void moveLine(int x0, int y0, int x1, int y1);
    void moveLine(float x0, float y0, float x1, float y1, float resolution);
    void moveLine(float x0, float y0, float x1, float y1);
    //void moveTo(float x, float y);
    void moveTo(int x0, int x1, int y0, int y1, int dt);
    void moveBy(float dx, float dy);
    void moveArc(float centerX, float centerY, float radius, float startAngle, float endAngle, bool clockwise);
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
    void moveHline (float x0, float y0, float x1, float y1);
    void moveVline (float x0, float y0, float x1, float y1);
    void moveVline (float x0, float y0, float x1, float y1,  float resolution);
    int convertToSteps(float x);
    StepperMotor& _motorX;
    StepperMotor& _motorY1;
    StepperMotor& _motorY2;

    float _currentX;
    float _currentY;

    float _speed;
    float _acceleration;

    float _maxX;
    bool _isHomed;
};

#endif // MOTIONCONTROL_H

