#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>   
#include "Parser.h" // Ensure Parser is defined before using it


#define ENAX 3
#define DIRX 7
#define STEPX 9
#define MS1X 10
#define MS2X 10

#define ENAY1 4
#define ENAY2 5
#define DIRY 6
#define STEPY 8
#define MS1Y 10
#define MS2Y 10



#define NUMBER_OF_GCODES 3 // number of g-codes that will be allowed in buffer
#define NUM_VALID_COMMANDS_W_PARAMS 10 // number of g-code commands that will require parameters
// int must match number of params in  {"G0", "G1", "G2", "G3", "G92", "G28", "M17", "M18", "M203", "M201"} in globals.cpp
#define NUM_VALID_COMMANDS_WO_PARAMS 5 // similar to top
#define MIN_INT -32768 // used as NAN 
#define MAX_FEEDRATE_MM 5000.0 // mm/min
#define MAX_FEEDRATE_IN 196.85 // in/min


extern int CURRENT_UNIT;
extern float STEPS_PER_MM;
extern float STEPS_PER_INCH;




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
extern float Xcurrent, Ycurrent, FEEDRATE, icurrent, jcurrent, rcurrent, ACCELERATION;

// If StringType is not defined, define it
#ifndef StringType
#define StringType String
#endif

// Declare global valid command lists (define them in Parser.cpp)

extern const StringType validCommandsWParams[NUM_VALID_COMMANDS_W_PARAMS];
extern const StringType validCommandsWoutParams[NUM_VALID_COMMANDS_WO_PARAMS];
#endif // GLOBALS_H