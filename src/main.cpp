#include <Arduino.h>
#include <Parser.h>
#include <globals.h>
#include <Processor.h>



String commandsArr[10]; // size of array for parsing G-code string command
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
    }
}



