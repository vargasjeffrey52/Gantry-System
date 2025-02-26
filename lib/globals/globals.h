#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>  // Ensure String is defined
#include "Parser.h"   // Include Parser to use g_code_command

#define NUMBER_OF_GCODES 3

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

// Define StringType properly
using StringType = String;

// Declare global valid command lists (define them in Parser.cpp)
extern const StringType validCommandsWParams[];
extern const StringType validCommandsWoutParams[];

#endif // GLOBALS_H
