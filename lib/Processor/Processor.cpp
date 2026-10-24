#include "Processor.h"
#include "Parser.h"
#include "globals.h"
#include "MotionControl.h"
#include <math.h>

namespace {
constexpr uint32_t STANDBY_TIMEOUT_MS = 10000;
constexpr double ARC_PI = 3.14159265358979323846;

bool isSoundMCode(const String& command) {
    return command == "M700" || command == "M710" ||
           command == "M711" || command == "M712";
}

bool isRgbMCode(const String& command) {
    return command == "M720" || command == "M721";
}

bool isMotorEnableCommand(const String& command) {
    return command == "M17" || command == "M18";
}

bool validateMotorAxisParameters(const String* parameters, int size, String& error) {
    uint8_t seenAxes = 0;
    for (int i = 1; i < size; ++i) {
        const String& parameter = parameters[i];
        if (parameter.length() != 1) {
            error = "Error: M17/M18 axis selectors must be bare X, Y, or Z letters";
            return false;
        }

        uint8_t axis = 0;
        if (parameter[0] == 'X') {
            axis = Parser::g_code_command::AXIS_X;
        } else if (parameter[0] == 'Y') {
            axis = Parser::g_code_command::AXIS_Y;
        } else if (parameter[0] == 'Z') {
            axis = Parser::g_code_command::AXIS_Z;
        } else {
            error = "Error: M17/M18 supports only X, Y, and Z selectors";
            return false;
        }

        if (seenAxes & axis) {
            error = "Error: duplicate axis selector in M17/M18 command";
            return false;
        }
        seenAxes |= axis;
    }
    return true;
}

bool isValidSArgument(const String& command, const String& parameter, double value) {
    if (parameter.length() < 2 || parameter[0] != 'S') {
        return false;
    }
    for (unsigned int i = 1; i < parameter.length(); ++i) {
        if (parameter[i] < '0' || parameter[i] > '9') {
            return false;
        }
    }

    if (command == "M700") {
        return value == 1 || value == 2 || value == 3 || value == 4 || value == 5 || value == 6 || value == 7;
    }
    if (command == "M712") {
        return value >= 0 && value <= 100;
    }
    if (command == "M720") {
        return value == 0 || value == 1;
    }
    if (command == "M721") {
        return value >= 0 && value <= 255;
    }
    return value == 0 || value == 1;
}

double directedArcSweep(double startX, double startY, double endX, double endY,
                        double centerX, double centerY, bool clockwise) {
    const double startAngle = atan2(startY - centerY, startX - centerX);
    const double endAngle = atan2(endY - centerY, endX - centerX);
    double sweep = clockwise ? startAngle - endAngle : endAngle - startAngle;
    while (sweep < 0) {
        sweep += 2 * ARC_PI;
    }
    return sweep;
}

bool validateArcParameters(const String* parameters, int size, String& error) {
    bool seenX = false;
    bool seenY = false;
    bool seenF = false;
    bool seenI = false;
    bool seenJ = false;
    bool seenR = false;

    for (int i = 1; i < size; ++i) {
        const String& parameter = parameters[i];
        if (parameter.length() < 2) {
            error = "Error: G2/G3 parameters require values";
            return false;
        }

        bool* seen = nullptr;
        switch (parameter[0]) {
            case 'X': seen = &seenX; break;
            case 'Y': seen = &seenY; break;
            case 'F': seen = &seenF; break;
            case 'I': seen = &seenI; break;
            case 'J': seen = &seenJ; break;
            case 'R': seen = &seenR; break;
            default:
                error = "Error: G2/G3 supports only X, Y, F, I, J, and R";
                return false;
        }
        if (*seen) {
            error = "Error: duplicate parameter in G2/G3 command";
            return false;
        }
        *seen = true;
    }
    return true;
}

bool resolveArcCenter(const Parser::g_code_command& gcode,
                      double startX, double startY, double endX, double endY,
                      bool clockwise, double& centerX, double& centerY,
                      String& error) {
    if (!isfinite(startX) || !isfinite(startY) ||
        !isfinite(endX) || !isfinite(endY)) {
        error = "Error: arc endpoints must be finite coordinates";
        return false;
    }

    const bool hasI = gcode.I != MIN_INT;
    const bool hasJ = gcode.J != MIN_INT;
    const bool hasR = gcode.R != MIN_INT;
    const bool sameEndpoint = hypot(endX - startX, endY - startY) < 1e-6;

    if (hasR && (hasI || hasJ)) {
        error = "Error: specify either R or I/J for an arc, not both";
        return false;
    }
    if (!hasR && !hasI && !hasJ) {
        error = "Error: G2/G3 requires R or at least one of I/J";
        return false;
    }

    if (hasR) {
        const double radius = fabs(gcode.R);
        const double chordX = endX - startX;
        const double chordY = endY - startY;
        const double chordLength = hypot(chordX, chordY);
        if (sameEndpoint || !isfinite(radius) || radius <= 0) {
            error = "Error: R-format arcs require distinct endpoints and a nonzero radius";
            return false;
        }

        const double tolerance = fmax(0.05, radius * 0.001);
        if (chordLength > 2 * radius + tolerance) {
            error = "Error: arc radius is too small for the requested endpoints";
            return false;
        }

        const double midpointX = (startX + endX) / 2;
        const double midpointY = (startY + endY) / 2;
        const double halfChord = chordLength / 2;
        const double centerOffset = sqrt(fmax(0, radius * radius - halfChord * halfChord));
        const double perpendicularX = -chordY / chordLength;
        const double perpendicularY = chordX / chordLength;
        const double center1X = midpointX + perpendicularX * centerOffset;
        const double center1Y = midpointY + perpendicularY * centerOffset;
        const double center2X = midpointX - perpendicularX * centerOffset;
        const double center2Y = midpointY - perpendicularY * centerOffset;
        if (!isfinite(center1X) || !isfinite(center1Y) ||
            !isfinite(center2X) || !isfinite(center2Y)) {
            error = "Error: computed arc center is not finite";
            return false;
        }
        const double sweep1 = directedArcSweep(startX, startY, endX, endY,
                                                center1X, center1Y, clockwise);
        const double sweep2 = directedArcSweep(startX, startY, endX, endY,
                                                center2X, center2Y, clockwise);
        const bool wantMajorArc = gcode.R < 0;
        const bool chooseSecond = wantMajorArc ? sweep2 > sweep1 : sweep2 < sweep1;
        centerX = chooseSecond ? center2X : center1X;
        centerY = chooseSecond ? center2Y : center1Y;
        return true;
    }

    centerX = startX + (hasI ? gcode.I : 0);
    centerY = startY + (hasJ ? gcode.J : 0);
    const double startRadius = hypot(startX - centerX, startY - centerY);
    const double endRadius = hypot(endX - centerX, endY - centerY);
    if (!isfinite(centerX) || !isfinite(centerY) ||
        !isfinite(startRadius) || !isfinite(endRadius)) {
        error = "Error: arc center and radius must be finite";
        return false;
    }
    const double tolerance = fmax(0.05, startRadius * 0.001);
    if (startRadius <= 0 || (!sameEndpoint && fabs(startRadius - endRadius) > tolerance)) {
        error = "Error: I/J arc center does not produce matching start and end radii";
        return false;
    }
    return true;
}
}

