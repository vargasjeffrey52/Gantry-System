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