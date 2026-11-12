#include <Arduino.h>
#include "MotionControl.h"
#include <ctype.h> // Include the ctype library for character functions
#include <math.h>
#include <limits.h>
#include "globals.h"

namespace {
constexpr double ARC_PI = 3.14159265358979323846;
constexpr uint32_t MAX_ARC_SAMPLES = 1000000;
}

MotionControl::MotionControl(StepperMotor& motorX, StepperMotor& motorY1,
                             SoundManager& soundManager, RgbStatus& rgbStatus)
 :  _motorX(motorX),
    _motorY1(motorY1),
    _soundManager(soundManager),
    _rgbStatus(rgbStatus),
    _safeMoveEnabled(true),
    _xLimitWasActive(false),
    _yLimitWasActive(false),
    _crashEvent(false)
{
  
}

void MotionControl::begin() {
    // Closed normally-closed switches ground the inputs; pull-ups mark opens HIGH.
    pinMode(X_LIMIT_PIN, INPUT_PULLUP);
    pinMode(Y_LIMIT_PIN, INPUT_PULLUP);
}

void MotionControl::updateEndstopStatus() {
    checkLimitsAndStop();
}

// Core Methods
// individual movements of each axis
void MotionControl::moveNsteps(int n, char axis){
    bool dir = (n >= 0) ? true : false;


    axis = toupper(axis);
    for (int i=0; i < abs(n); i++) {
    _soundManager.updateStepSound(1.0f);
    if (axis == 'X'){
        // a transition from true to false is a step
        _motorX.setDirection(dir);
        _motorX.step(true); 
        _motorX.delayStep();

        _motorX.step(false);
        _motorX.delayStep();
    } else if( axis == 'Y'){
        _motorY1.setDirection(dir);
        _motorY1.step(true);
        _motorY1.delayStep();

        _motorY1.step(false);
        _motorY1.delayStep();
    }
    _soundManager.update();
    _rgbStatus.update();
}
    _soundManager.finishStepSound();

}

/*
*/
bool MotionControl::moveLine(float x0, float y0, float x1, float y1, float feedrate){
    const double deltaX = fabs(x1 - x0);
    const double deltaY = fabs(y1 - y0);
    const double pathLength = hypot(deltaX, deltaY);
    if (!isfinite(pathLength) || pathLength <= 0) {
        return true;
    }

    const double ratioX = deltaX / pathLength;
    const double ratioY = deltaY / pathLength;
    double maxPathSpeed = INFINITY;
    double pathAcceleration = INFINITY;
    if (ratioX > 0) {
        maxPathSpeed = fmin(maxPathSpeed,
                            MAX_FEEDRATE_X / (stepsPerUnit('X') * ratioX));
        pathAcceleration = fmin(pathAcceleration,
                                MAX_ACCELERATION_X / (stepsPerUnit('X') * ratioX));
    }
    if (ratioY > 0) {
        maxPathSpeed = fmin(maxPathSpeed,
                            MAX_FEEDRATE_Y / (stepsPerUnit('Y') * ratioY));
        pathAcceleration = fmin(pathAcceleration,
                                MAX_ACCELERATION_Y / (stepsPerUnit('Y') * ratioY));
    }

    const float limitedFeedrate = static_cast<float>(fmin(feedrate, maxPathSpeed));
    const float limitedAcceleration = static_cast<float>(pathAcceleration);
    if (!isfinite(limitedFeedrate) || limitedFeedrate <= 0 ||
        !isfinite(limitedAcceleration) || limitedAcceleration <= 0) {
        return true;
    }

    if (fabs(x1 - x0) > fabs(y1 - y0)){
        return moveHline(x0, y0, x1, y1, limitedFeedrate, limitedAcceleration);
    } else{
        return moveVline(x0, y0, x1, y1, limitedFeedrate, limitedAcceleration);
    }
}




// void MotionControl::moveTo(float x, float y){
    
// }
bool MotionControl::moveTo(int x0, int x1, int y0, int y1, int dt, float speedRatio){
    if (checkLimitsAndStop()) {
        return false;
    }

    //TODO: need a to call on stepperMotor.cpp to move a single step at a time.
    _soundManager.updateStepSound(speedRatio);
    if (x0 != x1){
        _motorX.SingleStep(dt);
        Xcurrent += (_motorX.getDirection() ? 1.0f : -1.0f) / stepsPerUnit('X');
        if (checkLimitsAndStop()) {
            return false;
        }
    }
    if (y0 != y1){
        _motorY1.SingleStep(dt);
        Ycurrent += (_motorY1.getDirection() ? 1.0f : -1.0f) / stepsPerUnit('Y');
        if (checkLimitsAndStop()) {
            return false;
        }
    }
    _soundManager.update();
    _rgbStatus.update();
    return true;
}

