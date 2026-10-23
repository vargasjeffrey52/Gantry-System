#include "Processor.h"
#include "Parser.h"
#include "globals.h"
#include "MotionControl.h"

Processor::Processor()
    : motorX(STEPX, DIRX, ENAX, MS1X, MS2X),
      motorY1(STEPY, DIRY, ENAY1, MS1X, MS2X),
      M_Control(motorX, motorY1)
{
    Serial.println("PROCESSOR INIT");
}

bool Processor::send_to_parser(String command){
    int size = PARSER.command_to_arr(command, ' ', commandsArr);
        // Serial.print("Size of array: "); // debugging
        // Serial.println(size); // debugging

        // check main command Ex. G0, G1, G2, G3,...
        if (PARSER.validate_main_command(commandsArr, size)) {
            // Serial.println(PARSER.getSystemMessage()); // debugging

            // check if command requires parameters Ex. X, Y, Z, F, ...
            if (PARSER.requiresParams(commandsArr)) {
               // Serial.println(PARSER.getSystemMessage()); // debugging

                // check if parameters have values Ex. X10, Y20, Z30, F40, empty and not invalid char
                if (PARSER.validate_params_with_values(commandsArr, size)) {
                    // Serial.println(PARSER.getSystemMessage()); // debugging
                    //gcode = parser.create_gcode_object(commandsArr, size);
                    GCODE = PARSER.create_gcode_object(commandsArr, size);
                    //print_Gcode_object(GCODE); 
                    enqueue_gcode(GCODE);
                    return true;
                    
                    
                }
                // bad values on params
                else {
                    Serial.println(PARSER.getSystemMessage()); // debugging
                    return false;
                }
            } 
            // command does not require parameters
            else {
                if (size > 1){
                    Serial.println("Error: " + commandsArr[0] + " does not take parameters");
                    //Serial.println(PARSER.getSystemMessage()); // debugging
                    return false;
                }
                GCODE = PARSER.create_gcode_object(commandsArr, size);
                //print_Gcode_object(GCODE);
                enqueue_gcode(GCODE);
                return true;
            }
        }
        // bad main command
        else {
            Serial.println(PARSER.getSystemMessage()); // debugging
            // Serial.println();
            // Serial.println("The following G-code commands was dequeued: ");
            // print_Gcode_object(dequeue_gcode());
            return false;
            
        }

}


bool Processor::is_movement_command(Parser::g_code_command gcode){
    if (gcode.main_command == "G0" || gcode.main_command == "G1" || gcode.main_command == "G2" || gcode.main_command == "G3"){
        return true;
    }
    return false;
}

bool Processor::is_modifier_command(Parser::g_code_command gcode){
    if (gcode.main_command == "G90" || gcode.main_command == "G91" || gcode.main_command == "M502" || gcode.main_command == "M114" || gcode.main_command == "M119"){
        return true;
    }
    return false;
}

bool Processor::is_getter_command(Parser::g_code_command gcode){
    if (gcode.main_command == "M114" || gcode.main_command == "M119"){
        return true;
    }
    return false;
}

void Processor::send_to_motion_control(Parser::g_code_command gcode){
    // do something
    float x0 = 0, y0 = 0;
    String cmd = gcode.main_command;

    if (cmd == "G2" || cmd == "G3") {
        Serial.println("Error: arcs (G2/G3) are not implemented yet");
        return;
    }

    if (gcode.F != MIN_INT){
        FEEDRATE = gcode.F * stepsPerUnit() / STEPS_PER_MM;
    }

    float feed = (cmd == "G0") ? MAX_FEEDRATE_MM : FEEDRATE;
    float x1 = (gcode.X != MIN_INT) ? gcode.X : x0;
    float y1 = (gcode.Y != MIN_INT) ? gcode.Y : y0;

    M_Control.moveLine(x0, y0, x1, y1, feed);
}

void Processor::send_to_modifier(Parser::g_code_command gcode){
    // do something
    Serial.println();
    Serial.println("Sending to modifier ##############");
    Serial.println();
}
void Processor::send_to_getter(Parser::g_code_command gcode){
    // do something
    Serial.println();
    Serial.println("Sending to getter ##############");
    Serial.println();
}