import ctypes
import os

# Absolute path to the DLL
DLL_PATH = r"C:\Users\varga\Documents\PlatformIO\Projects\GantryCode\TestDevelopment\commandparser.dll"

# Check if the DLL exists
if not os.path.exists(DLL_PATH):
    raise FileNotFoundError(f"❌ DLL not found at {DLL_PATH}")
print("✅ DLL found!")

# Load the DLL
parser = ctypes.CDLL(DLL_PATH)

# Define function input & output types
parser.parse_command.argtypes = [ctypes.c_char_p]  # Input: string (char*)
parser.parse_command.restype = ctypes.c_char_p  # Output: string (char*)

def run_test(command, expected_output):
    """Run a test case to validate G-code parsing."""
    response = parser.parse_command(command.encode()).decode()

    if response == expected_output:
        print(f"✅ Test Passed: {command} → {response}")
    else:
        print(f"❌ Test Failed: {command} → {response} (Expected: {expected_output})")

# ✅ List of valid commands (must match the C++ validCommands[] list)
valid_commands = ["G0", "G1", "G90", "G21", "M30"]

# ✅ Test valid G-code commands
print("\n🔹 Running Valid Command Tests...\n")
for cmd in valid_commands:
    run_test(f"{cmd} X10 Y20 F1500", "OK: Command is valid")

# ✅ Test invalid commands
print("\n🔹 Running Invalid Command Tests...\n")
run_test("G99 X10 Y10", "Invalid command type: G99")
run_test("M50", "Invalid command type: M50")
run_test("", "Invalid command: too short.")

print("\n🎉 All tests completed!")