bool MotionControl::moveArc(float startX, float startY, float endX, float endY,
                            float centerX, float centerY, float feedrate, bool clockwise) {
    const double radius = hypot(startX - centerX, startY - centerY);
    const double maxFeedrate = fmin(MAX_FEEDRATE_X / stepsPerUnit('X'),
                                    MAX_FEEDRATE_Y / stepsPerUnit('Y'));
    const double maxAcceleration = fmin(MAX_ACCELERATION_X / stepsPerUnit('X'),
                                        MAX_ACCELERATION_Y / stepsPerUnit('Y'));
    if (!isfinite(radius) || radius <= 0 || !isfinite(feedrate) ||
        feedrate <= 0 || !isfinite(maxFeedrate) || maxFeedrate <= 0 ||
        !isfinite(maxAcceleration) || maxAcceleration <= 0) {
        return false;
    }

    const double startAngle = atan2(startY - centerY, startX - centerX);
    const double endAngle = atan2(endY - centerY, endX - centerX);
    double sweep = clockwise ? startAngle - endAngle : endAngle - startAngle;
    while (sweep < 0) {
        sweep += 2 * ARC_PI;
    }
    if (sweep < 1e-9) {
        sweep = 2 * ARC_PI;
    }

    const double pathLengthSteps = radius * sweep * fmax(stepsPerUnit('X'), stepsPerUnit('Y'));
    const double arcStepsPerUnit = fmax(stepsPerUnit('X'), stepsPerUnit('Y'));
    const double requestedMaxSpeed = fmin(feedrate, maxFeedrate) * arcStepsPerUnit;
    const double acceleration = maxAcceleration * arcStepsPerUnit;
    const double sampleCountValue = ceil(pathLengthSteps * 2.0);
    if (!isfinite(pathLengthSteps) || pathLengthSteps <= 0 ||
        !isfinite(sampleCountValue) || sampleCountValue > MAX_ARC_SAMPLES ||
        !isfinite(requestedMaxSpeed) || requestedMaxSpeed <= 0 ||
        !isfinite(acceleration) || acceleration <= 0) {
        return false;
    }

    const uint32_t sampleCount = static_cast<uint32_t>(sampleCountValue);
    double maxSpeed = requestedMaxSpeed;
    double accelerationDistance = (maxSpeed * maxSpeed) / (2 * acceleration);
    if (2 * accelerationDistance > pathLengthSteps) {
        maxSpeed = sqrt(acceleration * pathLengthSteps);
        accelerationDistance = pathLengthSteps / 2;
    }

    int previousX = convertToSteps(startX, 'X');
    int previousY = convertToSteps(startY, 'Y');
    const int finalX = convertToSteps(endX, 'X');
    const int finalY = convertToSteps(endY, 'Y');

    _motorX.enable();
    _motorY1.enable();

    for (uint32_t sample = 1; sample <= sampleCount; ++sample) {
        const double fraction = static_cast<double>(sample) / sampleCount;
        const double angle = startAngle + (clockwise ? -1 : 1) * sweep * fraction;
        const int nextX = (sample == sampleCount)
            ? finalX
            : convertToSteps(centerX + radius * cos(angle), 'X');
        const int nextY = (sample == sampleCount)
            ? finalY
            : convertToSteps(centerY + radius * sin(angle), 'Y');

        if (nextX != previousX) {
            _motorX.setDirection(nextX > previousX);
        }
        if (nextY != previousY) {
            _motorY1.setDirection(nextY > previousY);
        }

        const double distance = pathLengthSteps *
            (static_cast<double>(sample) - 0.5) / sampleCount;
        double speed = maxSpeed;
        if (distance < accelerationDistance) {
            speed = sqrt(2 * acceleration * distance);
        } else if (distance > pathLengthSteps - accelerationDistance) {
            speed = sqrt(2 * acceleration * (pathLengthSteps - distance));
        }
        speed = fmax(speed, 1.0);

        const double pulseDelayUs = 500000.0 / speed;
        const int stepDelay = pulseDelayUs >= INT_MAX
            ? INT_MAX
            : static_cast<int>(round(pulseDelayUs));
        if (!moveTo(previousX, nextX, previousY, nextY,
                    (stepDelay > 0) ? stepDelay : 1,
                    static_cast<float>(speed / requestedMaxSpeed))) {
            _soundManager.finishStepSound();
            return false;
        }

        previousX = nextX;
        previousY = nextY;
    }

    _soundManager.finishStepSound();
    return true;
}



