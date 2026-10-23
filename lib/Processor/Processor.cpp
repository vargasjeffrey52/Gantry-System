#include "Processor.h"
#include "Parser.h"
#include "globals.h"
#include "MotionControl.h"

namespace {
bool isSoundMCode(const String& command) {
    return command == "M700" || command == "M710" || command == "M711";
}

bool isValidSoundArgument(const String& command, const String& parameter, double value) {
    if (parameter.length() < 2 || parameter[0] != 'S') {
        return false;
    }
    for (unsigned int i = 1; i < parameter.length(); ++i) {
        if (parameter[i] < '0' || parameter[i] > '9') {
            return false;
        }
    }

    if (command == "M700") {
        return value == 1 || value == 2 || value == 3 || value == 4 || value == 5;
    }
    return value == 0 || value == 1;
}
}

Processor::Processor()
    : motorX(STEPX, DIRX, ENAX, MS1X, MS2X),
      motorY1(STEPY, DIRY, ENAY1, MS1X, MS2X),
      soundManager(BUZZER_PIN),
      M_Control(motorX, motorY1, soundManager)
{
    Serial.println("PROCESSOR INIT");
}

void Processor::begin() {
    soundManager.begin();
    soundManager.playStartupMelody();
}

void Processor::update() {
    soundManager.update();
}

bool Processor::send_to_parser(String command){
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
                    if (isSoundMCode(GCODE.main_command) &&
                        (size != 2 ||
                         !isValidSoundArgument(GCODE.main_command, commandsArr[1], GCODE.S))) {
                        Serial.println("Error: sound commands require one valid S argument");
                        return false;
                    }
                    //print_Gcode_object(GCODE); 
                    enqueue_gcode(GCODE);
                    return true;
                    
                    
                }
                // bad values on params
                else {
                    Serial.println(PARSER.getSystemMessage()); // debugging
                    return false;
                }
            } 
            // command does not require parameters
            else {
                if (size > 1){
                    Serial.println("Error: " + commandsArr[0] + " does not take parameters");
                    //Serial.println(PARSER.getSystemMessage()); // debugging
                    return false;
                }
                GCODE = PARSER.create_gcode_object(commandsArr, size);
                //print_Gcode_object(GCODE);
                enqueue_gcode(GCODE);
                return true;
            }
        }
        // bad main command
        else {
            Serial.println(PARSER.getSystemMessage()); // debugging
            // Serial.println();
            // Serial.println("The following G-code commands was dequeued: ");
            // print_Gcode_object(dequeue_gcode());
            return false;
            
        }

}


bool Processor::is_movement_command(Parser::g_code_command gcode){
    if (gcode.main_command == "G0" || gcode.main_command == "G1" || gcode.main_command == "G2" || gcode.main_command == "G3"){
        return true;
    }
    return false;
}

bool Processor::is_modifier_command(Parser::g_code_command gcode){
    if (gcode.main_command == "G90" || gcode.main_command == "G91" || gcode.main_command == "M502" || gcode.main_command == "M114" || gcode.main_command == "M119"){
        return true;
    }
    return false;
}

bool Processor::is_getter_command(Parser::g_code_command gcode){
    if (gcode.main_command == "M114" || gcode.main_command == "M119"){
        return true;
    }
    return false;
}

bool Processor::is_sound_command(Parser::g_code_command gcode){
    return isSoundMCode(gcode.main_command);
}

void Processor::send_to_sound_manager(Parser::g_code_command gcode){
    if (gcode.main_command == "M700") {
        switch (static_cast<int>(gcode.S)) {
            case 1: soundManager.playStartupMelody(); break;
            case 2: soundManager.playCompletionMelody(); break;
            case 3: soundManager.playSingleTone(); break;
            case 4: soundManager.playRandomTone(); break;
            case 5: soundManager.playCrashMelody(); break;
            default:
                Serial.println("Error: M700 S must be 1-5");
                break;
        }
    } else if (gcode.main_command == "M710") {
        if (gcode.S == 0 || gcode.S == 1) {
            soundManager.setEnabled(gcode.S == 1);
        } else {
            Serial.println("Error: M710 S must be 0 or 1");
        }
    } else if (gcode.main_command == "M711") {
        if (gcode.S == 0 || gcode.S == 1) {
            soundManager.setStepSoundEnabled(gcode.S == 1);
        } else {
            Serial.println("Error: M711 S must be 0 or 1");
        }
    }
}

void Processor::send_to_motion_control(Parser::g_code_command gcode){
    // do something
    float x0 = 0, y0 = 0;
    String cmd = gcode.main_command;

    if (cmd == "G2" || cmd == "G3") {
        Serial.println("Error: arcs (G2/G3) are not implemented yet");
        return;
    }

    if (gcode.F != MIN_INT){
        FEEDRATE = gcode.F * stepsPerUnit() / STEPS_PER_MM;
    }

    float feed = (cmd == "G0") ? MAX_FEEDRATE_MM : FEEDRATE;
    float x1 = (gcode.X != MIN_INT) ? gcode.X : x0;
    float y1 = (gcode.Y != MIN_INT) ? gcode.Y : y0;

    M_Control.moveLine(x0, y0, x1, y1, feed);
    soundManager.playCompletionMelody();
}

void Processor::send_to_modifier(Parser::g_code_command gcode){
    // do something
    Serial.println();
    Serial.println("Sending to modifier ##############");
    Serial.println();
}
void Processor::send_to_getter(Parser::g_code_command gcode){
    // do something
    Serial.println();
    Serial.println("Sending to getter ##############");
    Serial.println();
}