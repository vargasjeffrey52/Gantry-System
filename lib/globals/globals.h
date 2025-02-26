#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>   
#include "Parser.h" // Ensure Parser is defined before using it

#define NUMBER_OF_GCODES 3
#define NUM_VALID_COMMANDS_W_PARAMS 10
#define NUM_VALID_COMMANDS_WO_PARAMS 5

extern Parser PARSER;
extern Parser::g_code_command Gcode_BUFFER[NUMBER_OF_GCODES];
extern Parser::g_code_command GCODE;

extern int HEAD, TAIL, Gcode_BUFFER_SIZE;

// Add g-code to the queue for later processing
extern void enqueue_gcode(Parser::g_code_command gcode);

// Get g-code from the queue for processing
extern Parser::g_code_command dequeue_gcode();

extern void print_Gcode_object(Parser::g_code_command gcode);

// Global movement variables
extern float Xcurrent, Ycurrent, feedrate, icurrent, jcurrent, rcurrent;

// If StringType is not defined, define it
#ifndef StringType
#define StringType String
#endif

// Declare global valid command lists (define them in Parser.cpp)

extern const StringType validCommandsWParams[NUM_VALID_COMMANDS_W_PARAMS];
extern const StringType validCommandsWoutParams[NUM_VALID_COMMANDS_WO_PARAMS];
#endif // GLOBALS_H