// x0 , y0, x1 and y1 are in units of mm or inches. 
bool MotionControl::moveHline (float x0, float y0, float x1, float y1,
                               float feedrate, float acceleration){
    _motorX.setDirection(x0<x1);
    _motorY1.setDirection(y0<y1);
    _motorX.enable();
    _motorY1.enable();


    // Convert coordinates to steps before interpolating velocity and distance.
    long x_0 = convertToSteps(x0, 'X');
    long x_1 = convertToSteps(x1, 'X');
    long y_0 = convertToSteps(y0, 'Y');
    long y_1 = convertToSteps(y1, 'Y');
    const double pathLengthUnits = hypot(x1 - x0, y1 - y0);
    const double majorAxisUnitsPerPathUnit = pathLengthUnits > 0
        ? fabs(static_cast<double>(x1) - x0) / pathLengthUnits
        : 1;
    // Convert the major-axis component of the active-unit feedrate to steps/second.
    double Vmax  = feedrate * majorAxisUnitsPerPathUnit * stepsPerUnit('X');
    //Vmax = 100;

    // figure out acceleration distance to reach vmax and deceleration from vmax to 0
    
    // ********FROM THIS POINT ON EVERYTHING SHOULD BE IN UNITS OF STEPS, STEPS/S or STEPS/s^2 *******
    double vi = 0; // starting velocity in units of steps/sec
    double acc = acceleration * majorAxisUnitsPerPathUnit * stepsPerUnit('X');
    //acc = 200;
    
    // alows me to draw the line starting from the endpoint to the startpoint if slope is negative
    // takes care of negative dx
    if (x_0 > x_1){
        double tempX = x_0;
        double tempY = y_0;
        x_0 = x_1;
        x_1 = tempX;
        y_0 = y_1;
        y_1 = tempY;
    } 
    
    // calculate the number of steps to take based on dx for horizontal line
    long dx = x_1 - x_0;
    long dy = y_1 - y_0;
    // direction of the line
    long dir = (dy >= 0) ? 1 : -1;
    // if slope is negative make it positive and draw line from endpoint to startpoint
    // takes care of negative dy
    dy *= dir;
    

    
    // dx in units of STEPS
    bool completed = true;
    if (dx != 0 ){
        long y = y_0;
        long yPrev = y;
        long D = 2*dy - dx;
        // Serial.println(String(x_0) + ", " + String(y_0) + ", " + String(x_1) + ", " + String(y_1));
        // moves in units of steps.
        for (long i = 0; i <= dx; i++){ 
            const double progress = (i == 0) ? 0.5 : i - 0.5;
            const double remaining = dx - progress;
            vi = fmin(Vmax, sqrt(2 * acc * fmin(progress, remaining)));
            // convert from steps/s to scc/step for delay time.
            long dt = round(500000.0/vi);
            // Serial.println("(" + String(i)+ ", " + String(y) +", " + String(dt) +")"); // printing each stept to check if it correctly generates a line.

            if (!moveTo(x_0, x_0 + i, yPrev, y, dt,
                        (Vmax > 0) ? vi / Vmax : 0)) {
                completed = false;
                break;
            }
            yPrev = y;
            if (D >= 0){
                y += dir;
                D = D - 2*dx;
            }
            D = D + 2*dy;
            
        }

    }
    _soundManager.finishStepSound();
    return completed;
}


