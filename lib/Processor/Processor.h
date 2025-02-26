#include <Arduino.h>
#include "Parser.h"
#include "globals.h"

class Processor {
public:
    Processor() = default;
    bool is_movement_command(String command);
    bool is_modifier_command(String command);

    void send_to_motion_control(String command);
    void send_to_modifier(String command);
    void send_to_parser(String command);


private:
    String commandsArr[10]; // size of array for parsing G-code string command


};