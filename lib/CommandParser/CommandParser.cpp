#include "CommandParser.h"


// Constructor
CommandParser::CommandParser() {}

// Parses a command string into an array of subcommands
/*This command takes a string and splits it into substring that are stored in an array.
    its intended to take a G-code command and split it into its components Ex)
    "G0 X10 Y20 F1500" will get converterted to --> ["G0", "X10", "Y20", "F1500"]
Parameter:
    commands - String containing the recieved possible G-code that has not been validated yet Ex)"G0 X10 Y20 F1500
    delimiter - String containing character in which to split the G-code string recieved Ex) " "
    commandsArr - Empty List to store G-code componenets Ex)["G0", "X10", "Y20", "F1500"]
Return: 
    number of components in G-Code
*/
int CommandParser::CmdToArr(const StringType& commands, char delimiter, StringType* commandsArr) {
    int numCommands = 0;
    int startIndex = 0;
    // Serial.print("[");// DEBUGGING
    for (int index = 0; index <= commands.length(); index++) {
        if (commands[index] == delimiter || index == commands.length()) {
            commandsArr[numCommands++] = 
            #ifdef ARDUINO
                // Arduino substring command start index is inclusive and end index is exclusive
                commands.substring(startIndex, index);
            #else
                // PC substr fucntion is inclusive on start index and lenght of substring
                commands.substr(startIndex, index - startIndex);
            #endif

            // Serial.print(commands.substring(startIndex, index)); // check GET RID OF THIS EVENTUALLY
            // if(index < commands.length()){
            //     Serial.print(", ");// check GET RID OF THIS EVENTUALLY
            // } // DEBUGGING
            startIndex = index + 1;
        }
    }
    // Serial.println("]");// check GET RID OF THIS EVENTUALLY DEBUGGING
    return numCommands;
}


// Extracts numeric values from a command (e.g., "X10" -> 10)
int CommandParser::parseValues(const StringType& command) {
    #ifdef ARDUINO
        return command.substring(1).toInt(); // Arduino's String toInt()
    #else
        return std::stoi(command.substr(1)); // PC version using std::stoi()
    #endif
}

// Extracts the main subcommand identifier (e.g., "G" from "G1")
StringType CommandParser::parseSubCommand(const StringType& command) {
    #ifdef ARDUINO
        return command.substring(0, 1);
    #else 
        return command.substr(0, 1);
    #endif
}

/**
 * @brief Checks if a given string represents a valid numeric value.
 * 
 * This function determines whether a string contains only numeric characters ('0'-'9')
 * and at most one decimal point ('.'). It does not check for negative signs ('-') 
 * or scientific notation (e.g., "1.23e4").
 * 
 * @param str The input string to check.
 * @return true if the string represents a valid number, false otherwise.
 */
// Helper function to check if a string contains only numbers (for validation)
bool CommandParser::isNumber(const StringType& str) {
    for (char c : str) {
        if (!isdigit(c) && c != '.') return false;
    }
    return true;
}

// Validates the basic structure of a command
int CommandParser::validateMainCMD(const StringType& command) {
    int valid = -1; // -1 is not valid 0 valid with params 1 valid without params
    if (command.length() < 2) {
        _errorMessage = "Invalid command: too short.";
        return valid; // -1 is not a valid command.
    }
    StringType mainCMD;
    bool isValidCommand = false;

    #ifdef ARDUINO
        int end = command.indexOf(' ');
        if (end==-1){
            mainCMD =command.substring(0);
        }else {
            mainCMD = command.substring(0,end);
        }
    #else
        mainCMD = command.substr(0, command.find(' '));
    #endif
    // checks if the commands sent is in the list of valid commands
    for(const StringType validCMD: validCommandsWParams ){
        if (mainCMD == validCMD){
            isValidCommand = true;
            valid = 0; // valid with params
            break;
        }
    }

    for(const StringType validCMD: validCommandsWoutParams ){
        if (mainCMD == validCMD){
            isValidCommand = true;
            valid = 1;//
            break;
        }
    }

    if (!isValidCommand) {
        _errorMessage = "Invalid command type: " + mainCMD;
        return -1; // -1 is not a valid command.
    }

   
    return valid;
    
 
}










// Returns the last error message
StringType CommandParser::getAckMessage() {
    return _errorMessage;   
}

// Parses a full command into structured components
/* Stores the command into a G-code command Object (parsedCommand)
A G-code object contains (commandType,coordinates, speed, validCheck... )
Ex) ParsedCommand (G0, X10, Y20, F1500)

Parameters
    commandArr - Array of strings containing gcode.
    numCommands - number of elements in array

TODO: need to validate subcomands better. 
*/
CommandParser::ParsedCommand CommandParser::parseCommandComponents(StringType* commandsArr, int numCommands) {
    ParsedCommand parsed;
    // 
    if (numCommands == 0) {
        _errorMessage = "Error: Empty command received.";
        parsed.isValid = false;
        return parsed;
    }
    StringType command = commandsArr[0]; 

    // List of valid G-code commands (add more as needed)
    const StringType validCommands[] = {"G0", "G1", "G90", "G21", "M30"};
    const int numValidCommands = sizeof(validCommands) / sizeof(validCommands[0]);

    bool isValidCommand = false;
    for (int i = 0; i < numValidCommands; i++) {
        if (command == validCommands[i]) {
            isValidCommand = true;
            break;
        }
    }


    parsed.commandType = 
        #ifdef ARDUINO
            command.substring(0, 1);
        #else
            command.substr(0, 1);
        #endif // Extract command type (e.g., G)

    for (int i = 1; i < numCommands; i++) {
        char subType = commandsArr[i][0];
        StringType valueStr = 
        #ifdef ARDUINO
            commandsArr[i].substring(1);
        #else
            commandsArr[i].substr(1);
        #endif

        if (!isNumber(valueStr)) {
            _errorMessage = "Error: Invalid parameter format: " + commandsArr[i];
            parsed.isValid = false;
            return parsed;
        }

        int value = 
            #ifdef ARDUINO
                valueStr.toInt();
            #else
                std::stoi(valueStr);
            #endif

        if (subType == 'X') parsed.xValue = value;
        else if (subType == 'Y') parsed.yValue = value;
        else if (subType == 'F') parsed.speed = value;
    }

    parsed.isValid = true;
    return parsed;
}

// Expose function for Python testing (PC only)
#ifndef ARDUINO
extern "C" {
    EXPORT const char* parse_command(const char* command) {
        static std::string response;
        CommandParser parser;

        StringType cmd = command;
        bool valid = parser.validateMainCMD(cmd);
        response = valid ? "OK: Command is valid" : parser.getAckMessage();

        return response.c_str();
    }
}
#endif
