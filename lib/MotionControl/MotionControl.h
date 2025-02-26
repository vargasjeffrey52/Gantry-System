#include "StepperMotor.h"
#include <ctype.h> // Include the ctype library for character functions

class MotionControl {
public:
    // Constructor
    MotionControl(StepperMotor& motorX, StepperMotor& motorY1, StepperMotor& motorY2);

    // Core Methods
    void moveNsteps(int n, char axis);
    void moveLine(int x0, int y0, int x1, int y1);
    void moveTo(float x, float y);
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
    StepperMotor& _motorX;
    StepperMotor& _motorY1;
    StepperMotor& _motorY2;

    float _currentX;
    float _currentY;

    float _speed;
    float _acceleration;

    float _maxX;
    float _maxY;

    bool _isHomed;
};
