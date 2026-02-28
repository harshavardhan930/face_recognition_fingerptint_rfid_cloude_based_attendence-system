#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>  // Include the ESP8266 WebServer library
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Pin Definitions for RFID
#define RST_PIN  D3
#define SS_PIN   D4

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

// Fingerprint sensor connections
SoftwareSerial mySerial(5, 4);  // RX = D5, TX = D4 for SoftwareSerial
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// WiFi Credentials
const char* ssid = "ybr";          // Replace with your WiFi name
const char* password = "harsha555";  // Replace with your WiFi password

// Google Sheets Web App URL
const char* webAppURL = "https://script.google.com/macros/s/AKfycbw-C-f70QvaBAb3EOf_-W1V7i4PhVMEfbr0UWb0nd95MYTVFZ2ss4LGBS8PGCPvsI4RyA/exec";  // Replace with your Web App URL

// Define UIDs for each RFID card
byte uid1[] = {0xF3, 0x73, 0x5D, 0xFB};  // UID for "Harsha"
byte uid2[] = {0x53, 0x59, 0x5F, 0x1A};  // UID for "Hema"
byte uid3[] = {0x4, 0xA8, 0x8B, 0x3F};  // UID for "John"
byte uid4[] = {0x9A, 0xBC, 0xDE, 0xF0};  // UID for "Sara"
byte uid5[] = {0x11, 0x22, 0x33, 0x44};  // UID for "Mike"

// Create an instance of the server
ESP8266WebServer server(80);  // Web server on port 80

void sendToGoogleSheets(String name, String type); // Declare function here
void handleRequest();  // Declare the request handler function

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize RFID reader
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Place your RFID card on the reader...");

  // Initialize Fingerprint sensor
  initializeFingerprintSensor();

  // Connect to WiFi
  connectToWiFi();


  // Start the web server and define the routes
  server.on("/update", HTTP_GET, handleRequest);
  server.begin();
}

// Function to initialize the fingerprint sensor
void initializeFingerprintSensor() {
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) {
    //Serial.println("Fingerprint sensor found!");
  } else {
    Serial.println("FP sensor not found.");
    while (1) { delay(1); } // Stop execution
  }

  finger.getTemplateCount();
  if (finger.templateCount == 0) {
    Serial.println("no fingerprint");
  }
}

// Function to connect to WiFi
void connectToWiFi() {
  Serial.print(" WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected!");
  Serial.println(WiFi.localIP());
}

// Function to handle incoming requests
void handleRequest() {
  if (server.hasArg("name") && server.hasArg("type")) {
    String name = server.arg("name");
    String type = server.arg("type");

   // Serial.print("Received data: ");
    Serial.print(name);
    Serial.println(type);

    server.send(200, "text/plain", "Data received successfully");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

// Function to compare two UIDs for RFID
bool compareUID(byte* uid1, byte* uid2) {
  for (byte i = 0; i < 4; i++) {
    if (uid1[i] != uid2[i]) {
      return false;
    }
  }
  return true;
}

// Function to get fingerprint ID and return the corresponding name
String getFingerprintName() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK: break;
    case FINGERPRINT_NOFINGER: return "";  // No finger detected
    case FINGERPRINT_PACKETRECIEVEERR: return "Error";
    default:  return "Error";
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    return "Error";
  }

  p = finger.fingerSearch();
  if (p != FINGERPRINT_OK) {
    return "";  // No match found
  }

  int id = finger.fingerID;
  switch (id) {
    case 1: return "Harsha";
    case 2: return "Hema";
    case 3: return "Rahul";
    case 4: return "Priya";
    case 5: return "Arjun";
    case 6: return "Ravi";
    case 7: return "Anusha";
    case 8: return "Srikanth";
    case 9: return "Suresh";
    case 10: return "Megha";
    default: return "Unknown ID";
  }
}

void loop() {

  // Check for RFID card
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      String rfidName = "Unknown Card";
      String rfidType = "RFID";

      // Check if the UID matches any predefined UID and print the corresponding name
      if (compareUID(mfrc522.uid.uidByte, uid1)) {
        rfidName = "Harsha";
      } else if (compareUID(mfrc522.uid.uidByte, uid2)) {
        rfidName = "Hema";
      } else if (compareUID(mfrc522.uid.uidByte, uid3)) {
        rfidName = "John";
      } else if (compareUID(mfrc522.uid.uidByte, uid4)) {
        rfidName = "Sara";
      } else if (compareUID(mfrc522.uid.uidByte, uid5)) {
        rfidName = "Mike";
      }

      // Send RFID data to Google Sheets
      sendToGoogleSheets(rfidName, rfidType);
      mfrc522.PICC_HaltA(); // Halt the card
    }
  }

  // Check for fingerprint
  String fingerprintName = getFingerprintName();
  if (fingerprintName != "" && fingerprintName != "Error") {
    String fingerprintType = "Fingerprint";
    sendToGoogleSheets(fingerprintName, fingerprintType);
    delay(2000);  // Add a small delay to avoid duplicate uploads
  }

  server.handleClient();  // Handle incoming client requests
}

// Function to send data to Google Sheets
void sendToGoogleSheets(String name, String type) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();  // Use for HTTPS without certificate validation

    HTTPClient http;

    // Prepare the full URL with parameters
    String url = String(webAppURL) + "?name=" + name + "&type=" + type;
   // Serial.print("Sending data to: ");
    Serial.println(url);

    http.begin(client, url);  // Start HTTP connection
    int httpResponseCode = http.GET(); // Send HTTP GET request

    if (httpResponseCode > 0) {
      String response = http.getString();
     // Serial.println("Response: " + response);
    } else {
     // Serial.print("Error in HTTP request: ");
     //Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi!");
  }
}
