#include <Arduino.h>
#include "Parser.h"
#include "globals.h"
#include "MotionControl.h"
#include "SoundManager.h"
#include "RgbStatus.h"

class Processor {
public:
    //Processor() = default;
    Processor();
    void begin();
    void update();
    void notifySerialActivity();
    void setSoundVolume(uint8_t volumePercent);
    void setRgbStatus(RgbStatus::Mode mode);
    bool is_movement_command(Parser::g_code_command gcode);
    bool is_modifier_command(Parser::g_code_command gcode);
    bool is_getter_command(Parser::g_code_command gcode);
    bool is_sound_command(Parser::g_code_command gcode);
    bool is_rgb_command(Parser::g_code_command gcode);

    bool send_to_parser(String command);
    void send_to_motion_control(Parser::g_code_command gcode);
    void send_to_modifier(Parser::g_code_command gcode);
    void send_to_getter(Parser::g_code_command gcode);
    void send_to_sound_manager(Parser::g_code_command gcode);
    void send_to_rgb_status(Parser::g_code_command gcode);


private:
    String commandsArr[10]; // size of array for parsing G-code string command
    StepperMotor motorX;
    StepperMotor motorY1;
    SoundManager soundManager;
    RgbStatus rgbStatus;
    MotionControl M_Control;
    bool _waitingForStartupMelody;
    uint32_t _lastSerialActivityAt;
    bool _standbyActive;
    bool _standbyMelodyStarted;
};