// x0 , y0, x1 and y1 are in units of mm or inches. 
bool MotionControl::moveVline (float x0, float y0, float x1, float y1,
                               float feedrate, float acceleration){
    _motorX.setDirection(x0<x1);
    _motorY1.setDirection(y0<y1);
    _motorX.enable();
    _motorY1.enable();

    // Convert coordinates to steps before interpolating velocity and distance.
    long x_0 = convertToSteps(x0, 'X');
    long x_1 = convertToSteps(x1, 'X');
    long y_0 = convertToSteps(y0, 'Y');
    long y_1 = convertToSteps(y1, 'Y');
    const double pathLengthUnits = hypot(x1 - x0, y1 - y0);
    const double majorAxisUnitsPerPathUnit = pathLengthUnits > 0
        ? fabs(static_cast<double>(y1) - y0) / pathLengthUnits
        : 1;
    // Convert the major-axis component of the active-unit feedrate to steps/second.
    double Vmax  = feedrate * majorAxisUnitsPerPathUnit * stepsPerUnit('Y');
    //Vmax = 100;

    // figure out acceleration distance to reach vmax and deceleration from vmax to 0
    
    // ********FROM THIS POINT ON EVERYTHING SHOULD BE IN UNITS OF STEPS, STEPS/S or STEPS/s^2 *******
    double vi = 0; // starting velocity in units of steps/sec
    double acc = acceleration * majorAxisUnitsPerPathUnit * stepsPerUnit('Y');
    //acc = 200;
    
    // alows me to draw the line starting from the endpoint to the startpoint if slope is negative
    // takes care of negative dx
    if (y_0 > y_1){
        double tempX = x_0;
        double tempY = y_0;
        x_0 = x_1;
        x_1 = tempX;
        y_0 = y_1;
        y_1 = tempY;
    } 
    
    // calculate the number of steps to take based on dx for horizontal line
    long dx = x_1 - x_0;
    long dy = y_1 - y_0;
    // direction of the line
    long dir = (dx >= 0) ? 1 : -1;
    // if slope is negative make it positive and draw line from endpoint to startpoint
    // takes care of negative dy
    dx *= dir;
    
    // dx in units of STEPS
    bool completed = true;
    if (dy != 0 ){
        long x = x_0;
        long xPrev = x;
        long D = 2*dx - dy;
        // Serial.println(String(x_0) + ", " + String(y_0) + ", " + String(x_1) + ", " + String(y_1));
        // moves in units of steps.
        for (long i = 0; i <= dy; i++){ 
            const double progress = (i == 0) ? 0.5 : i - 0.5;
            const double remaining = dy - progress;
            vi = fmin(Vmax, sqrt(2 * acc * fmin(progress, remaining)));
            // convert from steps/s to scc/step for delay time.
            long dt = round(500000.0/vi);
            // Serial.println("(" + String(x)+ ", " + String(i) +", " + String(dt) +")"); // printing each stept to check if it correctly generates a line.

            if (!moveTo(xPrev, x, y_0, y_0 + i, dt,
                        (Vmax > 0) ? vi / Vmax : 0)) {
                completed = false;
                break;
            }
            xPrev = x;
            if (D >= 0){
                x += dir;
                D = D - 2*dy;
            }
            D = D + 2*dx;
            
        }

    }
    _soundManager.finishStepSound();
    return completed;
}

bool MotionControl::checkLimitsAndStop() {
    const bool xLimitActive = digitalRead(X_LIMIT_PIN) == HIGH;
    const bool yLimitActive = digitalRead(Y_LIMIT_PIN) == HIGH;
    const bool limitActive = xLimitActive || yLimitActive;
    _rgbStatus.setCrashActive(limitActive);
    if ((xLimitActive && !_xLimitWasActive) ||
        (yLimitActive && !_yLimitWasActive)) {
        _crashEvent = true;
        _soundManager.playDeathMelody();
    }
    _xLimitWasActive = xLimitActive;
    _yLimitWasActive = yLimitActive;

    if (!limitActive || !_safeMoveEnabled) {
        return false;
    }

    _motorX.stop();
    _motorY1.stop();
    return true;
}

void MotionControl::setSafeMoveEnabled(bool enabled) {
    if (_safeMoveEnabled != enabled) {
        _xLimitWasActive = false;
        _yLimitWasActive = false;
    }
    _safeMoveEnabled = enabled;
}

bool MotionControl::consumeCrashEvent() {
    const bool crashEvent = _crashEvent;
    _crashEvent = false;
    return crashEvent;
}




int MotionControl::convertToSteps(float x, char axis){
    const float axisStepsPerUnit = stepsPerUnit(axis);
    return static_cast<int>(round(x * axisStepsPerUnit));
}


// //TODO: still need to implement all these methods. 
// void moveLine(int x0, int y0, int x_1, int y1);
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
