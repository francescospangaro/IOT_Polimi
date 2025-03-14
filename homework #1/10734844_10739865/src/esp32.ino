#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define INVERSE_HALF_SOUND_SPEED 58
#define S_TO_US 1000000
#define TRIG_PIN 5
#define ECHO_PIN 18
#define DUTY_CYCLE_DURATION ((44 % 50) + 5)

float distance;

float t_setup_total;
float t_measure_total;
float t_transmission_total;
float t_loop_total;
float t_deep_sleep_total = 0;
float t_wakeup;

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

  // Setup the WIFI module and esp now protocol
  setup_wifi_and_esp();
  // Send the message to the sink
  esp_now_send(broadcast_address, (uint8_t*)message.c_str(), message.length() + 1);
  // Turning off the WIFI module
  WiFi.mode(WIFI_OFF);

  t_transmission_total = millis() - t_transmission_total;
}

void setup() {
  //t_deep_sleep_total = millis() - t_deep_sleep_total;
  t_setup_total = millis();
  Serial.begin(115200);
  Serial.println("Board woke up at: "+String(t_setup_total)+"ms");

  // Starting with WIFI module off
  WiFi.mode(WIFI_OFF);

  // Setup HC_SR04 sensor's PINs
  setup_hcsr04();

  // Setting the timer for waking up the board (board woke up after: (44 % 50) + 5 = 49s)
  esp_sleep_enable_timer_wakeup(DUTY_CYCLE_DURATION * S_TO_US);

  // Compute the total time required by the board setup
  t_setup_total = millis() - t_setup_total;
}

float performDistanceRead() {
  // Start of the distance reading process
  t_measure_total = millis();

  // Makes sure that the trig_pin is low before reading
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Start a new measurement
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the measure
  int duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration / INVERSE_HALF_SOUND_SPEED;

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

  send_message(message);

  
  Serial.println("--------------BOARD TIMES--------------");
  Serial.println("\tTime spent in setup: " + String(t_setup_total));
  Serial.println("\tTime spent measuring the distance: " + String(t_measure_total));
  Serial.println("\tTime spent for transmitting the result: " + String(t_transmission_total));
  Serial.println("\tTime spent in deep sleep: " + String(t_deep_sleep_total));
  Serial.println("\tTime spent in deep sleep: " + String(t_deep_sleep_total));

  t_deep_sleep_total = millis();
  
  delay(10);
  esp_deep_sleep_start();
}