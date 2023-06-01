#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    868E6

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);

float decreaseThreshold = 5.0;  // Azalma eşik değeri (örneğin 5 metre)
float increaseThreshold = 5.0;  // Artış eşik değeri (örneğin 5 metre)
float previousDistance = 0.0;

void setupGPS() {
  GPSSerial.begin(9600, SERIAL_8N1, 34, 12);
}

String readData() {
  String packet;

  while (LoRa.available()) {
    packet = LoRa.readString();
  }

  return packet;
}

void displayDistance(float distance) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);

  if (distance >= increaseThreshold) {
    display.println("Distance increased!   Stand Your Position.");
  } else if (distance <= -decreaseThreshold) {
    display.println("Distance decreased!   Pay attention!");
  } else {
    display.println("Good Driving,   Drive Safely :)");
  }

  display.display();
}

void displaySerialDistance(float distance) {
  if (distance >= increaseThreshold) {
    Serial.println("Distance increased! Drive safely.");
  } else if (distance <= -decreaseThreshold) {
    Serial.println("Distance decreased! Pay attention.");
  } else {
    Serial.println("Distance is stable. Drive carefully.");
  }
}

float calculateDistance(float lat1, float lon1, float lat2, float lon2) {
  // Haversine formülü ile mesafe hesaplama
  float r = 6371000; // Dünya yarıçapı (metre cinsinden)
  float dLat = radians(lat2 - lat1);
  float dLon = radians(lon2 - lon1);
  float a = sin(dLat / 2) * sin(dLat / 2) + cos(radians(lat1)) * cos(radians(lat2)) * sin(dLon / 2) * sin(dLon / 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  float distance = r * c;

  return distance;
}

void displayCoordinates(float transmitterLat, float transmitterLon, float receiverLat, float receiverLon, float distance) {
  Serial.println("GPS Geographic Coordinates");
  Serial.println("----Transmitter Latitude: " + String(transmitterLat) + " m");
  Serial.println("----Transmitter Longitude: " + String(transmitterLon) + " m");
  Serial.println("----Receiver Latitude: " + String(receiverLat) + " m");
  Serial.println("----Receiver Longitude: " + String(receiverLon) + " m");
  Serial.println("----Distance: " + String(distance) + " m");
  Serial.println("******************************");
}

void processGPSData() {
  if (gps.location.isValid()) {
    float latitude = gps.location.lat();
    float longitude = gps.location.lng();

    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      String packet = readData();
      String receivedLatitude = packet.substring(packet.indexOf("LAT:") + 4, packet.indexOf(","));
      String receivedLongitude = packet.substring(packet.indexOf("LON:") + 4);

      // Convert received latitude and longitude to float with 6 decimal places
      float receivedLat = receivedLatitude.toFloat();
      float receivedLon = receivedLongitude.toFloat();

      // Calculate distance
      float distance = calculateDistance(latitude, longitude, receivedLat, receivedLon);

      // Check if distance has decreased or increased significantly
      if (previousDistance > 0.0) {
        float difference = previousDistance - distance;
        displaySerialDistance(difference);
        displayDistance(difference);
      }

      // Update previous distance value
      previousDistance = distance;

      // Display coordinates and distance
      displayCoordinates(latitude, longitude, receivedLat, receivedLon, distance);
    }
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Wire.begin();
  Wire.setClock(400000);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  LoRa.setPins(SS, RST, DI0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  setupGPS();
}

void loop() {
  while (GPSSerial.available() > 0) {
    gps.encode(GPSSerial.read());
  }

  processGPSData();
  delay(1000);
}
