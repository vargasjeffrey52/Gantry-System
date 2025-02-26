#include "Processor.h"
#include "globals.h"



void Processor::send_to_parser(String command){
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
                    print_Gcode_object(GCODE); 
                    enqueue_gcode(GCODE);                
                    
                    
                }
                // bad values on params
                else {
                    Serial.println(PARSER.getSystemMessage()); // debugging
                }
            } 
            // command does not require parameters
            else {
                Serial.println(PARSER.getSystemMessage()); // debugging
            }
        }
        // bad main command
        else {
            Serial.println(PARSER.getSystemMessage()); // debugging
            Serial.println();
            Serial.println("The following G-code commands was dequeued: ");
            print_Gcode_object(dequeue_gcode());
            
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
    Serial.println("Sending to motion control ##############");
}

void Processor::send_to_modifier(Parser::g_code_command gcode){
    // do something
    Serial.println("Sending to modifier ##############");
}
void Processor::send_to_getter(Parser::g_code_command gcode){
    // do something
    Serial.println("Sending to getter ##############");
}