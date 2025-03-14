#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define INVERSE_HALF_SOUND_SPEED 58
#define S_TO_US 1000000
#define TRIG_PIN 5
#define ECHO_PIN 18
#define DUTY_CYCLE_DURATION ((44 % 50) + 5)

#define DEBUG 1
#define DEBUG_PRINT(str) { \ 
  if (DEBUG) {             \
    Serial.println(str);   \
  }                        \
}

float distance;

float t_setup_total;
float t_measure_total;
float t_transmission_total;
float t_execution;
float t_deep_sleep_total = 0;
float t_wifi_off;
float t_wifi_on;


// MAC receiver
uint8_t broadcast_address[] = {0x8C, 0xAA, 0xB5, 0x84, 0xFB, 0x90};

esp_now_peer_info_t peer_info;

// Sending callback
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Ok" : "Error");
}

//Receiving Callback
void OnDataRecv(const uint8_t * mac, const uint8_t *data, int len) {
  Serial.print("Message received: ");
  char received_string[len];
  memcpy(received_string, data, len);
  Serial.println(String(received_string));
}

void setup_wifi_and_esp(){
  // Enabling the WIFI to send data
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_2dBm);
  // Initializing the esp now protocol
  esp_now_init();
  // Register send and receive callbacks
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
  // Peer Registration
  memcpy(peer_info.peer_addr, broadcast_address, 6);
  peer_info.channel = 0;  
  peer_info.encrypt = false;

  // Add peer (Is possible to register multiple peers)
  esp_now_add_peer(&peer_info);
}

void setup_hcsr04() {
  pinMode(TRIG_PIN, OUTPUT); // Sets the trig_pin as an Output
  pinMode(ECHO_PIN, INPUT); // Sets the echo_pin as an Input
}

void send_message(String message){
  // Start of the transmission process
  t_transmission_total = millis();

  DEBUG_PRINT("\n-------------------MESSAGE TRANSMISSION-------------------");

  // Setup the WIFI module and esp now protocol
  DEBUG_PRINT("\tActivating the WIFI module and esp_now protocol...");
  t_wifi_on = millis();
  setup_wifi_and_esp();
  DEBUG_PRINT("\t## Completed! ##");

  // Send the message to the sink
  DEBUG_PRINT("\tSending the message to the sink node...");
  esp_now_send(broadcast_address, (uint8_t*)message.c_str(), message.length() + 1);
  DEBUG_PRINT("\t## Completed! ##");

  // Turning off the WIFI module
  DEBUG_PRINT("\tTurning off the WIFI module");
  WiFi.mode(WIFI_OFF);
  t_wifi_on = millis() - t_wifi_on;

  t_transmission_total = millis() - t_transmission_total;
}

void setup() {
  t_setup_total = millis();
  Serial.begin(115200);
  DEBUG_PRINT("\n-------------------SETUP-------------------");
  DEBUG_PRINT("\tBoard woke up at: "+String(t_setup_total)+"ms");

  // Starting with WIFI module off
  DEBUG_PRINT("\tTurning off the WIFI module...");
  WiFi.mode(WIFI_OFF);
  t_wifi_off = millis();
  // Setup HC_SR04 sensor's PINs
  DEBUG_PRINT("\tConfiguring the HC_SR04 sensor...");
  setup_hcsr04();
  DEBUG_PRINT("\t## Completed! ##");


  // Compute the total time required by the board setup
  t_setup_total = millis() - t_setup_total;
}

float performDistanceRead() {
  // Start of the distance reading process
  t_measure_total = millis();

  DEBUG_PRINT("\n-------------------DISTANCE COMPUTATION-------------------");

  // Makes sure that the trig_pin is low before reading
  DEBUG_PRINT("\tResetting TRIG_PIN...");
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  DEBUG_PRINT("\t## Completed! ##");

  // Start a new measurement
  DEBUG_PRINT("\tStarting measurement...");
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  DEBUG_PRINT("\t## Completed! ##");

  // Read the measure
  DEBUG_PRINT("\tReading the measurement...");
  int duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration / INVERSE_HALF_SOUND_SPEED;
  DEBUG_PRINT("\t## Completed! ##");

  // Compute the total time required to perform the read
  t_measure_total = millis() - t_measure_total;

  return distance;
}

void loop() {
  // Compute the distance
  distance = performDistanceRead();

  // If the distance is < 50cm, it's considered OCCUPIED, otherwise it's considered FREE
  String message;
  if(distance < 50){
    message = "OCCUPIED";
  } else {
    message = "FREE";
  }

  // Send the message to the sink node
  send_message(message);


  //Serial.println("--------------BOARD TIMES--------------");
  //Serial.println("\tTime spent in setup: " + String(t_setup_total));
  //Serial.println("\tTime spent measuring the distance: " + String(t_measure_total));
  //Serial.println("\tTime spent for transmitting the result: " + String(t_transmission_total));
  //Serial.println("\tTime spent in deep sleep: " + String(t_deep_sleep_total));
  //Serial.println("\tTime spent in deep sleep: " + String(t_deep_sleep_total));


  // Setting the timer for waking up the board (board woke up after: (44 % 50) + 5 = 49s)
  int wake_up_timer = DUTY_CYCLE_DURATION * S_TO_US;
  DEBUG_PRINT("\tConfiguring the board to wake up at: "+String(millis()+wake_up_timer));
  esp_sleep_enable_timer_wakeup(wake_up_timer);

  
  t_execution = t_setup_total + t_measure_total + t_transmission_total;
  t_wifi_off = t_execution - t_wifi_on;
  t_deep_sleep_total = wake_up_timer/1000 - t_execution;

  Serial.println(String(t_setup_total)+","+String(t_measure_total)+","+String(t_transmission_total)+","+String(t_execution)+","+String(t_deep_sleep_total)+","+String(t_wifi_on)+","+String(t_wifi_off));

  DEBUG_PRINT("Board going into deep sleep....");
  esp_deep_sleep_start();
}
