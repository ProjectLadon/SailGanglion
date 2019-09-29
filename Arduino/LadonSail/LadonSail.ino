
#define DEBUG_MODE  (1)

// Get the very base of the stack...
char* stack_start;

#define ESP8266

#include <Servo.h>
#include <SparkFunMPU9250-DMP.h>
#include <ESP8266WiFi.h>
#include <ros.h>
#include <std_msgs/Int32.h>
//#include <GDBStub.h>

/* 
* Sail mode is selected by tieing pin to gnd. Pins are pulled up internally.
* Single-sail mode is selected if both pins are left floating.
* Tieing both pins to gnd will halt the process.
*/
#define FORESAIL_SELECT_PIN  (14)
#define MIZZEN_SELECT_PIN    (12)

#define SAIL_SERVO_PIN  (2)
#define COMPASS_MAG_ADDRESS (0x1e)
#define COMPASS_ACC_ADDRESS (0x68)

// Create ROS messages and pointers for subscribers and publishers
ros::NodeHandle*    nh = new ros::NodeHandle();
std_msgs::Int32*  hdg_msg = new std_msgs::Int32();
std_msgs::Int32*  tail_msg = new std_msgs::Int32();
ros::Publisher*     hdg_publisher;
ros::Publisher*     tail_publisher;
ros::Subscriber<std_msgs::Int32>* cmd_subscriber;

// IP Addresses
IPAddress foresailIP(192,168,8,90);
IPAddress mizzenIP(192,168,8,91);
IPAddress mainIP(192,168,8,92);
IPAddress ros_server(192,168,8,1);
IPAddress netmask(255,255,255,0);
const uint16_t ros_port = 11411;

// IMU object & addresses
#define AHRS true         // Set to false for basic data read
MPU9250_DMP* imu = new MPU9250_DMP();
bool imuGood = false;

// WiFi parameters
char ssid[] = "BeagleBone-DEB1";
char password[] = "BeagleBone";

// Declare functions to be exposed to the API
void tail_callback(const std_msgs::Int32& cmd);

// Declare sail write function
bool sailWrite (int cmd);

// Create Servo instance
Servo sail;
int32_t servoOffset = 90;
#define FORESAIL_OFFSET  (90)
#define MIZZEN_OFFSET  (90)
#define SINGLE_OFFSET  (90)
#define SERVO_RANGE   (45)