Processor::Processor()
    : motorX(STEPX, DIRX, ENAX, MS1X, MS2X),
      motorY1(STEPY, DIRY, ENAY1, MS1X, MS2X),
      motorZ(STEPZ, DIRZ, ENAZ, MS1Z, MS2Z),
      soundManager(BUZZER_PIN),
      rgbStatus(RGB_LED_PIN),
      M_Control(motorX, motorY1, soundManager, rgbStatus),
      _waitingForStartupMelody(false),
      _lastSerialActivityAt(0),
      _standbyActive(false),
      _standbyMelodyStarted(false),
      _absolutePositioning(false)
{
    Serial.println("PROCESSOR INIT");
}

void Processor::begin() {
    rgbStatus.begin();
    soundManager.begin();
    soundManager.playStartupMelody();
    _waitingForStartupMelody = soundManager.isPlaying();
    _lastSerialActivityAt = millis();
    _standbyActive = false;
    _standbyMelodyStarted = false;
}

void Processor::update() {
    soundManager.update();
    if (_waitingForStartupMelody && !soundManager.isPlaying()) {
        _waitingForStartupMelody = false;
        rgbStatus.setMode(RgbStatus::Mode::ReadyFlash);
    }

    if (!_standbyActive && !_waitingForStartupMelody &&
        millis() - _lastSerialActivityAt >= STANDBY_TIMEOUT_MS) {
        if (soundManager.isPlaying()) {
            soundManager.stop();
        }
        _standbyActive = true;
        _standbyMelodyStarted = false;
        rgbStatus.setMode(RgbStatus::Mode::Standby);
    }

    if (_standbyActive) {
        if (_standbyMelodyStarted && !soundManager.isPlaying()) {
            _standbyMelodyStarted = false;
        }
        if (!_standbyMelodyStarted && !soundManager.isPlaying() &&
            soundManager.isEnabled()) {
            soundManager.playCustomTheme();
            _standbyMelodyStarted = soundManager.isPlaying();
        }
    }

    rgbStatus.update();
}

