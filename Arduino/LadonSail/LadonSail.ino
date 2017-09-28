
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
WiFiServer server(LISTEN_PORT);
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
  // Start Serial
  Serial.begin(115200);

  // start servo
  sail.attach(SAIL_PIN);
  sail.write(90);

  // turn on LED output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Function to be exposed
  rest.function("sail",servoControl);

  // variables to be exposed
  rest.variable("heading", &sailHeading);
  rest.variable("aoa", &aoa);

  // Check if we are foresail or mizzen
  // Give name and ID to device
  pinMode(PROG_PIN, INPUT);
  if  (digitalRead(PROG_PIN) == HIGH) {
    rest.set_id("1");
    rest.set_name("foresail");
    WiFi.config(foresailIP);
    if (Serial) Serial.println("Foresail");
  } else {
    rest.set_id("2");
    rest.set_name("mizzen");
    WiFi.config(mizzenIP);
    if (Serial) Serial.println("Mizzen");
  }

  // Connect to WiFi
  while (status != WL_CONNECTED) {
    if (Serial) Serial.print("Attempting to connect to SSID: ");
    if (Serial) Serial.println(ssid);
    status = WiFi.begin(ssid, password);

    // Wait 1 seconds for connection:
    delay(1000);
  }
  if (Serial) Serial.println("WiFi connected");
  digitalWrite(LED_BUILTIN, HIGH);

  // Start the server
  server.begin();
  if (Serial) Serial.println("Server started");
//  Udp.begin(UDP_PORT);

  // Print the IP address
  IPAddress ip = WiFi.localIP();
  if (Serial) Serial.print("IP Address: ");
  if (Serial) Serial.println(ip);

}

void loop() {

  // check for connection, reconnect if missing
  if (WiFi.status() == WL_CONNECTED) {
    //digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    if (Serial) {
      Serial.println("Lost WiFi connection");
      Serial.print("Wifi status is: ");
      Serial.println(WiFi.status());
    }
    WiFi.begin(ssid, password);
    delay(100);
    return;
  }

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }
  int cnt = 0;
  while(!client.available() && (cnt < 50)) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1);
    cnt++;
  }
  if (client.available()) {
    if (Serial) {Serial.print("Received "); Serial.print(client.available()); Serial.println(" bytes");}
  } else {
    if (Serial) Serial.println("Connection established no bytes received");
  }
  rest.handle(client);
  client.stop();

  // Handle UDP calls
//  if (Udp.parsePacket()) {
//    IPAddress remote(Udp.remoteIP());
//    if (remote != controllerIP) {
//      if (Serial) Serial.println("UDP packet from invalid IP");
//    } else {
//      String command;
//      while (Udp.available()) {
//        command += Udp.read();
//      }
//      int cmd = command.toInt();
//      if (Serial) {
//        Serial.print("Executing UDP command: ");
//        Serial.println(cmd);
//      }
//      sailWrite(cmd);
//    }
//  }
  
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


