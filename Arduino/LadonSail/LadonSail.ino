#define DEBUG_MODE  (1)

#include <Servo.h>
#include <SPI.h>
#include <WiFi101.h>
#include <aREST.h>
#include <Wire.h>

#define PROG_PIN  (0)
#define SAIL_PIN  (3)
#define COMPASS_MAG_ADDRESS (0x1e)
#define COMPASS_ACC_ADDRESS (0x68)

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

// Declare functions to be exposed to the API
int servoControl(String command);

// Create Servo instance
Servo sail;

// Create variables for aREST
float aoa = 0;
float sailHeading = 0;

void setup(void)
{
  // Start Serial
  Serial.begin(115200);

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
  } else {
    rest.set_id("2");
    rest.set_name("mizzen");
    WiFi.config(mizzenIP);
  }

  // Connect to WiFi
  while (status != WL_CONNECTED) {
    if (Serial) Serial.print("Attempting to connect to SSID: ");
    if (Serial) Serial.println(ssid);
    status = WiFi.begin(ssid, password);

    // Wait 10 seconds for connection:
    delay(10000);
  }
  if (Serial) Serial.println("WiFi connected");

  // Start the server
  server.begin();
  if (Serial) Serial.println("Server started");

  // Print the IP address
  IPAddress ip = WiFi.localIP();
  if (Serial) Serial.print("IP Address: ");
  if (Serial) Serial.println(ip);

  sail.attach(SAIL_PIN);
  sail.write(90);
}

void loop() {

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  int cnt = 0;
  while(!client.available() && (cnt < 100)){
    delay(1);
    cnt++;
  }
  rest.handle(client);
  client.stop();
}

// Custom function accessible by the API
int servoControl(String command) {

  // Get state from command
  if (Serial) Serial.print("Incoming command: ");
  if (Serial) Serial.println(command);
  int cmd = command.toInt();
  sail.write(cmd);
  return cmd;
}
