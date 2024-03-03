# EE344
Auto-dialling Alarm System for Temperature and Humidity Monitoring

## Changes from original shield
- 1 Phase residential 3 Wire power monitoring, thus:
    - Only two current channels (IA and IB) 
    - Only two active voltage channels (VA and VB) and a ground voltage channel
- A RaspberryPi Pico is used instead of an Arduino Zero. Similar specs (32 bit, 3.3V logic)
- Using a LiPo 1S battery to power the board instead of a microUSB port (?)
- Instead of using an EEPROM, we utilize the 2MB flash memeory of the RPi Pico (?)  
- yee haw

## Power consumption by ADE9000 side
Since we are using the 3.3V mode of the ADuM6404 isolator, we are limited to 132mW of power, or 132mW/3.3V = 40mA of current
1. ADE9000: I_in = 15 mA ()
2. LED: 2mA*5 = 10mA
3. ADuM4151 SPI isolator: 9 mA (17 mA max)
4. SN74LV4T125PWR buffer: 30 uA + 8mA output current
We seem to be just over the budget (42 mA). However the LEDs can be removed and the buffer can be removed since 3.3V -> 5V translation will not be needed.