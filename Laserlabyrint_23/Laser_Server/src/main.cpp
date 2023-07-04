#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24Network.h>
#include <FastLED.h>
#include <Servo.h>


#define CE_PIN  9
#define CSN_PIN 10
#define LED_PIN 2
#define NUM_LEDS 5
#define NETWORK_ADDRESS 0
#define SERVO_PIN 4
#define TRIGGER_PIN 6

Servo servo;
unsigned long lastReceived[NUM_LEDS];
int states[NUM_LEDS] = {0};
CRGB leds[NUM_LEDS];
RF24 radio(CE_PIN, CSN_PIN);
RF24Network network(radio);

void checkConnectionTimeout();
void updateLEDs();
void sendThresholdUpdate();
void ServoSetup();
void ServoHandler();

void setup() {
  pinMode(TRIGGER_PIN, OUTPUT);

  ServoSetup();
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  radio.begin();
  network.begin(90, NETWORK_ADDRESS);
  for (int i = 0; i < NUM_LEDS; ++i) {
    lastReceived[i] = millis();
  }
  updateLEDs();

  Serial.begin(115200);
  Serial.println("Started");
}

void loop() {
  network.update();
  while (network.available()) {
    RF24NetworkHeader header;
    int incomingState;
    network.read(header, &incomingState, sizeof(incomingState));
    states[header.from_node - 1] = incomingState;
    lastReceived[header.from_node - 1] = millis();
  }
  checkConnectionTimeout();
  updateLEDs();
  sendThresholdUpdate();
  ServoHandler();
}

void checkConnectionTimeout() {
  unsigned long now = millis();
  for (int i = 0; i < NUM_LEDS; ++i) {
    if (now - lastReceived[i] > 1000) {
      states[i] = 0;
    }
  }
}

void updateLEDs() {
  for (int i = 0; i < NUM_LEDS; i++) {
    switch (states[i]) {
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
  }
  FastLED.show();
}

void sendThresholdUpdate() 
{
  if(analogRead(A0) < 100)
  {
    for(int i = 0; i < NUM_LEDS; i++) 
    {
      RF24NetworkHeader header(i+1, 'T');
      network.write(header, NULL, 0);
    }
    Serial.println("Sent threshold updates");
  }
}

void ServoSetup(){
  servo.attach(SERVO_PIN);
  servo.write(0);
}

void ServoHandler(){
  bool isonetriggered = false;
  for(int i = 0; i < NUM_LEDS; i++) {
    if(states[i] == 2)
    {
      isonetriggered = true;
    }
  }
  if(isonetriggered)
  {
    servo.write(180);
    Serial.println("Servo is triggered");
    digitalWrite(TRIGGER_PIN, HIGH);
  }
  else
  {
    servo.write(0);
    digitalWrite(TRIGGER_PIN, LOW);
  }

}