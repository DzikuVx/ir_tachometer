#include <Adafruit_SSD1306.h>

//#define SSD1306_128_64

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define PIN_LED 6
#define PIN_RECEIVER A1
#define PIN_IR_EMITTER 7
#define PIN_BLADE_BUTTON 9
#define PIN_RESET_BUTTON 8
#define THRESHOLD 90
#define OFF_TIME 200
#define ON_TIME 200
#define LOCK_TIME 500

#define BLADES 2
#define BLADES_MIN 2
#define BLADES_MAX 4

#define FREQ_MIN 6
#define FREQ_MAX 900

#define CATCH_DELAY (1000000/FREQ_MAX)/2

#define MICROS_THRESHOLD 100

#define SCREEN_UPDATE_THRESHOLD_MILIS 500

uint8_t blades = BLADES;

void setup() {

  pinMode(PIN_IR_EMITTER, OUTPUT);
  digitalWrite(PIN_IR_EMITTER, HIGH);

  pinMode(PIN_BLADE_BUTTON, INPUT_PULLUP);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RECEIVER, INPUT);
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.display();
}

byte state = LOW;
long lockMillis = 0;

int prevVal = 0;

uint32_t prevRisingMillis = 0;
uint32_t risingMillis = 0;

uint32_t cycle = 0;

int smooth(int data, float filterVal, float smoothedVal){

  if (filterVal > 1){
    filterVal = .99;
  }
  else if (filterVal <= 0){
    filterVal = 0;
  }

  smoothedVal = (data * (1 - filterVal)) + (smoothedVal  *  filterVal);

  return (int)smoothedVal;
}

uint32_t freq;
uint32_t freqFiltered;
uint32_t rpm;

int val;

uint8_t buttonBladesPrevState = HIGH;
uint8_t buttonBladesState = HIGH;

uint32_t lastScreenUpdate = 0;


void loop() {

  buttonBladesState = digitalRead(PIN_BLADE_BUTTON);
  Serial.println(buttonBladesState);
  if (buttonBladesState == HIGH && buttonBladesPrevState == LOW) {
    blades++;
    if (blades > BLADES_MAX) {
      blades = BLADES_MIN;
    }
  }

  buttonBladesPrevState = buttonBladesState;

  uint32_t currentMillis = millis();

  if (currentMillis > lastScreenUpdate + SCREEN_UPDATE_THRESHOLD_MILIS) {

    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("Blades: ");
    display.println(blades);
    display.println("");

    display.setTextSize(2);
    display.print("RPM: ");
    display.println(rpm);

    display.print("Freq: ");
    display.println(freqFiltered);

    display.display();

    lastScreenUpdate = currentMillis;
  }

  val = analogRead(PIN_RECEIVER);

  Serial.println(val);

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

    freq = 1000000 / dst;

    /*
     * We measure only frequencies that are within range
     */
    if (freq > FREQ_MIN && freq < FREQ_MAX) {
      freqFiltered = smooth(freq, 0.95, freqFiltered);
      rpm = (freqFiltered * 60) / blades;
    }

    prevRisingMillis = risingMillis;
    delayMicroseconds(CATCH_DELAY);
  }

  prevVal = val;
  cycle++;
}
