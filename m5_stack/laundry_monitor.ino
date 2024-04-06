#include <M5Core2.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "esp_wpa2.h"

// ---------- STRUCTS ----------

// accelerometer data structs
struct Vector {
  float x = 0, y = 0, z = 0;
  float get_magnitude() {
    return sqrt(x * x + y * y + z * z);
  }
};

// ---------- Helper Functions Forward Declarations -------------

// machine data helper functions
void init_machine_data();
String display_menu(const String[], int);
void display_menu_option(String);
void set_display_state(String, String message = "");

// wifi helper functions
String send_state(String);
String get_url_with_state(String);
String get_request(String);

// ---------- CONSTANTS ----------

// accelerometric data constants
#define INTERVAL_SIZE 100
#define ACC_MEAN 1.0F
#define ACC_SCALAR 100.0F
const double min_working_vibration = 3;
const unsigned long acc_delay = 10;

// wifi constants
const String ssid = "nyu";
const String network_username = "se2422";
const String network_password = "SMHME04@nyu2";
const String server_ip = "10.225.71.5";
const String server_port = "8080";

// machine data constants
const String buildings[] = {
  "a1a", "a1b", "a1c", "a2a", "a2b", "a2c",
  "a3", "a4", "a5a", "a5b", "a5c",
  "a6a", "a6b", "a6c"
};
const int building_count = sizeof(buildings) / sizeof(buildings[0]);
const String machine_ids[] = {
  "w1", "w2", "w3", "w4", "w5", "w6",
  "d1", "d2", "d3", "d4", "d5", "d6"
};
const int machine_ids_count = sizeof(machine_ids) / sizeof(machine_ids[0]);
const unsigned long used_to_free_delay = 5000;
const unsigned long free_to_used_delay = 5000;
const unsigned long reservation_delay = 5000;


// display constants
const unsigned long touch_delay = 100;

// ---------- Global Variables ----------

// machine data global variables
String building = "";
String machine_id = "";
String state = "FREE";
unsigned long last_free_time = 0;
unsigned long last_used_time = 0;
unsigned long reservation_time = 0;

// acceloremeter data global variables
unsigned long last_acc_time = 0;
float acc_squareddevs_sum = 0.0F, acc_stddev = 0.0F;
float acc_squareddevs[INTERVAL_SIZE];
int acc_index = 0;

// ---------- MAIN Program ----------

void setup() {
  // init M5Stack and IMU sensor unit
  M5.begin();
  M5.IMU.Init();

  // start serial communication
  Serial.begin(9600);

  // setup wifi connection
  WiFi.disconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(
    ssid.c_str(),
    WPA2_AUTH_PEAP,
    network_username.c_str(),
    network_username.c_str(),
    network_password.c_str()
  );
  
  // WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // initialise monitored machine data
  init_machine_data();

  // set initial state
  state = "FREE";
  last_free_time = millis();
  set_display_state(state);
  send_state(state);
}

void loop() {
  // Calculating Accelerometric Standdard Deviation
  if ((millis() - last_acc_time) >= acc_delay) {
    // collect accelerometer data
    Vector acc;
    M5.IMU.getAccelData(&acc.x, &acc.y, &acc.z);

    // keeping track of the deviations squared for the last
    // INTERVAL_SIZE datapoints
    acc_squareddevs_sum -= acc_squareddevs[acc_index];
    float curr_dev = ACC_SCALAR * (ACC_MEAN - acc.get_magnitude());
    acc_squareddevs[acc_index] = curr_dev * curr_dev;
    acc_squareddevs_sum += acc_squareddevs[acc_index];
    acc_index = (acc_index + 1) % INTERVAL_SIZE;

    // calculating the acceleration magnitude standard deviation
    acc_stddev = sqrt(acc_squareddevs_sum / INTERVAL_SIZE);

    // display acc_stddev
    Serial.print("acc_stddev: ");
    Serial.println(acc_stddev);

    if (acc_stddev >= min_working_vibration) {
      if (
        state != "USED" &&
        (millis() - last_free_time) >= free_to_used_delay
      ) {
        state = "USED";
        send_state(state);
        set_display_state(state);
      }

      last_used_time = millis();
    }

    if (acc_stddev < min_working_vibration) {
      if (
        (state == "USED" &&
        (millis() - last_used_time) >= used_to_free_delay) ||
        (state == "RESERVED" &&
        (millis() - reservation_time) >= reservation_delay)
      ) {
        state = "FREE";
        String payload = send_state(state);
        StaticJsonDocument<200> json;
        deserializeJson(json, payload);
        const char* json_state = json["state"];
        Serial.print("json_state: ");
        Serial.println(json_state);
        if (!strcmp(json_state,"RESERVED")) {
          state = "RESERVED";
          const char* json_reserver = json["reserver"];
          set_display_state(state, json_reserver);
          reservation_time = millis();
        } else {
          set_display_state(state);
        }
      }

      last_free_time = millis();
    }

    // reset last acc data check time
    last_acc_time = millis();
  }
}

