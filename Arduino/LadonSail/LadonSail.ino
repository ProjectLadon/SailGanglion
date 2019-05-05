
//#define DEBUG_MODE  (1)

#define MKR1000

#include <Servo.h>
#include <SparkFunMPU9250-DMP.h>
#include <WiFi101.h>
#include <ros.h>
#include <std_msgs/Float32.h>

#define PROG_PIN  (0)
#define SAIL_PIN  (3)
#define COMPASS_MAG_ADDRESS (0x1e)
#define COMPASS_ACC_ADDRESS (0x68)
#define AOA_PIN   (A0)
#define AOA_PWR   (A1)
#define AOA_GND   (A2)
#define IMU_PWR   (10)
#define IMU_GND   (9)

// IP Addresses
IPAddress foresailIP(192,168,8,90);
IPAddress mizzenIP(192,168,8,91);
IPAddress ros_server(192,168,8,1);
const uint16_t ros_port = 11411;

// IMU object & addresses
#define AHRS true         // Set to false for basic data read
MPU9250_DMP imu;
bool imuGood = false;

// Status
int status = WL_IDLE_STATUS;

// Create ROS messages and pointers for subscribers and publishers
ros::NodeHandle     nh;
std_msgs::Float32   cmd_msg;
std_msgs::Float32   hdg_msg;
std_msgs::Float32   tail_msg;
ros::Publisher*     hdg_publisher;
ros::Publisher*     tail_publisher;
ros::Subscriber<std_msgs::Float32>* cmd_subscriber;

// WiFi parameters
char ssid[] = "BeagleBone-DEB1";
char password[] = "BeagleBone";

// Declare functions to be exposed to the API
void tail_callback(const std_msgs::Float32& cmd);

// Declare sail write function
bool sailWrite (int cmd);

// Create Servo instance
Servo sail;
#define SERVO_OFFSET  (90)
#define SERVO_RANGE   (45)

void setup(void) {
  // Start Serial
  Serial.begin(115200);

  // start servo
  sail.attach(SAIL_PIN);
  sail.write(SERVO_OFFSET);

  // turn on LED output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);

  if (Serial) Serial.println("I live!");
    
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
  // Create ROS publishers and subscribers
  pinMode(PROG_PIN, INPUT);
  if  (digitalRead(PROG_PIN) == HIGH) {
    WiFi.config(foresailIP);
    static ros::Subscriber<std_msgs::Float32> tailsub("/foresail/cmd", tail_callback);
    static ros::Publisher hdgpub("/foresail/heading", &hdg_msg);
    static ros::Publisher tailpub("/foresail/tail", &tail_msg);
    cmd_subscriber = &tailsub;
    hdg_publisher = &hdgpub;
    tail_publisher = &tailpub;
    if (Serial) Serial.println("Foresail");
  } else {
    WiFi.config(mizzenIP);
    static ros::Subscriber<std_msgs::Float32> tailsub("/mizzen/cmd", tail_callback);
    static ros::Publisher hdgpub("/foresail/heading", &hdg_msg);
    static ros::Publisher tailpub("/foresail/tail", &tail_msg);
    cmd_subscriber = &tailsub;
    hdg_publisher = &hdgpub;
    tail_publisher = &tailpub;
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

  // Print the IP address
  IPAddress ip = WiFi.localIP();
  if (Serial) Serial.print("IP Address: ");
  if (Serial) Serial.println(ip);

  // Start ROS
  nh.getHardware()->setConnection(ros_server, ros_port);
  nh.initNode();
  nh.advertise(*hdg_publisher);
  nh.advertise(*tail_publisher);
  nh.subscribe(*cmd_subscriber);

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
      hdg_msg.data = 360 - imu.yaw;
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

  // ROS publication
  hdg_publisher->publish(&hdg_msg);
  tail_publisher->publish(&tail_msg);

  // ROS Spin
  nh.spinOnce();
  delay(50);

  return;
}

// Custom function accessible by the API
void tail_callback(const std_msgs::Float32& cmd) {

  // Get state from command
  if (Serial) Serial.print("Incoming command: ");
  if (Serial) Serial.println(cmd.data);
  int cmd_int = (int)(cmd.data + SERVO_OFFSET);
  if (sailWrite(cmd_int)) {
    tail_msg.data = cmd_int;
  } else tail_msg.data = 0;
}

// Sail writing command
bool sailWrite(int cmd) {
  if (cmd > (SERVO_OFFSET + SERVO_RANGE)) cmd = (SERVO_OFFSET + SERVO_RANGE);
  if (cmd < (SERVO_OFFSET - SERVO_RANGE)) cmd = (SERVO_OFFSET - SERVO_RANGE);
  if (Serial) Serial.print("Executing command "); if (Serial) Serial.println(cmd);
  sail.write(cmd);
  return true;
}
