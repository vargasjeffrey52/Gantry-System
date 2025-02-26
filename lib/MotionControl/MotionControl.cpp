#include "MotionControl.h"

MotionControl::MotionControl(StepperMotor& motorX, StepperMotor& motorY1, StepperMotor& motorY2)
 :  _motorX(motorX),   // Initialize _motorX with motorX
    _motorY1(motorY1), // Initialize _motorY1 with motorY1
    _motorY2(motorY2)  // Initialize _motorY2 with motorY2
{
  
}

// Core Methods
// individual movements of each axis
void MotionControl::moveNsteps(int n, char axis){
    bool dir = (n >= 0) ? true : false;


    axis = toupper(axis);
    for (int i=0; i < abs(n); i++) {
    if (axis == 'X'){
        // a transition from true to false is a step
        _motorX.setDirection(dir);
        _motorX.step(true); 
        _motorX.delayStep();

        _motorX.step(false);
        _motorX.delayStep();
    } else if( axis == 'Y'){
        _motorY1.setDirection(dir);
        _motorY2.setDirection(dir);
        _motorY1.step(true);
        _motorY2.step(true);
        _motorY1.delayStep();

        _motorY1.step(false);
        _motorY2.step(false);
        _motorY1.delayStep();

    }
  }
    
}

// //TODO: still need to implement all these methods. 
// void moveLine(int x0, int y0, int x1, int y1);
// void moveTo(float x, float y);
// void moveBy(float dx, float dy);
// void moveArc(float centerX, float centerY, float radius, float startAngle, float endAngle, bool clockwise);
// void home();

// // Movement Utilities
// void setSpeed(float speed);
// void setAcceleration(float acceleration);
// bool isMoving();
// void stop();

// // Position Tracking
// float getCurrentX();
// float getCurrentY();
// void resetPosition(float x = 0, float y = 0);

// // Safety and Limits (Optional)
// void setLimits(float maxX, float maxY);
// bool isWithinLimits(float x, float y);

// // Debugging
// void printStatus();
