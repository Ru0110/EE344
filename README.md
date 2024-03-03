# EE344
Auto-dialling Alarm System for Temperature and Humidity Monitoring

## Changes from original shield
- 1 Phase residential 3 Wire power monitoring, thus:
    - Only two current channels (IA and IB) 
    - Only two active voltage channels (VA and VB) and a ground voltage channel
- A RaspberryPi Pico is used instead of an Arduino Zero. Similar specs (32 bit, 3.3V logic)
- Using a LiPo 1S battery to power the board instead of a microUSB port (?)
- Instead of using an EEPROM, utilize the 2MB flash memeory of the RPi Pico (?)  
- yee haw