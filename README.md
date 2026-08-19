# ENITIO26-Escape-Room-Puzzle-Box
Code for the puzzle box (panic room) under Escape Room for ENITIO AY2026/2027 (12 Zodiac)


# General Overview
This is the puzzle box station from ENITIO26, which consists of three stages, each stage has several levels. Details on implementation will be explained in later sections.


  Stage 1 involves the use of the hand crank, the use of which will cause the LEDs to light up in sequence, reflecting the speed at which the crank is being turned. Participants will need to hold the LEDs at a certain level which will be given via a sequence of puzzles/riddles.

  Stage 2 only involves the use of the toggle switches, which will control which LEDs are light up. Each switch will toggle certain LEDs, e.g. the first switch toggles the 2nd, 5th and 7th LED. MSB is the top leftmost switch for this stage, and LSB is the bottom rightmost switch.

Stage 3 involves the use of the toggle switches and the button, and participants are required to solve the questions on the paper provided to get a password, which they will use the switches as bits to indicate a certain ASCII character. In order to enter their choice of character once they have selected the desired character, they must press the button to store the current character into the buffer. The MSB and LSB are now swapped, meaning the way that the bits are read is now reversed. (BUG, REQUIRES FIXING)
