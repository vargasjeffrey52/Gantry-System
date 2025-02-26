#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

#ifdef ARDUINO
    #include <Arduino.h>
    using StringType = String;  // Use Arduino String
#else
    #include <string>
    #include <cmath>  // For NAN
    #ifdef _WIN32
        #define EXPORT __declspec(dllexport)
    #else
        #define EXPORT
    #endif
    using StringType = std::string;  // Use std::string on PC
#endif

class CommandParser {
public:
    // Constructor
    CommandParser();

    // Core Parsing Methods
    int CmdToArr(const StringType& commands, char delimiter, StringType* commandsArr);
    int parseValues(const StringType& command);
    StringType parseSubCommand(const StringType& command);

    // Validation and Error Handling
    int validateMainCMD(const StringType& command);
    StringType getAckMessage();

    // Parsed Command Structure
    struct ParsedCommand {
        StringType commandType;
        float xValue;
        float yValue;
        float speed;
        bool isValid;
        
        ParsedCommand() : commandType(""), xValue(NAN), yValue(NAN), speed(NAN), isValid(false) {}
    };

    // Parsing Full Command Components
    ParsedCommand parseCommandComponents(StringType* commandsArr, int numCommands);
    
private:
    StringType _errorMessage;
    bool isNumber(const StringType& str);
    static const StringType validCommandsWParams[];  // Declare as a private attribute
    static const StringType validCommandsWoutParams[];  // Declare as a private attribute
};

// Expose function for Python testing (PC only)
#ifndef ARDUINO
extern "C" {
    EXPORT const char* parse_command(const char* command);
}
#endif

#endif // COMMANDPARSER_H
