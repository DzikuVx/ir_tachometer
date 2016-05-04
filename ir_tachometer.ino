#include <VirtualWire.h>

#define PIN_LED 9
#define PIN_RECEIVER A10
#define PIN_IR_EMITTER 7
#define THRESHOLD 100
#define OFF_TIME 200
#define ON_TIME 200
#define LOCK_TIME 500

#define MIN_RPM 100
#define MAX_RPM 10000
#define BLADES 2

void setup() {

  pinMode(PIN_IR_EMITTER, OUTPUT);
  digitalWrite(PIN_IR_EMITTER, HIGH);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RECEIVER, INPUT);
  Serial.begin(115200);
}

byte state = LOW;
long lockMillis = 0;

int prevVal = 0;

uint32_t prevRisingMillis = 0;
uint32_t risingMillis = 0;

uint32_t cycle = 0;

int smooth(int data, float filterVal, float smoothedVal){

  if (filterVal > 1){      // check to make sure params are within range
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }

  smoothedVal = (data * (1 - filterVal)) + (smoothedVal  *  filterVal);

  return (int)smoothedVal;
}

int freq;
int freqFiltered;
uint32_t rpm;
uint32_t rpmFiltered;

void loop() {

  int val = analogRead(PIN_RECEIVER);
//  Serial.println(val);

  if (lockMillis != 0 && lockMillis + LOCK_TIME < millis()) {
    state = LOW;
    digitalWrite(PIN_LED, state);
    lockMillis = 0;
  }

  if (val > THRESHOLD && lockMillis == 0) {
    state = HIGH;
    digitalWrite(PIN_LED, state);
    lockMillis = millis();
  }

  if (val > THRESHOLD && prevVal < THRESHOLD) {
    risingMillis = micros();

    uint32_t dst = risingMillis - prevRisingMillis;

    rpm = (60000000 / dst) / BLADES;

    rpmFiltered = smooth(rpm, 0.99, rpmFiltered);

//    if (rpm > MIN_RPM && rpm < MAX_RPM) {
      Serial.println(rpmFiltered);  
//    }
    
    prevRisingMillis = risingMillis;
  }

//  if (cycle % 1000 == 0) {
//    Serial.println(dst);
//  }

  prevVal = val;
  cycle++;

//  Serial.println(val);


}
