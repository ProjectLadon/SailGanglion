# SailGanglion
This is the brains of each sail. This covers the electronics, sensors, and software for each sail. The master branch is currently devoted to the subscale test vessel; future variants will get their own branches. 

## Required Design Properties
* Continuously controls the orientation of the tail of each wing in response to commands.
* Tail orientation commanded +/- 30 degrees of neutral
* Battery powered
* Receives commands via a WiFi REST interface 
* Senses and transmits angle of attack data on REST request
* Senses and transmits sail heading data on REST request

## Hardware Components
* Arduino MKR1000 (https://store.arduino.cc/usa/arduino-mkr1000)
* Hitec HS-81 Servo (http://hitecrcd.com/products/servos/micro-and-mini-servos/analog-micro-and-mini-servos/hs-81-standard-micro-servo/product)
* Sparkfun 9Dof Sensor Stick (https://www.sparkfun.com/products/13944)
* AOA sensor based on TLV494

## Mechanical Design
The sail was designed in Fusion 360. There will be a link here...

## Libraries
* aREST (https://github.com/marcoschwartz/aREST)
* TLV494 driver (AOA sensor, https://github.com/ProjectLadon/TLV493D-3D-Magnetic-Sensor-Arduino-Library)
* LSM9DS1 driver (9 DOF sensors, https://github.com/sparkfun/SparkFun_LSM9DS1_Arduino_Library)
