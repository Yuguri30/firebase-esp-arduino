
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include <Wire.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Pandesal"
#define WIFI_PASSWORD "three5eight"

#define Tx2 16
#define Rx2 17

// Insert Firebase project API Key
#define API_KEY "AIzaSyASTqGTe5EOttmde0UqzqJ-k4jrlG7ZphQ"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "louiebelleza30@gmail.com"
#define USER_PASSWORD "My_g00gle_@cc_passw0rd30"

// Insert RTDB URL
#define DATABASE_URL "https://hydroponics-c9f41-default-rtdb.asia-southeast1.firebasedatabase.app"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Variables to save database paths
String databasePath, phPath, celciusPath, farenheitPath, waterlevelPath, humidityPath;
// variable name for the serial read
float dhtTemp, ph, celcius, farenheit, waterlevel, humidity;

// Timer variables (send new readings every 1 minutes)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 5000;
bool signupOK = false;


// define LED pins
#define LedPinA 33
#define LedPinB 25
bool ledStateA = false;
bool ledStateB = false;



// define waterpump pins

#define ENA 12
#define PumpA 14
#define PumpB 27
#define ENB 26

#define ENC 15
#define PumpC 2
#define PumpD 0
#define END 4

bool PumpStateA = false;
bool PumpStateB = false;
bool PumpStateC = false;
bool PumpStateD = false;

// define waterpump speed



// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Write float values to the database
void sendFloat(String path, float value) {
  if (Firebase.RTDB.setFloat(&fbdo, path.c_str(), value)) {
    Serial.print("Writing value: ");
    Serial.print(value);
    Serial.print(" on the following path: ");
    Serial.println(path);
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  } else {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

void setup() {
  // for LEDPIN
  pinMode(LedPinA, OUTPUT);
  pinMode(LedPinB, OUTPUT);

  // for waterpump pin
  pinMode(PumpA, OUTPUT);
  pinMode(PumpB, OUTPUT);
  pinMode(PumpC, OUTPUT);
  pinMode(PumpD, OUTPUT);

  //(Optional for waterpump)
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(ENC, OUTPUT);
  pinMode(END, OUTPUT);

  Serial2.begin(115200, SERIAL_8N1, 16, 17);  // Initialize UART2 at  9600 bps, Rx and Tx
  Serial.begin(115200);

  // Initialize Wifi conn
  initWiFi();

  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback;  //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  //Initialize the library with the Firebase authen and config anonymous method
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("signup OK");
    bool signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid;

  // Update database path for sensor readings
  phPath = databasePath + "/ph";
  celciusPath = databasePath + "/celcius";
  farenheitPath = databasePath + "/farenheit";
  waterlevelPath = databasePath + "/waterlevel";
  humidityPath = databasePath + "/humidity";
}


void loop() {
  if (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');

    int phIndex = data.indexOf("ph:");
    int celciusIndex = data.indexOf("celsius:");
    int fahrenheitIndex = data.indexOf("fahrenheit:");
    int waterlevelIndex = data.indexOf("waterlevel:");
    int humidityIndex = data.indexOf("humidity:");

    if (phIndex >= 0 && celciusIndex >= 0 && fahrenheitIndex >= 0 && waterlevelIndex >= 0 && humidityIndex >= 0) {

      int celciusEndIndex = data.indexOf(",", celciusIndex);
      int fahrenheitEndIndex = data.indexOf(",", fahrenheitIndex);
      int humidityEndIndex = data.indexOf(",", humidityIndex);  // Correctly find the end index of the humidity value

      String celciusSubstr = data.substring(celciusIndex + 8, celciusEndIndex);
      String fahrenheitSubstr = data.substring(fahrenheitIndex + 11, fahrenheitEndIndex);
      String humiditySubstr = data.substring(humidityIndex + 9, humidityEndIndex);  // Correctly extract the humidity substring

      ph = data.substring(phIndex + 3).toFloat();
      waterlevel = data.substring(waterlevelIndex + 3).toFloat();
      humidity = humiditySubstr.toFloat();  // Correctly convert the humidity substring to float

      celcius = celciusSubstr.toFloat();
      farenheit = fahrenheitSubstr.toFloat();

      // Send data to Firebase
      sendFloat(phPath, ph);
      sendFloat(celciusPath, celcius);
      sendFloat(farenheitPath, farenheit);
      sendFloat(waterlevelPath, waterlevel);
      sendFloat(humidityPath, humidity);  // Correctly send the humidity value

      Serial.print("pHValue: ");
      Serial.println(ph);

      Serial.print("Celsius: ");
      Serial.println(celcius);

      Serial.print("Fahrenheit: ");
      Serial.println(farenheit);

      Serial.print("Water Level: ");
      Serial.println(waterlevel);

      Serial.print("Humidity: ");
      Serial.println(humidity);
    }
  }

  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    sendFloat(phPath, ph);
    sendFloat(celciusPath, celcius);
    sendFloat(farenheitPath, farenheit);
    sendFloat(waterlevelPath, waterlevel);
    sendFloat(humidityPath, humidity);  // Correctly send the humidity value

    // LEDA
    if (Firebase.RTDB.getBool(&fbdo, "/LED/LEDA")) {
      if (fbdo.dataType() == "boolean") {
        ledStateA = fbdo.boolData();
        Serial.println("Successful READ from " + fbdo.dataPath() + ":" + ledStateA + "(" + fbdo.dataType() + ")");
        digitalWrite(LedPinA, ledStateA);
      } else {
        Serial.println("Failed : " + fbdo.errorReason());
        Serial.println("Failed to read from /LED/LEDA");
      }
    } else {
      Serial.println("Firebase is not ready or signup is not OK.");
    }

    // LEDB
    if (Firebase.RTDB.getBool(&fbdo, "/LED/LEDB")) {
      if (fbdo.dataType() == "boolean") {
        ledStateB = fbdo.boolData();
        Serial.println("Successful READ from " + fbdo.dataPath() + ":" + ledStateB + "(" + fbdo.dataType() + ")");
        digitalWrite(LedPinB, ledStateB);
      } else {
        Serial.println("Failed : " + fbdo.errorReason());
        Serial.println("Failed to read from /LED/LEDB");
      }
    } else {
      Serial.println("Firebase is not ready or signup is not OK.");
    }

    // PumpA
    if (Firebase.RTDB.getBool(&fbdo, "/Pump/PumpA")) {
      if (fbdo.dataType() == "boolean") {
        PumpStateA = fbdo.boolData();
        Serial.println("Successful READ from " + fbdo.dataPath() + ":" + PumpStateA + "(" + fbdo.dataType() + ")");
        analogWrite(ENA, 255);  //ENA pin
        digitalWrite(PumpA, PumpStateA);
      } else {
        Serial.println("Failed : " + fbdo.errorReason());
        Serial.println("Failed to read from /Pump/PumpA");
      }
    } else {
      Serial.println("Firebase is not ready or signup is not OK.");
    }

    // PumpB
    if (Firebase.RTDB.getBool(&fbdo, "/Pump/PumpB")) {
      if (fbdo.dataType() == "boolean") {
        PumpStateB = fbdo.boolData();
        Serial.println("Successful READ from " + fbdo.dataPath() + ":" + PumpStateB + "(" + fbdo.dataType() + ")");
        analogWrite(ENB, 255);  //ENB pin
        digitalWrite(PumpB, PumpStateB);
      } else {
        Serial.println("Failed : " + fbdo.errorReason());
        Serial.println("Failed to read from /Pump/PumpB");
      }
    } else {
      Serial.println("Firebase is not ready or signup is not OK.");
    }

    // PumpC
    if (Firebase.RTDB.getBool(&fbdo, "/Pump/PumpC")) {
      if (fbdo.dataType() == "boolean") {
        PumpStateC = fbdo.boolData();
        Serial.println("Successful READ from " + fbdo.dataPath() + ":" + PumpStateC + "(" + fbdo.dataType() + ")");
        analogWrite(ENC, 255);  //ENC pin
        digitalWrite(PumpC, PumpStateC);
      } else {
        Serial.println("Failed : " + fbdo.errorReason());
        Serial.println("Failed to read from /Pump/PumpC");
      }
    } else {
      Serial.println("Firebase is not ready or signup is not OK.");
    }

    // PumpD
    if (Firebase.RTDB.getBool(&fbdo, "/Pump/PumpD")) {
      if (fbdo.dataType() == "boolean") {
        PumpStateD = fbdo.boolData();
        Serial.println("Successful READ from " + fbdo.dataPath() + ":" + PumpStateD + "(" + fbdo.dataType() + ")");
        analogWrite(END, 255);  //END pin
        digitalWrite(PumpD, PumpStateD);
      } else {
        Serial.println("Failed : " + fbdo.errorReason());
        Serial.println("Failed to read from /Pump/PumpD");
      }
    } else {
      Serial.println("Firebase is not ready or signup is not OK.");
    }
  }
}


// Placeholder for the tokenStatusCallback function
// Implement the actual logic based on your requirements.
void tokenStatusCallback(FirebaseData &fbdo, const char *message) {
  Serial.println(message);
}
