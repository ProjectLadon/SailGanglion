//#define DEBUG_MODE  (1)

#include <Servo.h>
#include <SPI.h>
#include <WiFi101.h>
#include <aREST.h>
#include <Wire.h>
#include <WiFiUdp.h>

#define PROG_PIN  (0)
#define SAIL_PIN  (3)
#define COMPASS_MAG_ADDRESS (0x1e)
#define COMPASS_ACC_ADDRESS (0x68)
#define UDP_PORT  (13000)

// IP Addresses
IPAddress foresailIP(192,168,0,90);
IPAddress mizzenIP(192,168,0,91);
IPAddress controllerIP(192,168,0,92);

// Status
int status = WL_IDLE_STATUS;

// Create aREST instance
aREST rest = aREST();

// WiFi parameters
char ssid[] = "Underworld";
char password[] = "divedivedive";

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80

// Create an instance of the server
//WiFiServer server(LISTEN_PORT);
//WiFiUDP Udp;

// Declare functions to be exposed to the API
int servoControl(String command);

// Declare sail write function
bool sailWrite (int cmd);

// Create Servo instance
Servo sail;
#define SERVO_OFFSET  (90)
#define SERVO_RANGE   (30)

// Create variables for aREST
float aoa = 0;
float sailHeading = 0;

void setup(void)
{
  
  // Check if we are foresail or mizzen
  // Give name and ID to device
  pinMode(PROG_PIN, INPUT);
  if  (digitalRead(PROG_PIN) == HIGH) {
    rest.set_id("1");
    rest.set_name("foresail");
    WiFi.config(foresailIP);
  } else {
    rest.set_id("2");
    rest.set_name("mizzen");
    WiFi.config(mizzenIP);
  }

  sail.attach(SAIL_PIN);
  sail.write(90);
}

void loop() {
  sail.write(90);
  delay(15000);
  sail.write(105);
  delay(15000);
  sail.write(120);
  delay(15000);
  sail.write(75);
  delay(15000);
  sail.write(60);
  delay(15000);
  
}

// Custom function accessible by the API
int servoControl(String command) {

  // Get state from command
  if (Serial) Serial.print("Incoming command: ");
  if (Serial) Serial.println(command);
  int cmd = command.toInt();
  if (sailWrite(cmd)) {
    return cmd;
  } else return 0;
}

// Sail writing command
bool sailWrite(int cmd) {
  if (cmd > (SERVO_OFFSET + SERVO_RANGE)) return false;
  if (cmd < (SERVO_OFFSET - SERVO_RANGE)) return false;
  sail.write(cmd);
  return true;
}


