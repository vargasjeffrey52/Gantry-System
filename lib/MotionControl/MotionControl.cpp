#include <Arduino.h>
#include "MotionControl.h"
#include <ctype.h> // Include the ctype library for character functions
#include "globals.h"

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

/*
*/
void MotionControl::moveLine(float x0, float y0, float x1, float y1){
    if (abs(x1 - x0) > abs(y1 - y0)){
        moveHline(x0, y0, x1, y1);
    } else{
        moveVline(x0, y0, x1, y1);
    }

} 




// void MotionControl::moveTo(float x, float y){
    
// }
void MotionControl::moveTo(int x0, int x1, int y0, int y1, int dt){
    //TODO: need a to call on stepperMotor.cpp to move a single step at a time.
    if (x0 != x1){
        _motorX.SingleStep(dt);
    }
    if (y0 != y1){
        _motorY1.SingleStep(dt);
    }
}



// x0 , y0, x1 and y1 are in units of mm or inches. 
void MotionControl::moveHline (float x0, float y0, float x1, float y1){
    _motorX.setDirection(x0<x1);
    _motorY1.setDirection(y0<y1);
    _motorX.enable();
    _motorY1.enable();
    _motorY2.enable();


    //TODO: Will first configure this to work in units of mm. ill figure out how to make it work with inches later
    // convert all values to units of steps before interpolating velocity and distances. 
    //note Feedrate is normally given in units of mm/min, or in/min so that requires another conversion 
    // Acceleration normally given in units of mm/s^2 or
    long x_0 = convertToSteps(x0);
    long x_1 = convertToSteps(x1);
    long y_0 = convertToSteps(y0);
    long y_1 = convertToSteps(y1);
    //TODO: currently assuming FEEDRATE is in units of mm/min and converting to steps per/sec (need to fix for inch/sec)
    double Vmax  = (FEEDRATE/60.0) * STEPS_PER_MM;
    //Vmax = 100;

    // figure out acceleration distance to reach vmax and deceleration from vmax to 0
    
    // ********FROM THIS POINT ON EVERYTHING SHOULD BE IN UNITS OF STEPS, STEPS/S or STEPS/s^2 *******
    double vi = 0; // starting velocity in units of steps/sec
    double acc = ACCELERATION * STEPS_PER_MM;
    //acc = 200;
    
    double iacc = (Vmax * Vmax )/(2*acc); //TODO: assuming acceeleration is in units of mm/s^2 change so that its more generic
    //iacc = iacc * STEPS_PER_MM; //converting to steps
    double idcc = iacc; // deceleration distance must be symetric since time spend accelerating much be = to time spend decelerating for smooth movement
    
    
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
    

    
    // takes care of not enough distance to accelerate to vmax ie
    if ((dx - iacc - idcc) < 0 ){
        iacc = dx/2.0;
        idcc = iacc;
    }
    
    // dx in units of STEPS
    if (dx != 0 ){
        long y = y_0;
        long yPrev = y;
        long D = 2*dy - dx;
        // Serial.println(String(x_0) + ", " + String(y_0) + ", " + String(x_1) + ", " + String(y_1));
        // moves in units of steps.
        for (long i = 0; i <= dx; i++){ 
            
            if (i < iacc-1){
                // accelerate
                vi = sqrt(2 * acc * (i+1));
            }else if ( i > dx - idcc -1){
                // decelerate
                vi = sqrt(2 * acc *(dx-i +1));
            }
            else {
                // constant velocity
                vi = Vmax;
            }
            // convert from steps/s to scc/step for delay time.
            long dt = round((1e6/vi));
            // Serial.println("(" + String(i)+ ", " + String(y) +", " + String(dt) +")"); // printing each stept to check if it correctly generates a line.

            moveTo(x_0, x_0 + i, yPrev, y, dt); // move x first
            yPrev = y;
            if (D >= 0){
                y += dir;
                D = D - 2*dx;
            }
            D = D + 2*dy;
            
        }

    }
    _motorX.disable();
    _motorY1.disable();

}


