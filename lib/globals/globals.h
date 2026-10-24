#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>   
#include "Parser.h" // Ensure Parser is defined before using it


#define ENAX 1
#define DIRX 40
#define STEPX 41
#define MS1X 2
#define MS2X 42
// #define ENAX 3
// #define DIRX 7
// #define STEPX 9
// #define MS1X 10
// #define MS2X 10

#define ENAY1 39
#define DIRY 21
#define STEPY 47
#define MS1Y 38
#define MS2Y 14
#define BUZZER_PIN 4
#define RGB_LED_PIN 48



#define NUMBER_OF_GCODES 3 // number of g-codes that will be allowed in buffer
#define NUM_VALID_COMMANDS_W_PARAMS 16 // movement, sound, and RGB commands
#define NUM_VALID_COMMANDS_WO_PARAMS 5 // number of commands that take no parameters
#define MIN_INT -32768 // used as NAN 
#define G0_DEFAULT_FEEDRATE_MM 99999.0 // mm/min, used by G0 only when F is omitted
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

// steps per 1 unit in the current unit mode (mm, inch, or step)
extern float stepsPerUnit();

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