// ---------- Helper Functions Definitions ----------

// --- machine data definitions ---
void init_machine_data() {
  building = display_menu(buildings, building_count);
  machine_id = display_menu(machine_ids, machine_ids_count);
}

String display_menu(const String options[], int option_count) {
  M5.Lcd.clear(BLACK);
  const int arrow_area_width = 70;

  unsigned long last_touch_time = 0;
  int option_index = 0;
  display_menu_option(options[option_index]);
  while (true) {
    TouchPoint_t touch = M5.Touch.getPressPoint();
    if (touch.x != -1 && millis() - last_touch_time > touch_delay) {
      last_touch_time = millis();

      if (touch.x < arrow_area_width) {
        option_index = (option_index + option_count - 1) % option_count;
      } else if (touch.x > (M5.Lcd.width() - arrow_area_width)) {
        option_index = (option_index + 1) % option_count;
      } else {
        return options[option_index];
      }

      display_menu_option(options[option_index]);
    }
  }
}

void display_menu_option(String option) {
  M5.Lcd.clear(BLACK);

  // draw option
  int text_width = option.length() * 34;
  int text_height = 40;
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(6);
  M5.Lcd.drawString(
    option.c_str(),
    M5.Lcd.width() / 2 - text_width / 2,
    M5.Lcd.height() / 2 - text_height / 2
  );

  // draw left arrow
  M5.Lcd.drawTriangle(
    30, int(M5.Lcd.height() / 2),
    50, int(M5.Lcd.height() / 2) - 10,
    50, int(M5.Lcd.height() / 2) + 10,
    WHITE
  );

  // draw right arrow
  M5.Lcd.drawTriangle(
    M5.Lcd.width() - 30, int(M5.Lcd.height() / 2),
    M5.Lcd.width() - 50, int(M5.Lcd.height() / 2) - 10,
    M5.Lcd.width() - 50, int(M5.Lcd.height() / 2) + 10,
    WHITE
  );
}

void set_display_state(String state, String message) {
  int colour;
  if (state == "FREE") {
    colour = GREEN;
  } else if (state == "USED") {
    colour = RED;
  } else if (state == "RESERVED") {
    colour = BLUE;
  } else exit(0);
  M5.Lcd.clear(colour);

  // draw state
  int text_width = state.length() * 35;
  int text_height = 40;
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(6);
  M5.Lcd.drawString(
    state.c_str(),
    M5.Lcd.width() / 2 - text_width / 2,
    M5.Lcd.height() / 2 - text_height / 2
  );

  // draw message
  text_width = message.length() * 11;
  text_height = 12;
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawString(
    message.c_str(),
    M5.Lcd.width() / 2 - text_width / 2,
    int(M5.Lcd.height() * 0.75) - text_height / 2
  );
}

// --- wifi definitions ---
String send_state(String state) {
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connection Error!");
  }

  HTTPClient http;
  String request_url = get_url_with_state(state);
  String payload = get_request(request_url);

  Serial.print("Payload: ");
  Serial.println(payload.c_str());

  return payload;
}

String get_url_with_state(String state) {
  return "http://" + server_ip + ":" + server_port + "/MONITOR?building=" + building + "&machine_id=" + machine_id + "&state=" + state;
}

String get_request(String url) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(url.c_str());

  // Send HTTP POST request
  int http_response_code = http.GET();

  String payload = "{}";

  if (http_response_code > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(http_response_code);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(http_response_code);
  }

  // Free resources
  http.end();

  return payload;
}
