#include "Parser.h"
#include "globals.h"

// Define the static members
const StringType Parser::validCommandsWParams[] = {"G0", "G1", "G2", "G3", "G92", "G28", "M17", "M18", "M203", "M201"};
const StringType Parser::validCommandsWoutParams[] = {"G90", "G91", "M502", "M114", "M119"};

// command to arr (split by ' ')
int Parser::command_to_arr(const StringType& command, char delimiter, StringType* commandsArr) {
    int startindx = 0;
    int endindx = 0;
    int arrindx = 0;
    for (int i = 0; i < command.length(); i++) {
        if (command[i] == delimiter) {
            endindx = i;
            #ifdef ARDUINO
                commandsArr[arrindx] = command.substring(startindx, endindx);
            #else
                commandsArr[arrindx] = command.substr(startindx, endindx - startindx);
            #endif
            startindx = i + 1;
            arrindx++;
        }
    }
    // add what is left in the command array 
    if (command.length() > 0){
        String temp = command.substring(endindx); 
        temp.trim();
        commandsArr[arrindx] = temp;
        arrindx++;
    }
    // if (arrindx == 0 && command.length() > 0) {
    //     commandsArr[arrindx] = command;
    //     arrindx++;
    // }

    return arrindx;
}

// Given an array of G-code and its params it Validate Main Command
bool Parser::validate_main_command(StringType* cmd_Array, int size) {
    if (size < 1){
        _systemMessage = "Error: Command is empty";
        return false;
    } 
    for (const StringType& validCMD : validCommandsWParams) {
        if (cmd_Array[0] == validCMD) {
            _systemMessage = "Ok: Main Command is valid";
            return true;
        }
    }
    for (const StringType& validCMD : validCommandsWoutParams) {
        if (cmd_Array[0] == validCMD) {
            _systemMessage = "Ok: Main Command is valid";
            return true;
        }
    }
    _systemMessage = "Error: Main Command is invalid " + cmd_Array[0];
    return false;
}

// Given an array of G-code and its params it checks if the command requires parameters
// must perform a validate_main_command before this
bool Parser::requiresParams(const StringType* cmd_Array) {
    for (const StringType& validCMD : validCommandsWParams) {
        if (cmd_Array[0] == validCMD){
            _systemMessage = "Ok: Command requires parameters";
            return true;
        } 
    }
    _systemMessage = "Ok: Command does not require parameters"; 
    return false;    
}
/*Some G-code commands */
bool Parser::validate_params_with_values(const StringType* cmd_Array, int size) {
    // itterate through each param and check if it is valid
    for (int i = 1; i < size; i++) {
        // check first character of param
        if (cmd_Array[i][0] == 'X' || cmd_Array[i][0] == 'Y' || cmd_Array[i][0] == 'Z' || cmd_Array[i][0] == 'F' || cmd_Array[i][0] == 'I' || cmd_Array[i][0] == 'J' || cmd_Array[i][0] == 'R') {
            // if its a valid param but no value is given then do nothing default value will be given when create_goce_object is called
            if (cmd_Array[i].length() < 2) {
                // empty value could apply to commands such as M17 where no value is needed to enable motor
                continue;
            }
            for(int j = 1; j < cmd_Array[i].length(); j++) {
                if (!isDigit(cmd_Array[i][j]) && cmd_Array[i][j] != '.' && cmd_Array[i][j] != '-') {
                    _systemMessage = "Error: Invalid value in parameter " + cmd_Array[i];
                    return false;
                }
            }
        } else {
            _systemMessage = "Error: Invalid parameter " + cmd_Array[i];
            return false;
        }
    }
    _systemMessage = "Ok: Parameters are valid";
    return true;
}

bool Parser::validate_params_without_values(const StringType* cmd_Array, int size) {
    if (size < 1) return false;
    for (int i = 1; i < size; i++) {
        if(cmd_Array[i][0] != 'X' && cmd_Array[i][0] != 'Y' && cmd_Array[i][0] != 'Z' && cmd_Array[i][0] != 'F') {
            _systemMessage = "Error: Invalid parameter " + cmd_Array[i];
            return false;
        }
    }
    _systemMessage = "Ok: Parameters are valid";
    return true;
}

StringType Parser::getSystemMessage() {
    return _systemMessage;
}

Parser::g_code_command Parser::create_gcode_object(const StringType* cmd_Array, int size) {
    g_code_command gcode;
    
    gcode.main_command = cmd_Array[0];
    if (size < 1) return gcode;
    if(size > 1) {
        for (int i = 1; i < size; i++) {
            // only update params that have values i.e. string length > 1
            if (cmd_Array[i].length() > 1){
                // check first character of param and update value
                if (cmd_Array[i][0] == 'X') {
                    gcode.X = cmd_Array[i].substring(1).toDouble();
                } else if (cmd_Array[i][0] == 'Y') {
                    gcode.Y = cmd_Array[i].substring(1).toDouble();
                } else if (cmd_Array[i][0] == 'F') {
                    gcode.F = cmd_Array[i].substring(1).toDouble();
                } else if (cmd_Array[i][0] == 'I') {
                    gcode.I = cmd_Array[i].substring(1).toDouble();
                } else if (cmd_Array[i][0] == 'J') {
                    gcode.J = cmd_Array[i].substring(1).toDouble();
                } else if (cmd_Array[i][0] == 'R') {
                    gcode.R = cmd_Array[i].substring(1).toDouble();
                }
            }
        }
    }
    
    
    return gcode;
}