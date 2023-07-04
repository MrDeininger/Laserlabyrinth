#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24Network.h>
#include <FastLED.h>

#define SERIAL_DEBUG
#define CE_PIN  9
#define CSN_PIN 10
#define LED_PIN 2
#define NUM_LEDS 5
#define PHOTO_RESISTOR_PIN A0
#define NETWORK_ADDRESS 2
#define SERVER_ADDRESS 0


int THRESHOLD = 120;
CRGB leds[NUM_LEDS];
RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);
unsigned long StateTimer = 0;

// Status 0: not connected, 1: connected, 2: laserschranke triggered
int state = 0;
void checkLaserschranke();
void updateLEDs();
void sendState();
void updateThreshold();
void processNetworkRequests();


void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  radio.begin();
  network.begin(90, NETWORK_ADDRESS);
  updateLEDs();
  Serial.begin(115200);
  Serial.println("Started");
}

void loop() {
  network.update();
  processNetworkRequests();
  checkLaserschranke();
  updateLEDs();
  sendState();
}


void checkLaserschranke() {
  if (analogRead(PHOTO_RESISTOR_PIN) > THRESHOLD) {
    state = 2;
    StateTimer = millis() + 3000; // Set state to laserschranke triggered for 3 seconds
    Serial.print("Laserschranke triggered: ");
    Serial.println(analogRead(PHOTO_RESISTOR_PIN));
    
  }
}

void updateLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    switch (state) {
      case 0:
        leds[i] = CRGB::Red;
        break;
      case 1:
        leds[i] = CRGB::Blue;
        break;
      case 2:
        leds[i] = CRGB::Green;
        break;
    }
    FastLED.show();
  }
}

void sendState() {
    unsigned long currentMillis = millis();
    RF24NetworkHeader header(SERVER_ADDRESS);
    bool isSent = network.write(header, &state, sizeof(state));
    if (!isSent) {
        state = currentMillis > StateTimer ? 0 : 2; // Set state to not connected if the message was not sent 
    }
    else {
        state = currentMillis > StateTimer ? 1 : 2; // Set state to connected if the message was sent
    }
}

void updateThreshold() {
  uint16_t buffer;
  Serial.println("Updating threshold");

  for(int i = 0; i<100; i++){
    buffer += analogRead(PHOTO_RESISTOR_PIN);
  }
  buffer /= 100;
  Serial.println(buffer + 25);
  THRESHOLD = (int)buffer + 25;
}

void processNetworkRequests() {
  while (network.available()) {
    RF24NetworkHeader header;
    network.peek(header);
    if (header.type == 'T') {
      network.read(header, NULL, 0);
      updateThreshold();

      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Blue;
        FastLED.show();
        }
      delay(250);

      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
        FastLED.show();
        }
      delay(250);

      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Blue;
        FastLED.show();
        }
      delay(250);

      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
        FastLED.show();
        }
      delay(250);
    }
  }
}