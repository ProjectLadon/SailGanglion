
//#define DEBUG_MODE  (1)

#include <Servo.h>
#include <SparkFunMPU9250-DMP.h>
#include <WiFi101.h>
#include <aREST.h>

#define PROG_PIN  (0)
#define SAIL_PIN  (3)
#define COMPASS_MAG_ADDRESS (0x1e)
#define COMPASS_ACC_ADDRESS (0x68)
#define UDP_PORT  (13000)
#define AOA_PIN   (A0)
#define AOA_PWR   (A1)
#define AOA_GND   (A2)
#define IMU_PWR   (10)
#define IMU_GND   (9)

// IP Addresses
IPAddress foresailIP(192,168,0,90);
IPAddress mizzenIP(192,168,0,91);

// IMU object & addresses
#define AHRS true         // Set to false for basic data read
MPU9250_DMP imu;
bool imuGood = false;

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

// orientation functions
void processIMU();

// Create Servo instance
Servo sail;
#define SERVO_OFFSET  (90)
#define SERVO_RANGE   (30)

// Create variables for aREST
float sailHeading = 0;
float tailAngle = 0;

void setup(void) {
  // Start Serial
  Serial.begin(115200);
//  Wire.begin();

  // start servo
  sail.attach(SAIL_PIN);
  sail.write(90);

  // turn on LED output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // start the IMU
//  pinMode(IMU_GND, OUTPUT);
//  pinMode(IMU_PWR, OUTPUT);
//  digitalWrite(IMU_GND, LOW);
//  digitalWrite(IMU_PWR, HIGH);
  delay(1000);

  if (Serial) Serial.println("I live!");
    
  // Function to be exposed
  rest.function("sail",servoControl);

  // variables to be exposed
  rest.variable("heading", &sailHeading);
  rest.variable("tail", &tailAngle);

  
  if (imu.begin() != INV_SUCCESS) {
    while (1)
    {
      if (Serial) Serial.println("Unable to communicate with MPU-9250");
      if (Serial) Serial.println("Check connections, and try again.");
      if (Serial) Serial.println();
      delay(5000);
    }
  }
  imu.dmpBegin(DMP_FEATURE_6X_LP_QUAT | // Enable 6-axis quat
               DMP_FEATURE_GYRO_CAL, // Use gyro calibration
               10);

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

  // Print the IP address
  IPAddress ip = WiFi.localIP();
  if (Serial) Serial.print("IP Address: ");
  if (Serial) Serial.println(ip);

}

void loop() {

  // trigger IMU processing 
  if ( imu.fifoAvailable() ) {
    // Use dmpUpdateFifo to update the ax, gx, mx, etc. values
    if ( imu.dmpUpdateFifo() == INV_SUCCESS) {
      // computeEulerAngles can be used -- after updating the
      // quaternion values -- to estimate roll, pitch, and yaw
      //if (Serial) Serial.println("Getting compass data...");
      imu.computeEulerAngles();
      sailHeading = 360 - imu.yaw;
    }
  }

  // check for connection, reconnect if missing
  if (WiFi.status() == WL_CONNECTED) {
    //digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    if (Serial) Serial.println("Lost WiFi connection");
    if (Serial) Serial.print("Wifi status is: ");
    if (Serial) Serial.println(WiFi.status());
    WiFi.begin(ssid, password);
    delay(100);
    return;
  }

  // Handle REST calls
  WiFiClient client = server.available();
  if (!client) {
    //if (Serial) Serial.println("No client");
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
    if (Serial) Serial.print("Received "); if (Serial) Serial.print(client.available()); if (Serial) Serial.println(" bytes");
  } else {
    if (Serial) Serial.println("Connection established no bytes received");
  }
  rest.handle(client);
  client.stop();

  return;
}

// Custom function accessible by the API
int servoControl(String command) {

  // Get state from command
  if (Serial) Serial.print("Incoming command: ");
  if (Serial) Serial.println(command);
  int cmd = command.toInt() + SERVO_OFFSET;
  if (sailWrite(cmd)) {
    return cmd;
  } else return 0;
}

// Sail writing command
bool sailWrite(int cmd) {
  if (cmd > (SERVO_OFFSET + SERVO_RANGE)) cmd = (SERVO_OFFSET + SERVO_RANGE);
  if (cmd < (SERVO_OFFSET - SERVO_RANGE)) cmd = (SERVO_OFFSET - SERVO_RANGE);
  if (Serial) Serial.print("Executing command "); if (Serial) Serial.println(cmd);
  sail.write(cmd);
  tailAngle = cmd;
  return true;
}
