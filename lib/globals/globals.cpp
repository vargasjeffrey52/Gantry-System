#include "globals.h"

// Define global variables
Parser PARSER;
Parser::g_code_command Gcode_BUFFER[NUMBER_OF_GCODES];
Parser::g_code_command GCODE;

const StringType validCommandsWParams[NUM_VALID_COMMANDS_W_PARAMS] = {"G0", "G1", "G2", "G3", "G92", "G28", "M17", "M18", "M203", "M201"};
const StringType validCommandsWoutParams[NUM_VALID_COMMANDS_WO_PARAMS] = {"G90", "G91", "M502", "M114", "M119"};


int HEAD = 0, TAIL = 0, Gcode_BUFFER_SIZE = 0;
float Xcurrent = 0, Ycurrent = 0, jcurrent, icurrent, rcurrent;

// Add g-code to the queue for later processing
void enqueue_gcode(Parser::g_code_command gcode) {
    Gcode_BUFFER[HEAD] = gcode;
    HEAD = (HEAD + 1) % NUMBER_OF_GCODES;
    if (Gcode_BUFFER_SIZE < NUMBER_OF_GCODES) {
        Gcode_BUFFER_SIZE++;
    }
}

// Get g-code from the queue for processing
Parser::g_code_command dequeue_gcode() {
   
    if (Gcode_BUFFER_SIZE == 0) {
        return Parser::g_code_command(); // return empty gcode object
    }
    Parser::g_code_command gcode = Gcode_BUFFER[TAIL];
    TAIL = (TAIL + 1) % NUMBER_OF_GCODES;
    Gcode_BUFFER_SIZE--;
    return gcode;
}

void print_Gcode_object(Parser::g_code_command gcode) {
    Serial.print("Main Command: ");
    Serial.println(gcode.main_command);
    Serial.print("X: ");
    Serial.print((int) gcode.X);
    Serial.print(" Y: ");
    Serial.print(gcode.Y);
    Serial.print(" F: ");
    Serial.println(gcode.F);
    Serial.print("I: ");
    Serial.print(gcode.I);
    Serial.print(" J: ");
    Serial.print(gcode.J);
    Serial.print(" R: ");
    Serial.println(gcode.R);
}
