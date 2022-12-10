# Platformer
Platformer is Team 2's game submission for EE1000 - Embedded Systems Term 1 project.

This code is designed and tested to run on an Arduino Uno (Any AVR microcontroller should work), to be displayed on a 32x32 LED Matrix, and to be interfaced through 2 joysticks. A full parts list can be found further below.

## Further Improvements
- The code can be further improved by adding a proper library to convert ASCII characters into their pixel positions to display on the screen. For now, it's tediously figuring out the location of each pixel for each character that needs to be implemented
- Currently, direct port manipulation is used with the help of binary algebra, but Inline Assembly (ASM) could be even faster
- For this specific purpose, only 1-bit colour depth is needed, but it is possible to achieve 4-bit colour
- Currently, the random seed gets it's noise from a non-existent (but defined) pin on the Arduino Uno: analog A6. But implementing a LM35 temperature sensor would yield more random results
- Landscapes and a more dynamic background can be implemented

## Image Gallery
- Casing showcase:
<img src="https://user-images.githubusercontent.com/56694167/206869108-90fdcb13-a651-4462-9a39-df489e1d2204.png" width="200">

- States:
<img src="https://user-images.githubusercontent.com/56694167/206869163-221dfe78-acda-4a38-980b-ed95d6be8e4e.png" width="200">

- Gravity (Notice blue player falling): 
<img src="https://user-images.githubusercontent.com/56694167/206869216-4873c1e5-1b5b-43c0-a2c8-2625f9dbaf60.png" width="200">

## Parts list
| Part          | Price (Total)        | Amount        |
| ------------- |:--------------------:|:-------------:|
| [Adafruit LED 32x32 Matrix](https://shop.pimoroni.com/products/rgb-led-matrix-panel?variant=19321740999) | £18    | 1 |
| [Seeed Thumb Joystick](https://www.rapidonline.com/seeed-101020028-grove-thumb-joystick-3-3-5v-75-0457)  | £5.44  | 2 |
| [Arduino Uno](https://www.rapidonline.com/arduino-uno-a000066-board-r3-73-4440)                          | £17.25 | 1 |

All prices are as of the 10th of December, 2022.

<img src="https://user-images.githubusercontent.com/56694167/206868385-9546e384-f990-4d5d-8816-c6bc763fd15a.png" width="150">
