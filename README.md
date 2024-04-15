# EE344
Auto-dialling Alarm System for Temperature and Humidity Monitoring

## File organization
The following folders will be present in the root directory of the project:
- `energy_meter`: Contains KiCad design files for the sensing unit PCB
- `Software`: Contains all firmware used in the project
    - `RP2040_code`: Source code for the firmware of the sensing unit written using the RP2040 C/C++ SDK. The code may be built using CMake or the uf2 file in the build subdirectory may be flashed directly to the RPi Pico W.
    - `arduino_code`: Source code for the sensing unit based on arduino. Based on code provided by Analog Devices for the EVAL-ACE9000 SHIELDZ board.
    - `central_code`: Source code for the central unit written in MicroPython
    - Other python files are for setting up a TCP client to test data logging
- `CAD`: Contains files for the enclosures of the sensing unit and central unit  
- `Test`: Contains test data from a long-term test using an Air Conditioner
- `central_unit`: Contains KiCad design files for a central unit with a SIM800L module. This is not used in the final prototype but may be useful for future iterations
- `Website`: Contains Code for a locally hosted website to view real-time data from the sensing units 
- Documentation file
- README file (this file)

