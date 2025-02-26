#include <Arduino.h>
#include <Parser.h>
#include <globals.h>
#include <Processor.h>



Processor processor;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.setTimeout(100);  // Reduce timeout to speed up reading commands from serial monitor.
}

void loop() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        processor.send_to_parser(command);
        Parser::g_code_command gcode = dequeue_gcode();
        if (processor.is_movement_command(gcode)) {
            processor.send_to_motion_control(gcode);
        } else if (processor.is_modifier_command(gcode)) {
            processor.send_to_modifier(gcode);
        } else if (processor.is_getter_command(gcode)) {
            processor.send_to_getter(gcode);
        } else {
            Serial.println("Error: command not recognized" + gcode.main_command);
        }   

    }
}



