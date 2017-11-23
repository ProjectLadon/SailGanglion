//#define DEBUG_MODE  (1)

#include <Servo.h>
#include <SPI.h>
#include <WiFi101.h>
#include <aREST.h>
#include <Wire.h>
#include <LSM9DS1_Registers.h>
#include <LSM9DS1_Types.h>
#include <SparkFunLSM9DS1.h>

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
IPAddress controllerIP(192,168,0,92);

// IMU object & addresses
LSM9DS1 imu;
#define LSM9DS1_M   0x1E // Would be 0x1C if SDO_M is LOW
#define LSM9DS1_AG  0x6B // Would be 0x6A if SDO_AG is LOW
bool imuGood = false;

// magnetic corrections
#define FORESAIL_MAG_X_OFFSET   (-1247)
#define FORESAIL_MAG_Y_OFFSET   (-3754)
#define FORESAIL_MAG_Z_OFFSET   (-9061)
#define FORESAIL_MAG_X_GAIN     (0.0380)
#define FORESAIL_MAG_Y_GAIN     (0.0459)
#define FORESAIL_MAG_Z_GAIN     (0.0288)
#define MIZZEN_MAG_X_OFFSET     (-3222)
#define MIZZEN_MAG_Y_OFFSET     (-4112)
#define MIZZEN_MAG_Z_OFFSET     (-1149)
#define MIZZEN_MAG_X_GAIN       (0.0363)
#define MIZZEN_MAG_Y_GAIN       (0.0434)
#define MIZZEN_MAG_Z_GAIN       (0.0250)
int magOffsets[3];
float magGains[3];

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
float magScale(int val, int offset, float gain);
float getHeading(float mX, float mY, float mZ, float aX, float aY, float aZ);
float rad2deg(float rad);

// Create Servo instance
Servo sail;
#define SERVO_OFFSET  (90)
#define SERVO_RANGE   (30)

// Create variables for aREST
float aoa = 0;
bool hasAoA = false;
float sailHeading = 0;
float tailAngle = 0;

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

  // start the IMU
  pinMode(IMU_GND, OUTPUT);
  pinMode(IMU_PWR, OUTPUT);
  digitalWrite(IMU_GND, LOW);
  digitalWrite(IMU_PWR, HIGH);
  delay(100);
  imu.settings.device.commInterface = IMU_MODE_I2C;
  imu.settings.device.mAddress = LSM9DS1_M;
  imu.settings.device.agAddress = LSM9DS1_AG;
  if (imu.begin()) {
    imuGood = true;
    if (Serial) Serial.println("IMU started");;
  } else {
    if (Serial) Serial.println("IMU failed to start");;
  }

  // Function to be exposed
  rest.function("sail",servoControl);

  // variables to be exposed
  rest.variable("heading", &sailHeading);
  rest.variable("aoa", &aoa);
  rest.variable("tail", &tailAngle);

  // Check if we are foresail or mizzen
  // Give name and ID to device
  pinMode(PROG_PIN, INPUT);
  if  (digitalRead(PROG_PIN) == HIGH) {
    rest.set_id("1");
    rest.set_name("foresail");
    WiFi.config(foresailIP);
    hasAoA = true;
    pinMode(AOA_PWR, OUTPUT);
    pinMode(AOA_GND, OUTPUT);
    digitalWrite(AOA_PWR, HIGH);
    digitalWrite(AOA_GND, LOW);
    magOffsets[0] = FORESAIL_MAG_X_OFFSET;
    magOffsets[1] = FORESAIL_MAG_Y_OFFSET;
    magOffsets[2] = FORESAIL_MAG_Z_OFFSET;
    magGains[0] = FORESAIL_MAG_X_GAIN;
    magGains[1] = FORESAIL_MAG_Y_GAIN;
    magGains[2] = FORESAIL_MAG_Z_GAIN;
    if (Serial) Serial.println("Foresail");
  } else {
    rest.set_id("2");
    rest.set_name("mizzen");
    WiFi.config(mizzenIP);
    aoa = 180;
    magOffsets[0] = MIZZEN_MAG_X_OFFSET;
    magOffsets[1] = MIZZEN_MAG_Y_OFFSET;
    magOffsets[2] = MIZZEN_MAG_Z_OFFSET;
    magGains[0] = MIZZEN_MAG_X_GAIN;
    magGains[1] = MIZZEN_MAG_Y_GAIN;
    magGains[2] = MIZZEN_MAG_Z_GAIN;
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

  // read windvane data
  if (hasAoA) aoa = analogRead(A0);

  // read orientation sensor data
  if (imu.accelAvailable() && imu.magAvailable()) {
    imu.readMag(); 
    imu.readAccel(); 
    // Call print attitude. The LSM9DS1's mag x and y
    // axes are opposite to the accelerometer, so my, mx are
    // substituted for each other.
    sailHeading = getHeading(magScale(-imu.my, magOffsets[1], magGains[1]),
                             magScale(-imu.mx, magOffsets[0], magGains[0]),
                             magScale(imu.mz, magOffsets[2], magGains[2]),
                             imu.ax, imu.ay, imu.az);
  }

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
  tailAngle = cmd;
  return true;
}

float magScale(int val, int offset, float gain) {
  return gain * (float)(val + offset) ;
}

float rad2deg(float rad) {
  return (rad * 180)/PI;
}

// mag tilt compensation, per http://www.cypress.com/file/130456/download
float getHeading(float mX, float mY, float mZ, float aX, float aY, float aZ) {
  float Atotal = sqrt(aX*aX + aY*aY + aZ*aZ);
  float Ax = aX/Atotal;
  float Ay = aY/Atotal;
  float B = 1 - (Ax*Ax);
  float C = Ax * Ay;
  float D = sqrt(1 - (Ax*Ax) - (Ay*Ay));
  float x = mX*B - mY*C - mZ*Ax*D;       // equation 18
  float y = mY*D - mZ*Ay;                // equation 19
  float result = rad2deg(atan2(y,x));
  if (Serial) {
    Serial.print("mX: ");
    Serial.print(mX);
    Serial.print(" mY: ");
    Serial.print(mY);
    Serial.print(" mZ: ");
    Serial.print(mZ);
    Serial.print(" Heading: ");
    Serial.println(result);
  }
  return result;
}


