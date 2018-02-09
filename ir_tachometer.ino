//TODO delay at the end of readout cycle has to be dynamic, but probably it is not needed at all
//TODO Measurement should unlock after there is subsequent number of readouts below threshold, not just one at 0
//TODO Main loop filtering should filter time derivative not frequency

#include <Adafruit_SSD1306.h>

//#define SSD1306_128_64

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define PIN_LED 6
#define PIN_RECEIVER A1
#define PIN_IR_EMITTER 7
#define PIN_BLADE_BUTTON 9
#define PIN_SENS_BUTTON 8
#define THRESHOLD 90
#define OFF_TIME 200
#define ON_TIME 200
#define LOCK_TIME 500

#define BLADES 2
#define BLADES_MIN 1
#define BLADES_MAX 6

#define FREQ_MIN 4
#define FREQ_MAX 900

#define CATCH_DELAY (1000000/FREQ_MAX)/2 // delay after each run

#define MICROS_THRESHOLD 100

#define SCREEN_UPDATE_THRESHOLD_MILIS 500

#define SMOOTH_FACTOR 0.99

uint8_t blades = BLADES;

uint8_t thresholdIndex = 2;
uint16_t thresholds[5] = {50, 100, 150, 200, 250};

void setup()
{

  pinMode(PIN_IR_EMITTER, OUTPUT);
  digitalWrite(PIN_IR_EMITTER, HIGH);

  pinMode(PIN_BLADE_BUTTON, INPUT_PULLUP);
  pinMode(PIN_SENS_BUTTON, INPUT_PULLUP);

  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RECEIVER, INPUT);
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // initialize with the I2C addr 0x3C (for the 128x32)
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

int smooth(int data, float filterVal, float smoothedVal)
{

  if (filterVal > 1)
  {
    filterVal = .99;
  }
  else if (filterVal <= 0)
  {
    filterVal = 0;
  }

  smoothedVal = (data * (1 - filterVal)) + (smoothedVal * filterVal);

  return (int)smoothedVal;
}

uint32_t freq;
uint32_t freqFiltered;
uint32_t rpm;

int val;
int valFiltered;

uint8_t buttonBladesPrevState = HIGH;
uint8_t buttonBladesState = HIGH;

uint8_t buttonSensPrevState = HIGH;
uint8_t buttonSensState = HIGH;

uint32_t lastScreenUpdate = 0;

void loop()
{

  static bool updated = false;

  buttonBladesState = digitalRead(PIN_BLADE_BUTTON);
  if (buttonBladesState == HIGH && buttonBladesPrevState == LOW)
  {
    blades++;
    if (blades > BLADES_MAX)
    {
      blades = BLADES_MIN;
    }
  }
  buttonBladesPrevState = buttonBladesState;

  buttonSensState = digitalRead(PIN_SENS_BUTTON);
  if (buttonSensState == HIGH && buttonSensPrevState == LOW)
  {
    thresholdIndex++;
    if (thresholdIndex > 4)
    {
      thresholdIndex = 0;
    }
  }
  buttonSensPrevState = buttonSensState;

  uint32_t currentMillis = millis();

  if (currentMillis > lastScreenUpdate + SCREEN_UPDATE_THRESHOLD_MILIS)
  {

    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Blades: ");
    display.println(blades);
    display.println("");

    display.setCursor(64, 0);
    display.print("Sens: ");
    display.println(thresholds[thresholdIndex]);
    display.println("");

    display.setTextSize(2);
    display.print("RPM: ");
    display.println(rpm);

    display.print("Freq: ");
    display.println(freqFiltered);

    display.display();

    lastScreenUpdate = currentMillis;
    updated = true;
  }

  val = analogRead(PIN_RECEIVER);
  valFiltered = smooth(val, 0.97, valFiltered);
  valFiltered = val;

//  if (valFiltered > 0)
//  {
//     Serial.println(valFiltered);
//   }

  if (lockMillis != 0 && lockMillis + LOCK_TIME < millis())
  {
    state = LOW;
    digitalWrite(PIN_LED, state);
    lockMillis = 0;
  }

  if (valFiltered > thresholds[thresholdIndex] && lockMillis == 0)
  {
    state = HIGH;
    digitalWrite(PIN_LED, state);
    lockMillis = millis();
  }

  static uint8_t locked = false;

  if (locked == false && valFiltered > thresholds[thresholdIndex] && prevVal < thresholds[thresholdIndex])
  {
    locked = true;
    // Serial.print(valFiltered);
    // Serial.print(" ");
    // Serial.println(prevVal);
    risingMillis = micros();

    uint32_t dst = risingMillis - prevRisingMillis;

    freq = 1000000 / dst;
     Serial.println(freq);
    /*
     * We measure only frequencies that are within range
     */
    if (freq > FREQ_MIN && freq < FREQ_MAX && updated == false)
    {
      freqFiltered = smooth(freq, SMOOTH_FACTOR, freqFiltered);
      rpm = (freqFiltered * 60) / blades;
    }

    updated = false;

    prevRisingMillis = risingMillis;

//#ifdef CATCH_DELAY
//    delayMicroseconds(CATCH_DELAY);
//#endif
    delay(2);
  }

  if (locked == true && valFiltered == 0) {
    locked = false;
  }

  prevVal = valFiltered;
  cycle++;
}
