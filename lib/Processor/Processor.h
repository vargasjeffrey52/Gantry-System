#include <Arduino.h>
#include "Parser.h"
#include "globals.h"
#include "MotionControl.h"

class Processor {
public:
    //Processor() = default;
    Processor();
    bool is_movement_command(Parser::g_code_command gcode);
    bool is_modifier_command(Parser::g_code_command gcode);
    bool is_getter_command(Parser::g_code_command gcode);

    void send_to_parser(String command);
    void send_to_motion_control(Parser::g_code_command gcode);
    void send_to_modifier(Parser::g_code_command gcode);
    void send_to_getter(Parser::g_code_command gcode);


private:
    String commandsArr[10]; // size of array for parsing G-code string command
    StepperMotor motorX;
    StepperMotor motorY1;
    StepperMotor motorY2;
    
    MotionControl M_Control;
};