#ifndef PARSER_H
#define PARSER_H

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

class Parser {
public:
    // Constructor
    Parser() = default;

    // Command to arr (split by delimiter, default = ' ') returns array size
    int command_to_arr(const StringType& command, char delimiter, StringType* commandsArr);

    // Validate Main Command  (cmd = ["G1" , "X10", "Y20", "F30"]) must be an array
    bool validate_main_command(StringType* cmd_Array, int size);

    // Checks if the command requires parameters cmd = ["G1" , "X10", "Y20", "F30"]) X, Y, and F are parameters
    bool requiresParams(const StringType* cmd_Array);

    // Validates the parameters of the command cmd = ["G1" , "X10", "Y20", "F30"]) that contain values
    bool validate_params_with_values(const StringType* cmd_Array, int size);

    // Validates parameters of the command cmd = ["G1" , "X", "Y", "F"]) that do not contain values
    bool validate_params_without_values(const StringType* cmd_Array, int size);

    StringType getSystemMessage();

    struct g_code_command {
        String main_command;  // Main command Ex. G0, G1, G2, G3, ...
        double X, Y, F, I, J, R;
        static constexpr int MIN_INT = -32768;  // Use static constexpr for better portability

        // Constructor that ensures proper initialization of default values
        g_code_command() {
            // Initialize all values to default
            main_command = "";
            X = MIN_INT;
            Y = MIN_INT;
            F = MIN_INT;
            I = MIN_INT;
            J = MIN_INT;
            R = MIN_INT;
        }
    };

    g_code_command create_gcode_object(const StringType* cmd_Array, int size);

private:
    StringType _systemMessage;
    
    // Declare static valid command arrays for commands with and without parameters
    // static const StringType validCommandsWParams[];
    // static const StringType validCommandsWoutParams[];
};

// Declare global instance of Parser
extern Parser PARSER;

// Expose function for Python testing (PC only)
#ifndef ARDUINO
extern "C" {+
    EXPORT const char* parse_command(const char* command);
}
#endif

#endif // PARSER_H
