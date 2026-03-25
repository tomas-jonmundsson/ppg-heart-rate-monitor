#include "BluetoothSerial.h"

#define LED_PIN 32
#define SWITCH_PIN 33
#define SENSOR_PIN 25

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#include "BluetoothSerial.h"
#include <Esp.h>
//#include "btSupport.h"

String btName = "ESP32_tomj";
bool connected;

BluetoothSerial SerialBT;

// Includes a class to debounce switches
#include "switch.h"

// Defines pins for sensor, LED, and switch
#define SENSOR_PIN 25
#define LED_PIN 32
#define SWITCH_PIN 33

// Variable Definitions
#define TICK_20_MS 20000
#define TICK_1_SEC 1000000
#define DEBOUNCE_CNT 5

// Variable Initialisations
bool switch_state; // Checks whether the switch is pressed (true) or not pressed (false)
int counter=0;
bool pulse_detected = false;
bool valid_pulse_found = false;

// holds tick time counts
unsigned long last_tick_time;
unsigned long last_1sec_time;

// Variables to store pulse timing
unsigned long last_transition = 0;
unsigned long new_transition = 0;
unsigned long pulse_period = 0;
float heartrate = 0.0;

uint16_t sensor_reading;
uint16_t max_reading = 0;
uint16_t min_reading = 65535;
uint16_t threshold = 0;
uint8_t packet_ID = 0;

Switch mySwitch(1,DEBOUNCE_CNT);

int sensor_index = 0;
int hr_index = 0;
int switch_index = 0;
uint16_t sensor_array[54];
uint16_t heart_rate_array[1];
uint16_t switch_array[128];

void setup() 
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);
  pinMode(SENSOR_PIN, INPUT);

  last_tick_time = micros();
  last_1sec_time = micros();

  Serial.println("****************************");
  Serial.println("* ESP Bluetooth *");
  Serial.println("****************************");
  Serial.println("\nNow run the host program...");

  SerialBT.register_callback(btCallback);

  SerialBT.begin(btName); //Bluetooth device name

  if (!SerialBT.begin(btName))
  {
    Serial.println("An error occurred initializing Bluetooth");
  }
  else
  {
    Serial.println("Bluetooth initialized");
    Serial.println(btName);
  }
}

void loop() 
{
  if ((micros() - last_tick_time) >= TICK_20_MS) 
  {
    //timestamp the beginning of this loop
    last_tick_time = micros();

    sensor_reading = analogRead(SENSOR_PIN);
    bool switch_state = digitalRead(SWITCH_PIN);

    sensor_array[sensor_index] = sensor_reading;
    switch_array[switch_index] = switch_state;

    sensor_index = (sensor_index + 1) % 50;  // Wrap around
    switch_index = (switch_index + 1) % 50;  // Wrap around

      if (sensor_reading > max_reading) 
    {
      max_reading = sensor_reading;
    }
    if (sensor_reading < min_reading) 
    {
      min_reading = sensor_reading;
   }

    Serial.print("Sensor Reading: ");
    Serial.println(sensor_reading);

    // Check if a pulse is detected (rising edge)
    if (!valid_pulse_found && !pulse_detected && sensor_reading > threshold) {
      new_transition = micros();  // Record the time of the rising edge
      pulse_period = new_transition - last_transition;  // Calculate pulse period
      last_transition = new_transition;

      // Calculate heart rate only if pulse period is valid
      if (pulse_period > 300000) {  // Ignore periods shorter than 300 ms (~200 BPM)
        valid_pulse_found = true;
        heartrate = 60000 / ((float)pulse_period / 1000);  // Calculate heart rate in BPM
      }

      pulse_detected = true;  // Mark pulse as detected
      mySwitch.update(true);  // Switch to 'pulse detected' state
    }

    // Reset detection when sensor reading drops below threshold (falling edge)
    if (pulse_detected && sensor_reading < threshold) {
      mySwitch.update(false);  // Reset the switch state
      pulse_detected = false;  // Ready for the next pulse
    }
    
  }

  if ((micros() - last_1sec_time) >= TICK_1_SEC)
  {
    last_1sec_time = micros();

    // Calculate threshold and reset max/min values for the next second
    threshold = (max_reading + min_reading) / 2;  // Add an offset to improve pulse detection
    
    // Debugging info for threshold
    Serial.print("Threshold: ");
    Serial.println(threshold);
    //Serial.print("Pulse Period: ");
    //Serial.println(pulse_period);


    // Reset min and max readings for the next second
    max_reading = 0;
    min_reading = 65535;

    // Calculate heart rate only if a pulse period was detected
    if (valid_pulse_found) {
      Serial.print("Heart rate: ");
      Serial.println(heartrate);
      heart_rate_array[0] = heartrate;
      valid_pulse_found = false;
    }
    sendBluetoothData();
  }
}

