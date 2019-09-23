# SailGanglion
This is the brains of each sail. This covers the electronics, sensors, and software for each sail. The master branch is currently devoted to the subscale test vessel; future variants will get their own branches. 

## Required Design Properties
* Continuously controls the orientation of the tail of each wing in response to commands.
* Tail orientation commanded +/- 30 degrees of neutral
* Battery powered
* Uses rosserial over WiFi for all communications
* Subscribes to tail commands
* Publishes sail heading commands

## Electronic Hardware Components
* Adafruit Feather Huzzah ESP8266 (https://www.adafruit.com/product/2821)
* Hitec HS-82MG Servo (https://hitecrcd.com/products/servos/micro-and-mini-servos/analog-micro-and-mini-servos/hs-82mg/product)
* Sparkfun MPU-9250 Breakout (https://www.sparkfun.com/products/13762)

## Mechanical Design
The sail was designed in Fusion 360. There will be a link here...

## Libraries
* rosserial (https://github.com/ros-drivers/rosserial)
* Sparkfun MPU-9650 DMP library, modified for ESP compatibility (https://github.com/rupin/SparkFun_MPU-9250-DMP_Arduino_Library)

## Pinout
* I2C pins -- per standard (GPIO4 & GPIO5)
* Servo PWM -- GPIO2
* Foresail Select (pull down) -- GPIO14
* Mizzen Select (pull down) -- GPIO12
* Leave foresail and mizzen select pins floating for single-sail operation