void setup(void) {
  // Get the start of the stack
  char stack;
  stack_start = &stack;
  
  // Start Serial
  Serial.begin(115200);
  // gdbstub_init();

  // start servo
  sail.attach(SAIL_SERVO_PIN);
  sail.write(servoOffset);

  // turn on LED output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);

  if (DEBUG_MODE) Serial.println(F("I live!"));
    
  if (imu->begin() != INV_SUCCESS) {
    while (1)
    {
      if (DEBUG_MODE) Serial.println(F("Unable to communicate with MPU-9250"));
      if (DEBUG_MODE) Serial.println(F("Check connections, and try again."));
      if (DEBUG_MODE) Serial.println();
      delay(5000);
    }
  } else if (DEBUG_MODE) Serial.println("Connected to MPU-9250");
  if ((imu->dmpBegin(DMP_FEATURE_6X_LP_QUAT | // Enable 6-axis quat
               DMP_FEATURE_GYRO_CAL, // Use gyro calibration
               10) != INV_SUCCESS) && DEBUG_MODE) {
                Serial.println("DMP start failed!");
               }

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  
  // Check if we are foresail or mizzen
  // Create ROS publishers and subscribers
  pinMode(FORESAIL_SELECT_PIN, INPUT_PULLUP);
  pinMode(MIZZEN_SELECT_PIN, INPUT_PULLUP);

  // Foresail only mode
  if (digitalRead(FORESAIL_SELECT_PIN) == LOW && digitalRead(MIZZEN_SELECT_PIN) == HIGH) {
    WiFi.config(foresailIP, ros_server, netmask);
    cmd_subscriber = new ros::Subscriber<std_msgs::Int32>("/foresail/cmd", tail_callback);
    hdg_publisher = new ros::Publisher("/foresail/heading", hdg_msg);
    tail_publisher = new ros::Publisher("/foresail/tail", tail_msg);
    if (DEBUG_MODE) Serial.println(F("Foresail program pins detected"));
    servoOffset = FORESAIL_OFFSET;

  // Mizzen only mode
  } else if (digitalRead(FORESAIL_SELECT_PIN) == HIGH && digitalRead(MIZZEN_SELECT_PIN) == LOW) {
    WiFi.config(mizzenIP, ros_server, netmask);
    cmd_subscriber = new ros::Subscriber<std_msgs::Int32>("/mizzen/cmd", tail_callback);
    hdg_publisher = new ros::Publisher("/mizzen/heading", hdg_msg);
    tail_publisher = new ros::Publisher("/mizzen/tail", tail_msg);
    if (DEBUG_MODE) Serial.println(F("Mizzen program pins detected"));
    servoOffset = MIZZEN_OFFSET;

  // Single-sail mode
  } else if (digitalRead(FORESAIL_SELECT_PIN) == HIGH && digitalRead(MIZZEN_SELECT_PIN) == HIGH) {
    WiFi.config(mainIP, ros_server, netmask);
    cmd_subscriber = new ros::Subscriber<std_msgs::Int32>("/sail/cmd", tail_callback);
    hdg_publisher = new ros::Publisher("/sail/heading", hdg_msg);
    tail_publisher = new ros::Publisher("/sail/tail", tail_msg);
    if (DEBUG_MODE) Serial.println(F("Single-sail program pins detected"));
    servoOffset = SINGLE_OFFSET;

  // Illegal mode, both selection pins pulled low
  } else {
    if (DEBUG_MODE) Serial.println(F("Illegal mode selection"));
    for (;;) {} 
  }

  // Connect to WiFi
  uint32_t wifi_conn_cnt = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    if (DEBUG_MODE) Serial.print(F("Attempting to connect to SSID: "));
    if (DEBUG_MODE) Serial.print(ssid);
    if (DEBUG_MODE) Serial.print(F(" Attempt #"));
    if (DEBUG_MODE) Serial.println(wifi_conn_cnt);
    wifi_conn_cnt++;

    // Wait 1 seconds for connection:
    delay(1000);
  }
  if (DEBUG_MODE) Serial.println(F("WiFi connected"));
  digitalWrite(LED_BUILTIN, HIGH);

  // Print the IP address
  IPAddress ip = WiFi.localIP();
  if (DEBUG_MODE) Serial.print(F("IP Address: "));
  if (DEBUG_MODE) Serial.println(ip);

  // Start ROS
  nh->getHardware()->setConnection(ros_server, ros_port);
  nh->initNode();
  nh->advertise(*hdg_publisher);
  nh->advertise(*tail_publisher);
  nh->subscribe(*cmd_subscriber);

}

void loop() {

  // trigger IMU processing 
  if ( imu->fifoAvailable() ) {
    // Use dmpUpdateFifo to update the ax, gx, mx, etc. values
    if (DEBUG_MODE) Serial.println(F("Compass data available..."));
    if ( imu->dmpUpdateFifo() == INV_SUCCESS) {
      // computeEulerAngles can be used -- after updating the
      // quaternion values -- to estimate roll, pitch, and yaw
      if (DEBUG_MODE) Serial.println(F("Getting compass data..."));
      imu->computeEulerAngles();
      hdg_msg->data = 360 - imu->yaw;
    } else if (DEBUG_MODE) {
      Serial.print("Compass data fetch failed."); 
    }
  }

  // check for connection, reconnect if missing
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    if (DEBUG_MODE) Serial.println(F("Lost WiFi connection"));
    if (DEBUG_MODE) Serial.print(F("Wifi status is: "));
    if (DEBUG_MODE) Serial.println(WiFi.status());
    WiFi.begin(ssid, password);
    delay(100);
    return;
  }

  // ROS publication
  hdg_publisher->publish(hdg_msg);
  tail_publisher->publish(tail_msg);
  
  // ROS Spin
  nh->spinOnce();
  delay(50);

  return;
}

// Custom function accessible by the API
void tail_callback(const std_msgs::Int32& cmd) {

  // Get state from command
  if (DEBUG_MODE) Serial.print("Incoming command: ");
  if (DEBUG_MODE) Serial.println(cmd.data);
  int cmd_int = (int)(cmd.data + servoOffset);
  if (sailWrite(cmd_int)) {
   tail_msg->data = cmd_int - servoOffset;
  } else tail_msg->data = 0;
}

// Sail writing command
bool sailWrite(int cmd) {
  if (cmd > (servoOffset + SERVO_RANGE)) cmd = (servoOffset + SERVO_RANGE);
  if (cmd < (servoOffset - SERVO_RANGE)) cmd = (servoOffset - SERVO_RANGE);
  if (DEBUG_MODE) Serial.print("Executing command "); if (DEBUG_MODE) Serial.println(cmd);
  sail.write(cmd);
  return true;
}