void sendBluetoothData() {
  // Increment packet ID
  packet_ID++;
  
  // Create a string to hold the packet data
  String packet = "Packet ID: " + String(packet_ID) + "\n";

  // Append sensor array data
  packet += "Sensor Readings: ";
  for (int i = 0; i < 50; i++) {
    packet += String(sensor_array[i]);
    if (i < 49) packet += ", ";  // Add commas between values
  }
  packet += "\n";

  // Append heart rate data
  packet += "Heart Rate: " + String(heart_rate_array[0]) + "\n";

  // Append switch array data
  packet += "Switch States: ";
  for (int i = 0; i < 50; i++) {
    packet += String(switch_array[i]);
    if (i < 49) packet += ", ";  // Add commas between values
  }
  packet += "\n";

  // Send the packet over Bluetooth
  SerialBT.println(packet);

  // Print to Serial Monitor for debugging
  Serial.println(packet);
  
  // Reset the arrays after sending data
  memset(sensor_array, 0, sizeof(sensor_array));  // Clear the sensor array
  memset(heart_rate_array, 0, sizeof(heart_rate_array));  // Clear the heart rate array
  memset(switch_array, 0, sizeof(switch_array));  // Clear the switch array

  // Reset the array indices
  sensor_index = 0;
  hr_index = 0;
  switch_index = 0;
}

int openEvt = 0;

void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
//
// This function displays SPP events when they occur. This provides 
// information on what is hapening on the bluetooth link.
//
//
{
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    char buf[50];
    openEvt++;
    sprintf(buf, "Client Connected:%d", openEvt);
    Serial.println(buf);
    Serial.print("  Address = ");

    for (int i = 0; i < 6; i++)
    {
      sprintf(&(buf[i * 3]), "%02X:", param->srv_open.rem_bda[i]);
    }
    buf[17] = 0;
    Serial.println(buf);
  }


  if (event == ESP_SPP_INIT_EVT)
    Serial.println("ESP_SPP_INIT_EVT");
  else if (event == ESP_SPP_UNINIT_EVT)
    Serial.println("ESP_SPP_INIT_EVT");
  else if (event == ESP_SPP_DISCOVERY_COMP_EVT )
    Serial.println("ESP_SPP_DISCOVERY_COMP_EVT");
  else if (event == ESP_SPP_OPEN_EVT )
    Serial.println("ESP_SPP_OPEN_EVT");
  else if (event == ESP_SPP_CLOSE_EVT )
    Serial.println("ESP_SPP_CLOSE_EVT");
  else if (event == ESP_SPP_START_EVT )
    Serial.println("ESP_SPP_START_EVT");
  else if (event == ESP_SPP_CL_INIT_EVT )
    Serial.println("ESP_SPP_CL_INIT_EVT");
  else if (event == ESP_SPP_DATA_IND_EVT )
    Serial.println("ESP_SPP_DATA_IND_EVT");
  else if (event == ESP_SPP_CONG_EVT )
    Serial.println("ESP_SPP_CONG_EVT");
  else if (event == ESP_SPP_WRITE_EVT )
    Serial.println("ESP_SPP_WRITE_EVT");
  else if (event == ESP_SPP_SRV_OPEN_EVT )
    Serial.println("ESP_SPP_SRV_OPEN_EVT");
  else if (event == ESP_SPP_SRV_STOP_EVT )
    Serial.println("ESP_SPP_SRV_STOP_EVT");
  else
  {
    Serial.print("EV: ");
    Serial.println(event);
  };
}
