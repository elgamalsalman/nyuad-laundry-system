#include <WiFi.h>
#include <HTTPClient.h>
// #include <Arduino_JSON.h>

const char* ssid = "Salman's iPhone";
const char* password = "anything";
const String server_ip = "172.20.10.2";

const String building = "a1c";
const String machine_type = "d";
const int machine_number = 12;

String jsonBuffer;

String get_query_url(String);
String httpGETRequest(const char*);

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Send an HTTP GET request
  if (WiFi.status() == WL_CONNECTED) {
    String serverPath = get_query_url("on");

    jsonBuffer = httpGETRequest(serverPath.c_str());
    Serial.println(jsonBuffer);
    // JSONVar myObject = JSON.parse(jsonBuffer);

    // JSON.typeof(jsonVar) can be used to get the type of the var
    // if (JSON.typeof(myObject) == "undefined") {
    //   Serial.println("Parsing input failed!");
    //   return;
    // }

    // Serial.print("JSON object = ");
    // Serial.println(myObject);
    // Serial.print("Temperature: ");
    // Serial.println(myObject["main"]["temp"]);
    // Serial.print("Pressure: ");
    // Serial.println(myObject["main"]["pressure"]);
    // Serial.print("Humidity: ");
    // Serial.println(myObject["main"]["humidity"]);
    // Serial.print("Wind Speed: ");
    // Serial.println(myObject["wind"]["speed"]);
  } else {
    Serial.println("WiFi Disconnected");
  }
}

// ----- HELPER FUNCTIONS -----

String get_query_url(String state) {
  return "http://" + server_ip + "/LOG?building=" + building + "&type=" + machine_type + "&number=" + machine_number + "&state=" + state;
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