void Processor::notifySerialActivity() {
    _lastSerialActivityAt = millis();
    if (!_standbyActive) {
        return;
    }

    if (_standbyMelodyStarted) {
        soundManager.stop();
    }
    _standbyActive = false;
    _standbyMelodyStarted = false;
    rgbStatus.setMode(RgbStatus::Mode::Ready);
}

void Processor::setSoundVolume(uint8_t volumePercent) {
    soundManager.setVolume(volumePercent);
}

void Processor::setRgbStatus(RgbStatus::Mode mode) {
    rgbStatus.setMode(mode);
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
                    if ((isSoundMCode(GCODE.main_command) || isRgbMCode(GCODE.main_command)) &&
                        (size != 2 ||
                         !isValidSArgument(GCODE.main_command, commandsArr[1], GCODE.S))) {
                        Serial.println("Error: sound/RGB commands require one valid S argument");
                        return false;
                    }
                    if (GCODE.main_command == "G2" || GCODE.main_command == "G3") {
                        String arcParameterError;
                        if (!validateArcParameters(commandsArr, size, arcParameterError)) {
                            Serial.println(arcParameterError);
                            return false;
                        }
                    }
                    if (isMotorEnableCommand(GCODE.main_command)) {
                        String motorParameterError;
                        if (!validateMotorAxisParameters(commandsArr, size, motorParameterError)) {
                            Serial.println(motorParameterError);
                            return false;
                        }
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
    if (gcode.main_command == "G90" || gcode.main_command == "G91" ||
        gcode.main_command == "G92" || isMotorEnableCommand(gcode.main_command) ||
        gcode.main_command == "M502" ||
        gcode.main_command == "M114" || gcode.main_command == "M119"){
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

bool Processor::is_rgb_command(Parser::g_code_command gcode){
    return isRgbMCode(gcode.main_command);
}

void Processor::send_to_sound_manager(Parser::g_code_command gcode){
    if (gcode.main_command == "M700") {
        switch (static_cast<int>(gcode.S)) {
            case 1: soundManager.playStartupMelody(); break;
            case 2: soundManager.playCompletionMelody(); break;
            case 3: soundManager.playSingleTone(); break;
            case 4: soundManager.playRandomTone(); break;
            case 5: soundManager.playCrashMelody(); break;
            case 6: soundManager.playCustomTheme(); break;
            case 7: soundManager.playDeathMelody(); break;
            default:
                Serial.println("Error: M700 S must be 1-7");
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
    } else if (gcode.main_command == "M712") {
        if (gcode.S >= 0 && gcode.S <= 100) {
            soundManager.setVolume(static_cast<uint8_t>(gcode.S));
        } else {
            Serial.println("Error: M712 S must be an integer from 0 to 100");
        }
    }
}

void Processor::send_to_rgb_status(Parser::g_code_command gcode){
    if (gcode.main_command == "M720" && (gcode.S == 0 || gcode.S == 1)) {
        rgbStatus.setEnabled(gcode.S == 1);
    } else if (gcode.main_command == "M721" && gcode.S >= 0 && gcode.S <= 255) {
        rgbStatus.setBrightness(static_cast<uint8_t>(gcode.S));
    }
}

void Processor::send_to_motion_control(Parser::g_code_command gcode){
    String cmd = gcode.main_command;
    const bool isArc = cmd == "G2" || cmd == "G3";
    const bool clockwise = cmd == "G2";
    const float x0 = Xcurrent;
    const float y0 = Ycurrent;
    const bool hasFeedrate = gcode.F != MIN_INT;
    float modalFeedrate = FEEDRATE;
    if (hasFeedrate) {
        if (!isfinite(gcode.F) || gcode.F <= 0) {
            Serial.println("Error: feedrate must be greater than zero");
            return;
        }
        modalFeedrate = gcode.F * stepsPerUnit() / STEPS_PER_MM;
    }

    const float feed = (cmd == "G0" && !hasFeedrate)
        ? G0_DEFAULT_FEEDRATE_MM
        : modalFeedrate;
    if (!isfinite(feed) || feed <= 0 || !isfinite(ACCELERATION) || ACCELERATION <= 0) {
        Serial.println("Error: feedrate and acceleration must be greater than zero");
        return;
    }

    const float xValue = (gcode.X != MIN_INT) ? gcode.X : 0;
    const float yValue = (gcode.Y != MIN_INT) ? gcode.Y : 0;
    const float x1 = (gcode.X == MIN_INT)
        ? x0
        : (_absolutePositioning ? xValue : x0 + xValue);
    const float y1 = (gcode.Y == MIN_INT)
        ? y0
        : (_absolutePositioning ? yValue : y0 + yValue);
    double centerX = 0;
    double centerY = 0;
    String arcError;
    if (isArc && !resolveArcCenter(gcode, x0, y0, x1, y1, clockwise,
                                   centerX, centerY, arcError)) {
        Serial.println(arcError);
        return;
    }

    FEEDRATE = modalFeedrate;
    rgbStatus.setMode(RgbStatus::Mode::Moving);
    bool movementSucceeded = true;
    if (isArc) {
        movementSucceeded = M_Control.moveArc(x0, y0, x1, y1,
                                               centerX, centerY, feed, clockwise);
        if (!movementSucceeded) {
            Serial.println("Error: arc is too large or has invalid motion settings");
        }
    } else {
        M_Control.moveLine(x0, y0, x1, y1, feed);
    }

    if (!movementSucceeded) {
        rgbStatus.setMode(RgbStatus::Mode::Ready);
        return;
    }

    Xcurrent = x1;
    Ycurrent = y1;
    rgbStatus.setMode(RgbStatus::Mode::Ready);
    soundManager.playCompletionMelody();
}

void Processor::send_to_modifier(Parser::g_code_command gcode){
    if (gcode.main_command == "G90") {
        _absolutePositioning = true;
        Serial.println("Positioning mode: absolute");
        return;
    }
    if (gcode.main_command == "G91") {
        _absolutePositioning = false;
        Serial.println("Positioning mode: relative");
        return;
    }
    if (gcode.main_command == "G92") {
        if (gcode.X == MIN_INT && gcode.Y == MIN_INT) {
            Serial.println("Error: G92 requires X and/or Y");
            return;
        }
        if (gcode.X != MIN_INT) {
            Xcurrent = gcode.X;
        }
        if (gcode.Y != MIN_INT) {
            Ycurrent = gcode.Y;
        }
        Serial.println("Logical position updated");
        return;
    }
    if (isMotorEnableCommand(gcode.main_command)) {
        const bool enable = gcode.main_command == "M17";
        uint8_t axes = gcode.axisMask;
        if (axes == 0) {
            axes = Parser::g_code_command::AXIS_X |
                   Parser::g_code_command::AXIS_Y |
                   Parser::g_code_command::AXIS_Z;
        }

        if (axes & Parser::g_code_command::AXIS_X) {
            if (enable) motorX.enable();
            else motorX.disable();
        }
        if (axes & Parser::g_code_command::AXIS_Y) {
            if (enable) motorY1.enable();
            else motorY1.disable();
        }
        if (axes & Parser::g_code_command::AXIS_Z) {
            if (enable) motorZ.enable();
            else motorZ.disable();
        }
        Serial.println(enable ? "Selected stepper motors enabled" : "Selected stepper motors disabled");
        return;
    }

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