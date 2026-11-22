// C++ port of the original MicroPython uart_sender/main.py
// Buttons on GP0-GP4 (falling edge) send '0'-'4' to the ESP32 (9600 8N1).
// Debounce: 500 ms, shared by all buttons.
//
// Workaround: on this board the GP8 -> ESP32 IO16 line does not reach the
// ESP32, so we transmit on GP9 (wired to ESP32 IO17) instead. GP9 is not a
// hardware UART TX pin, so a PIO-based UART (SerialPIO, TX only) is used.
// The ESP32 side receives on IO17 with its TX disabled.
#include <Arduino.h>

static const uint8_t BUTTON_PINS[] = {0, 1, 2, 3, 4};
static const uint8_t NUM_BUTTONS = sizeof(BUTTON_PINS);
static const uint32_t DEBOUNCE_MS = 500;
static const uint8_t LINK_TX_PIN = 9;  // GP9 -> ESP32 IO17

static SerialPIO Link(LINK_TX_PIN, NOPIN);  // TX only

// Written in the ISR, read in loop()
static volatile int pressed_index = -1;
static volatile uint32_t debounce_time = 0;

static void handle_press(int index) {
  uint32_t now = millis();
  if (now - debounce_time > DEBOUNCE_MS) {
    debounce_time = now;
    pressed_index = index;
  }
}

static void isr0() { handle_press(0); }
static void isr1() { handle_press(1); }
static void isr2() { handle_press(2); }
static void isr3() { handle_press(3); }
static void isr4() { handle_press(4); }
static void (*const ISRS[])() = {isr0, isr1, isr2, isr3, isr4};

void setup() {
  Serial.begin(115200);  // USB serial (debug output, replaces print())

  Link.begin(9600, SERIAL_8N1);

  for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT);  // same as Pin.IN (no internal pull)
    attachInterrupt(digitalPinToInterrupt(BUTTON_PINS[i]), ISRS[i], FALLING);
  }
}

void loop() {
  int index = pressed_index;
  if (index >= 0) {
    pressed_index = -1;
    char c = '0' + index;
    Link.write(c);  // UART output happens outside the ISR
    Serial.println(c);
    Serial.println("Interrupt Detected");
  }
}
