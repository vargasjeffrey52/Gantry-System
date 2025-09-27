#include "StepperMotor.h"

//Constructor for instance of a stepper motor
StepperMotor::StepperMotor(int stepPin, int dirPin, int enPin, int ms1, int ms2){
    Serial.println("STEPPER CONSTRUCTOR");
    Serial.println("EN: " + String(enPin) + ", " + "dir: " + String(dirPin) + " step: " + String(stepPin) );
    _stepPin = stepPin;
    _dirPin = dirPin;
    _enPin = enPin;
    _ms1 = ms1;
    _ms2 = ms2;
    _stepDelay = 100; // 100 us delay between step (default)
    pinMode(enPin, OUTPUT);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    pinMode(ms1, OUTPUT);
    pinMode(ms2, OUTPUT);
    setStepMode(8); // Default set 1/8 microstep by default.
    disable();
}

//TODO: Implement speed control
void StepperMotor::setSpeed(float speed){

}


void StepperMotor::step(bool state){
    if (state == true)
    {
        digitalWrite(StepperMotor::_stepPin, HIGH);

    } else {
        digitalWrite(StepperMotor::_stepPin, LOW);

    }
    

}
void StepperMotor::SingleStep(int step_delay){
    enable();
    digitalWrite(StepperMotor::_stepPin, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(StepperMotor::_stepPin, LOW);
    delayMicroseconds(step_delay);


}
void StepperMotor::MultiStep(int steps){
    for (int i = 0; i < steps; i++)
    {
        digitalWrite(StepperMotor::_stepPin, HIGH);
        delayMicroseconds(StepperMotor::_stepDelay);
        digitalWrite(StepperMotor::_stepPin, LOW);
        delayMicroseconds(StepperMotor::_stepDelay);
    
    }

}

void StepperMotor::delayStep(){
    delayMicroseconds(StepperMotor::_stepDelay);
}


void StepperMotor::setDirection(bool dir){
    StepperMotor::_direction = dir;
    if (StepperMotor::_direction == true){
        digitalWrite(StepperMotor::_dirPin, HIGH);

    } else {
        digitalWrite(StepperMotor::_dirPin, LOW);

    }
}

bool StepperMotor::getDirection(){
    return StepperMotor::_direction;
}

void StepperMotor::enable(){
    digitalWrite(StepperMotor::_enPin, LOW);
}
void StepperMotor::disable(){
    digitalWrite(StepperMotor::_enPin, HIGH);
}

// Timing and step control
void StepperMotor::setStepMode(int microsteps){
    // 1/8 microstep
    
    if (microsteps == 16)
    {
        digitalWrite(StepperMotor::_ms1, HIGH);
        digitalWrite(StepperMotor::_ms2, HIGH);

    } else if (microsteps == 32)
    {
        digitalWrite(StepperMotor::_ms1, HIGH);
        digitalWrite(StepperMotor::_ms2, LOW);

    } else if (microsteps == 64)
    {
        digitalWrite(StepperMotor::_ms1, LOW);
        digitalWrite(StepperMotor::_ms2, HIGH);

    } else {
    digitalWrite(StepperMotor::_ms1, LOW);
    digitalWrite(StepperMotor::_ms2, LOW);
    
    }
    
    
    
}
void StepperMotor::setStepDelay(float delay){
    StepperMotor::_stepDelay = delay;
}

//TODO: Implement Diagnostics and state tracking
//TODO: Implement isMoving()
bool StepperMotor::isMoving(){
    return _isMoving;
}
//TODO: Implement getCurrentPosition
int StepperMotor::getCurrentPosition(){
    return 0;
}
//TODO: Implement resetPosition
void StepperMotor::resetPosition(){}

//TODO: implement Advanced features
//TODO: Implement Acceleration
void StepperMotor::accelerateTo(int targetSpeed, float acceleration){

}
//TODO: Implement deceleration
void StepperMotor::decelerateToStop(){

}
//TODO: Implement runToPosition
void StepperMotor::runToPosition(int targetPosition){

}
//TODO: Implement Stop
void StepperMotor::stop(){
    /*Must stop instantly if crashed make use of the interupt pins*/
}

// Helper methods
bool StepperMotor::isEnabled(){
    return _isEnabled;
}

void invertDirection(bool invert);
void invertEnable(bool invert);

// Debugging utilities
void printStatus();
