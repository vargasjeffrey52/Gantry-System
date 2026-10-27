#include <Arduino.h>
#include <Parser.h>
#include <globals.h>
#include <Processor.h>
#include "StepperMotor.h"
#include "MotionControl.h"



Processor* processor;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    Serial.setTimeout(100);  // Reduce timeout to speed up reading commands from serial monitor.
    processor = new Processor();
    // pinMode(ENAX, OUTPUT);
    // pinMode(STEPX, OUTPUT);
    // pinMode(DIRX, OUTPUT);
    // digitalWrite(ENAX, LOW);
    // for (int i = 0; i < 1600; i++)
    // {
    //     digitalWrite(STEPX, HIGH);
    //     delayMicroseconds(50);
    //     digitalWrite(STEPX, LOW);
    //     delayMicroseconds(50);
        
    // }
    // digitalWrite(ENAX, LOW);
    // Serial.println("DONE MOVING");
    delay(1000);
    processor->begin();
    Serial.println("ok: Setup complete");
    
}

void loop() {
    if (Serial.available()) {
        processor->notifySerialActivity();
        String command = Serial.readStringUntil('\n');
        // Only dequeue and dispatch when the parser actually queued a command.
        if (!processor->send_to_parser(command)) {
            return;
        }
        Parser::g_code_command gcode = dequeue_gcode();
        bool succeeded = false;
        if (processor->is_movement_command(gcode)) {
            succeeded = processor->send_to_motion_control(gcode);
        } else if (processor->is_sound_command(gcode)) {
            succeeded = processor->send_to_sound_manager(gcode);
        } else if (processor->is_rgb_command(gcode)) {
            succeeded = processor->send_to_rgb_status(gcode);
        } else if (processor->is_getter_command(gcode)) {
            succeeded = processor->send_to_getter(gcode);
        } else if (processor->is_modifier_command(gcode)) {
            succeeded = processor->send_to_modifier(gcode);
        } else {
            Serial.println("Error: command not recognized " + gcode.main_command);
        }
        if (succeeded && gcode.main_command != "M114") {
            Serial.println("ok: " + gcode.main_command + " executed");
        }
        processor->notifySerialActivity();

    }
    processor->update();
}
