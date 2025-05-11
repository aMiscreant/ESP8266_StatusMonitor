#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <Base64.h>
#include "SHA256.h"

// OLED setup
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 14 // D6
#define OLED_SCL 12 // D5
#define SCREEN_ADDRESS 0x3C

// WiFi credentials
const char* ssid = "TL;DR";
const char* password = "Miscreant1991";

// Server info
const char* host = "192.168.66.183";
const int httpsPort = 443;
const char* endpoint = "/status";
const int MAX_B64 = 1024;
byte decoded[MAX_B64];

// Shared 32-byte secret key (must match server)
const byte shared_key[32] = {
  0x1f, 0x1d, 0x9d, 0x2b, 0x8d, 0xc2, 0x19, 0x6d,
  0x87, 0xa7, 0x58, 0x1b, 0xf9, 0xad, 0x3c, 0x6d,
  0x4b, 0x8f, 0x1f, 0xdd, 0x68, 0xe1, 0x0f, 0x70,
  0x84, 0x3d, 0xaf, 0xd7, 0x0c, 0xf9, 0xa9, 0x2d
};

WiFiClientSecure client;
Adafruit_SSD1306* display;

// Display handler
void display_status(int sys, int ssh, int user, int tor) {
  display->clearDisplay();
  display->setTextSize(1);
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);

  display->println("Status Monitor");
  display->print("System: "); display->println(sys ? "Enabled" : "Disabled");
  display->print("SSH: ");    display->println(ssh ? "Enabled" : "Disabled");
  display->print("Users: ");  display->println(user > 0 ? String(user) : "None");
  display->print("Tor: ");    display->println(tor ? "Enabled" : "Disabled");

  display->display();
}

void HMAC_SHA256(byte* message, size_t messageLen, const byte* key, size_t keyLen, byte* result) {
  byte opad[64]; // Outer padding
  byte ipad[64]; // Inner padding
  byte key_block[64]; // Key block for HMAC

  // If the key is longer than block size, hash it first
  if (keyLen > 64) {
    SHA256 sha;
    sha.update(key, keyLen);
    byte key_hash[32];  // Store the result of the key hash
    sha.finalize(key_hash, 32);  // Finalize with size explicitly set
    memcpy(key_block, key_hash, 32);  // Use the hash as the key
    keyLen = 32;  // Now key is 32 bytes long
  } else {
    // Otherwise, just copy the key into the block
    memset(key_block, 0, 64);
    memcpy(key_block, key, keyLen);
  }

  // Prepare inner and outer pads
  for (int i = 0; i < 64; i++) {
    ipad[i] = key_block[i] ^ 0x36; // Inner pad = key XOR 0x36
    opad[i] = key_block[i] ^ 0x5c; // Outer pad = key XOR 0x5c
  }

  // Inner hash: SHA256(ipad || message)
  SHA256 sha_inner;
  sha_inner.update(ipad, 64);  // Update with inner pad
  sha_inner.update(message, messageLen);  // Update with message
  byte inner_hash[32];  // Store inner hash here
  sha_inner.finalize(inner_hash, 32);  // Finalize with size explicitly set

  // Outer hash: SHA256(opad || inner_hash)
  SHA256 sha_outer;
  sha_outer.update(opad, 64);  // Update with outer pad
  sha_outer.update(inner_hash, 32);  // Update with inner hash
  sha_outer.finalize(result, 32);  // Finalize with size explicitly set
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(OLED_SDA, OLED_SCL);
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  if (!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED init failed");
    while (true);
  }

  display->clearDisplay();
  display->setCursor(0, 0);
  display->println("Connecting WiFi...");
  display->display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  display->clearDisplay();
  display->setCursor(0, 0);
  display->println("WiFi Connected");
  display->display();
  delay(1000);

  client.setInsecure(); // Insecure SSL (for self-signed or dev servers)
}

void loop() {
  if (!client.connect(host, httpsPort)) {
    Serial.println("❌ Connection to server failed");
    return;
  }

  client.print(String("GET ") + endpoint + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  // Read full HTTP response
  String response;
  while (client.connected() || client.available()) {
    String line = client.readStringUntil('\n');
    response += line + "\n";
  }

  Serial.println("📥 Full HTTP Response:");
  Serial.println(response);

  // Extract base64 payload from body
  int start = response.indexOf("\r\n\r\n");
  if (start < 0) {
    Serial.println("❌ Failed to locate HTTP body");
    return;
  }

  String b64 = response.substring(start + 4);
  b64.trim();
  Serial.print("📦 Base64 payload: ");
  Serial.println(b64);

  // Base64 decode
  int decodedLen = Base64.decodedLength((char*)b64.c_str(), b64.length());
  if (decodedLen > MAX_B64) {
    Serial.println("❌ Decoded length too large");
    return;
  }

  int decodedBytes = Base64.decode((char*)decoded, (char*)b64.c_str(), b64.length());
  if (decodedBytes != decodedLen) {
    Serial.print("❌ Base64 decode mismatch: got ");
    Serial.print(decodedBytes);
    Serial.print(" expected ");
    Serial.println(decodedLen);
    return;
  }

  Serial.println("✅ Base64 decoded successfully");
  Serial.print("🔢 Decoded length: ");
  Serial.println(decodedLen);

  if (decodedLen <= 32) {
    Serial.println("❌ Payload too short for HMAC-SHA256 tag");
    return;
  }

  // Separate message and HMAC
  int messageLen = decodedLen - 32;
  byte* message = decoded;
  byte* receivedHmac = decoded + messageLen;

  Serial.println("🔐 Verifying HMAC-SHA256...");

  // Manually compute HMAC
  byte computedHmac[32];
  HMAC_SHA256(message, messageLen, shared_key, sizeof(shared_key), computedHmac);

  // Compare HMACs
  bool valid = true;
  for (int i = 0; i < 32; i++) {
    if (computedHmac[i] != receivedHmac[i]) {
      valid = false;
      break;
    }
  }

  if (!valid) {
    Serial.println("❌ HMAC mismatch. Message tampered.");
    display->clearDisplay();
    display->setCursor(0, 0);
    display->println("HMAC invalid");
    display->display();
    return;
  }

  Serial.println("✅ HMAC valid");

  // Parse JSON
  String jsonStr = "";
  for (int i = 0; i < messageLen; i++) {
    jsonStr += (char)message[i];
  }
  Serial.println("📄 JSON string:");
  Serial.println(jsonStr);

  DynamicJsonDocument doc(512);
  DeserializationError err = deserializeJson(doc, jsonStr);
  if (err) {
    Serial.print("❌ JSON parse error: ");
    Serial.println(err.c_str());
    display->clearDisplay();
    display->setCursor(0, 0);
    display->println("JSON error");
    display->display();
    return;
  }

  int sys  = doc["sys"]  | 0;
  int ssh  = doc["ssh"]  | 0;
  int user = doc["user"] | 0;
  int tor  = doc["tor"]  | 0;

  display_status(sys, ssh, user, tor);
  delay(10000);
}
