#include <Arduino.h>

#define RESET_VECTOR 0xfffc
#define ROM_BASE 0x8000
#define DELAY_74HC595 5

//                         A0, A1, A2, A3, A4, A5, A6, A7
const int addressPins[] = {28, 27, 26, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10};

//                      D0,D1,D2,D3,D4,D5,D6,D7
const int dataPins[] = { 2, 3, 4, 5, 6, 7, 8, 9};

const int clkPin = 0;
const int rwPin = 1;
const int dataPin = 2;
const int latchPin = 3;
const int serClkPin = 4;
const int stepPin = 9;
const int ramSelPin = 5;
const int ioSelPin = 6;
byte rom[] = {0xa9, 0xff, 0x8d, 0x02, 0x60, 0xa9, 0x50, 0x8d, 
              0x00, 0x60, 0x6a, 0x8d, 0x00, 0x60, 0x4c, 0x0a,
              0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void strobeLatchPin(int value) {
  digitalWrite(serClkPin, LOW);
  sleep_ms(DELAY_74HC595);
  digitalWrite(latchPin, value);
  sleep_ms(DELAY_74HC595);
  digitalWrite(serClkPin, HIGH);
  sleep_ms(DELAY_74HC595);
}

void writeDataBit(int value) {
    digitalWrite(serClkPin, LOW);
    sleep_ms(DELAY_74HC595);
    digitalWrite(dataPin, value);
    sleep_ms(DELAY_74HC595);
    digitalWrite(serClkPin, HIGH);
    sleep_ms(DELAY_74HC595);
}

void writeDataBus(byte value) {
  strobeLatchPin(LOW);
  for ( int i = 7; i >= 0; i -- ) {
    int bit = (value & (1 << i)) ? 1 : 0;
    writeDataBit(bit);
  }
  strobeLatchPin(HIGH);
}

byte readDataBus() {
  byte value = 0;
  for ( int i = 7; i >= 0; i -- ) {
    value |= digitalRead(dataPins[i]) << i;
  }
  return value;
}

word readAddressBus() {
  word value = 0;
  for ( int i = 0; i < 16; i ++ ) {
    value |= digitalRead(addressPins[i]) << i;
  }
  return value;
}

void step() {
  char addrType[10];
  char buffer[200];
  digitalWrite(LED_BUILTIN, HIGH);

  digitalWrite(clkPin, LOW);
  sleep_ms(150);
  bool reading = digitalRead(rwPin);

  // Read address
  word address = readAddressBus();
  
  byte data = 0xEA;

  // Where are we reading/writing
  boolean handle = false;
  if ( address < 0x6000) {
    strcpy(addrType, "RAM");
    digitalWrite(ramSelPin, LOW);
    digitalWrite(ioSelPin, HIGH);
  } else if ( address >= 0x6000 && address < 0x8000) {
    strcpy(addrType, "IO ");
    digitalWrite(ioSelPin, LOW);
    digitalWrite(ramSelPin, HIGH);
  } else {
    handle = true;
    digitalWrite(ramSelPin, HIGH);
    digitalWrite(ioSelPin, HIGH);
    strcpy(addrType, "ROM");
  }

  if ( reading && handle ) {
    // Handle reset vector better
    if ( address == RESET_VECTOR) {
      data = 0x00;
    } else if (address == RESET_VECTOR + 1) {
      data = 0x80;
    } else {
      data = rom[address-ROM_BASE];
    }
    writeDataBus(data);
  }

  sleep_ms(200);
  digitalWrite(clkPin, HIGH);

  sleep_ms(150);
  
  sprintf(buffer, "BUS Addr: %s : %4.4X - Data: %2.2X - DIR: %c", addrType, address, data, reading ? 'R' : 'W');
  Serial.println(buffer);

  digitalWrite(LED_BUILTIN, LOW);
  sleep_ms(500);
}


void setup() {
  // Setup RAM
  pinMode(LED_BUILTIN, OUTPUT);

  // IOSelect set to HIGH initially
  pinMode(ioSelPin, OUTPUT);
  digitalWrite(ioSelPin, HIGH);

  // RAMSelect set to HIGH initially
  pinMode(ramSelPin, OUTPUT);
  digitalWrite(ramSelPin, HIGH);

  // Setup pins for outputting to the databus via the 74HC595  
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(serClkPin, OUTPUT);

  digitalWrite(latchPin, HIGH);
  digitalWrite(serClkPin, HIGH);
  digitalWrite(dataPin, LOW);

  pinMode(stepPin, INPUT_PULLDOWN);

  for ( int i = 0; i < 16; i ++) {
    // Address pins are input
    pinMode(addressPins[i], INPUT);
  }
  // CLK is an output
  pinMode(clkPin, OUTPUT);
  // RW pin is an input
  pinMode(rwPin, INPUT_PULLUP);

  // attachInterrupt(digitalPinToInterrupt(clkPin), process, RISING);

  Serial.begin(9600);
  Serial.println("Starting...");
}

unsigned char button_checker;

void loop() {

  button_checker <<= 1;     
  button_checker += digitalRead(stepPin);
  if ( button_checker == 0x80) {
    step();
  }

}



