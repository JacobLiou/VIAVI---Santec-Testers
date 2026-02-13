import time
import socket


def create_object(IP, Port, timeOut=3):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((IP, Port))
    sock.settimeout(timeOut)
    return sock


def send_command(command, socket_object, endChar="\n", buffer_size=1024):
    commandNewline = f"{command}{endChar}"
    socket_object.send(bytes(commandNewline, "utf-8"))
    if "?" in commandNewline:
        data = socket_object.recv(buffer_size).decode('ASCII')
        while "\n" not in data:
            data += socket_object.recv(buffer_size).decode('ASCII')
        return data.replace("\n", "")


def error_check(socket_object):
    code, error = send_command("SYST:ERR?", socket_object).split(',')
    if code != '0':
        print(f"Error: {error}")


##  Initial Setup
IP = '10.14.132.194'  # From the MAP300 itself
pct_port = 8301  # The PCT Port is 8300 + slot number of mORL module
cmr_port = 8100  # The Chassis Port is 8100
cmr = create_object(IP, cmr_port)

## Start the Super Application (required after powering on the chassis)
send_command("SUPER:LAUNCH PCT", cmr)  # Command goes to the Chassis to start the super application
time.sleep(10)  # The unit takes about 7 seconds to launch the super application

## Connect to the PCT
pct = create_object(IP, pct_port)  # Only after the super application starts can we connect to the PCT

## Setup the ORL Window
# Supporting documentation includes the MEAS:ORL:SETUP command, with descriptions of the parameters
method = 2  # 1 = Integration mode, 2 = Discrete Mode. For APC connectors, Discrete Mode is generally more appropriate
origin = 1  # 1 = A+B anchored to DUT start, 2 = A+B anchored to DUT end, 3 = anchor A from DUT start, B from DUT end
Aoffset = -0.5  # Value is the distance in meters from the anchor to place the A marker
Boffset = 0.5  # Value is the distance in meters from the anchor to place the B marker
send_command(f"MEAS:ORL:SETUP 1,{method},{origin},{Aoffset},{Boffset}", pct)
error_check(pct)

## Setting up the system for the measurement
send_command(f"SOURCE:LIST 1310, 1550", pct)  # Set the wavelengths to be used
error_check(pct)
wavelengths = send_command("SOURCE:LIST?", pct).split(',')  # Check the wavelengths being used
send_command("PATH:LAUNCH 1", pct)  # Set the PCT to launch from J1
error_check(pct)
send_command("PATH:LIST 1,1-12", pct)  # Set the PCT to measure 12 channels
error_check(pct)

## Take a zero-loss measurement
override_reference = True
if override_reference:
    il = 0.1  # Set in dB
    length = 3  # Set in meters
    for i in range(1, 13):
        send_command(f"PATH:JUMPER:IL 1,{i},{il}", pct)  # Set the IL Override Value
        error_check(pct)
        send_command(f"PATH:JUMPER:IL:AUTO 1,{i},0", pct)  # Activate the override IL to be used
        error_check(pct)
        send_command(f"PATH:JUMPER:LENGTH 1,{i},{length}", pct)  # Set the Length Override Value
        error_check(pct)
        send_command(f"PATH:JUMPER:LENGTH:AUTO 1,{i},0", pct)  # Activate the override Length to be used
        error_check(pct)
else:
    for i in range(1,13):
        send_command(f"PATH:JUMPER:IL:AUTO 1,{i},1", pct)  # Deactivate the override IL to be used
        error_check(pct)
        send_command(f"PATH:JUMPER:LENGTH:AUTO 1,{i},1", pct)  # Deactivate the override Length to be used
        error_check(pct)
    send_command("SENS:FUNC 0", pct)  # Set the PCT to "reference mode"
    error_check(pct)
    send_command("MEAS:START", pct)  # Start the reference
    while send_command("MEAS:STATE?", pct) == "2":  # Wait until reference is done, 2 = still running
        time.sleep(0.1)
    print("Done Reference" if send_command("MEAS:STATE?", pct) == "1" else "Error Occurred")  # 1 = Complete, 3 = Error
    error_check(pct)
send_command("SENS:FUNC 1", pct)  # Set the PCT to "measurement mode"

## Take a measurement of IL and RL
error_check(pct)
send_command("MEAS:START", pct)  # Start the measurement
while send_command("MEAS:STATE?", pct) == "2":  # Wait until measurement is done, 2 = still running
    time.sleep(0.1)
print("Done Measurement" if send_command("MEAS:STATE?", pct) == "1" else "Error Occurred")  # 1 = Complete, 3 = Error
error_check(pct)

## Pull the results of the Measurement
for i in range(1,13):
    result = send_command(f"MEAS:ALL? {i},1", pct).split(',')  # Pulls the result line for each channel measured
    for j in range(len(wavelengths)):
        res = result[0+5*j:5+5*j]
        rl = float(res[3])  # The RL result is the 4th item in the result list
        print(f"Channel {i} @ {wavelengths[j]} nm = {rl} dB RL")
        error_check(pct)














