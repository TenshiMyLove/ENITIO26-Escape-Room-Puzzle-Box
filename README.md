# ENITIO26-Escape-Room-Puzzle-Box
Code for the puzzle box (panic room) under Escape Room for ENITIO AY2026/2027 (12 Zodiac).
Coded in C using the ESP-IDF v5.5.5 extension on VSCode. 
Schematic depicting circuit connections (as I remember) made using KiCad.





# General Overview
This is the puzzle box station from ENITIO26, which consists of three stages, each stage has several levels. Details on implementation will be explained in later sections.


  Stage 1 involves the use of the hand crank, the use of which will cause the LEDs to light up in sequence, reflecting the speed at which the crank is being turned. Participants will need to hold the LEDs at a certain level which will be given via a sequence of puzzles/riddles.

  Stage 2 only involves the use of the toggle switches, which will control which LEDs are light up. Each switch will toggle certain LEDs, e.g. the first switch toggles the 2nd, 5th and 7th LED. MSB is the top leftmost switch for this stage, and LSB is the bottom rightmost switch.

Stage 3 involves the use of the toggle switches and the button, and participants are required to solve the questions on the paper provided to get a password, which they will use the switches as bits to indicate a certain ASCII character. In order to enter their choice of character once they have selected the desired character, they must press the button to store the current character into the buffer. The MSB and LSB are now swapped, meaning the way that the bits are read is now reversed. (BUG, REQUIRES FIXING)





# Library Information
The library named QAPASS_LCD (both header and c file) is used for control over the 16x2 LCD used with a I2C backpack attached. This library was created by @voidloop at https://github.com/voidlooprobotech/ESP32_ESP-IDF_Code/tree/main/14_I2C_LCD_16x2

The library named 74HC595 is used for control over the shift register 74HC595, which in turn controls the LEDs on the box. This library was created by TenshiMyLove, and is designed only for outputs of 8-bit per function call. 





# Known Bugs & Issues
- 4 LEDs directly under the control of the GPIO pins of the ESP32 suffer from resolution issues due to rushed implementation (Less than a day to implement, feel free to change the implementation or rewrite the function as needed)
- storeBitsCmd causes the toggle switches to be read backwards from intended in stage 3, likely due to shift register receiving reversed byte for previous 2 stages 
- Loose jumper wires periodically causing IOs to be unresponsive





# Improvements to be made
- PCD design for the board for better connection
- LED update implementation rewrite