// x0 , y0, x1 and y1 are in units of mm or inches. 
void MotionControl::moveVline (float x0, float y0, float x1, float y1){
    _motorX.setDirection(x0<x1);
    _motorY1.setDirection(y0<y1);
    _motorX.enable();
    _motorY1.enable();
    _motorY2.enable();

    //TODO: Will first configure this to work in units of mm. ill figure out how to make it work with inches later
    // convert all values to units of steps before interpolating velocity and distances. 
    //note Feedrate is normally given in units of mm/min, or in/min so that requires another conversion 
    // Acceleration normally given in units of mm/s^2 or
    long x_0 = convertToSteps(x0);
    long x_1 = convertToSteps(x1);
    long y_0 = convertToSteps(y0);
    long y_1 = convertToSteps(y1);
    //TODO: currently assuming FEEDRATE is in units of mm/min and converting to steps per/sec (need to fix for inch/sec)
    double Vmax  = (FEEDRATE/60.0) * STEPS_PER_MM;
    //Vmax = 100;

    // figure out acceleration distance to reach vmax and deceleration from vmax to 0
    
    // ********FROM THIS POINT ON EVERYTHING SHOULD BE IN UNITS OF STEPS, STEPS/S or STEPS/s^2 *******
    double vi = 0; // starting velocity in units of steps/sec
    double acc = ACCELERATION * STEPS_PER_MM;
    //acc = 200;
    
    double iacc = (Vmax * Vmax )/(2*acc); //TODO: assuming acceeleration is in units of mm/s^2 change so that its more generic
    //iacc = iacc * STEPS_PER_MM; //converting to steps
    double idcc = iacc; // deceleration distance must be symetric since time spend accelerating much be = to time spend decelerating for smooth movement
    
    
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
    
    // takes care of not enough distance to accelerate to vmax ie
    if ((dy - iacc - idcc) < 0 ){
        iacc = dy/2.0;
        idcc = iacc;
    }
    // dx in units of STEPS
    if (dy != 0 ){
        long x = x_0;
        long xPrev = x;
        long D = 2*dx - dy;
        // Serial.println(String(x_0) + ", " + String(y_0) + ", " + String(x_1) + ", " + String(y_1));
        // moves in units of steps.
        for (long i = 0; i <= dy; i++){ 
            
            if (i < iacc -1){
                // accelerate
                vi = sqrt(2 * acc * (i+1));
            }else if ( i > dy - idcc -1){
                // decelerate
                vi = sqrt(2 * acc *(dy-i +1));
            }
            else {
                // constant velocity
                vi = Vmax;
            }
            // convert from steps/s to scc/step for delay time.
            long dt = round((1e6/vi));
            // Serial.println("(" + String(x)+ ", " + String(i) +", " + String(dt) +")"); // printing each stept to check if it correctly generates a line.

            moveTo(xPrev, x, y_0, y_0 + i, dt); // move x first
            xPrev = x;
            if (D >= 0){
                x += dir;
                D = D - 2*dy;
            }
            D = D + 2*dx;
            
        }

    }
    _motorX.disable();
    _motorY1.disable();

}





int MotionControl::convertToSteps(float x){
    //TODO: ASSUMES NO MICROSTEPPING. i.e.  1step in the program corresponds to 1 physical step on the driver/motor. 
    //TODO: the driver (TMC2209) starts by microstepping i.e. need to fix this method eventually.
    if(CURRENT_UNIT == 0){
        //DEFAULT 0 means x is in mm hence convert using global variable
        return (int) round(x * STEPS_PER_MM);   
    } else if (CURRENT_UNIT == 1)
    {
        // 1 means x is in units of Inches hence convert using global varialble
        return (int) round(x * STEPS_PER_INCH);
    } else {
        // anything else will be considered units of steps howerver in globals i defined it as -1 being units of steps
        return (int) x;
    }
    
    // return isInInches ? round(x * STEPS_PER_INCH) : round(x * STEPS_PER_MM);
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
