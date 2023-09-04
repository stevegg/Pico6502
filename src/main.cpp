#include <Arduino.h>
#include "incbin.h"
#include "server.h"

// INCBIN(romSource, "./6502 src/lcdHelloWorld.bin");
INCBIN(romSource, "./6502 src/test.bin");

#define PICO6502_RESETVEC 0xfffc
#define PICO6502_RAM_BASE 0x0000
#define PICO6502_ROM_BASE 0x8000
#define PICO6502_IO_BASE 0x6000
#define DELAY_74HC595 1
#define STEP_DELAY 50
#define ROM_SIZE 32768
// #define USE_GPIO_FUNCS 1

uint32_t stepDelay = 50;

//                         A0, A1, A2, A3, A4, A5, A6, A7
const int addressPins[] = {28, 27, 26, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10};

// 74HC595 Shift Register Pins
const int dataPin = 2;
const int latchPin = 3;
const int serClkPin = 4;

// 6502 Control lines
const int clkPin = 0;
const int rwPin = 1;

// Address decoding pins
const int ramSelPin = 5;
const int ioSelPin = 6;
const int romSelPin = 7;

// Other
const int stepPin = 9;

byte *rom = NULL;

void strobeLatchPin(int value)
{
  digitalWrite(serClkPin, LOW);
  sleep_us(DELAY_74HC595);
  digitalWrite(latchPin, value);
  sleep_us(DELAY_74HC595);
  digitalWrite(serClkPin, HIGH);
  sleep_us(DELAY_74HC595);
}

void writeDataBit(int value)
{
  digitalWrite(serClkPin, LOW);
  sleep_us(DELAY_74HC595);
  digitalWrite(dataPin, value);
  sleep_us(DELAY_74HC595);
  digitalWrite(serClkPin, HIGH);
  sleep_us(DELAY_74HC595);
}

byte writeDataBus(byte value)
{
  strobeLatchPin(LOW);
  for (int i = 7; i >= 0; i--)
  {
    int bit = (value & (1 << i)) ? 1 : 0;
    writeDataBit(bit);
  }
  strobeLatchPin(HIGH);
  return value;
}

word readAddressBus()
{
  word value = 0;
  for (int i = 0; i < 16; i++)
  {
    value |= (digitalRead(addressPins[i]) ? 1 : 0) << i;
  }
  return value;
}

bool singleStep = false;
char addrType[10];
char buffer[200];

void step()
{
  byte data = 0x00;

  // Turn on LED on to indicate we're processing
  digitalWrite(LED_BUILTIN, HIGH);
  // CLK goes LOW
  digitalWrite(clkPin, LOW);

  // Read signal pins
  bool reading = digitalRead(rwPin);
  word address = readAddressBus();

  bool ramSel = (address < PICO6502_IO_BASE);
  bool ioSel = (address >= PICO6502_IO_BASE && address < PICO6502_ROM_BASE);
  bool romSel = (address >= PICO6502_ROM_BASE);

  // Since these outputs are active LOW invert the value of the booleans
  digitalWrite(ramSelPin, !ramSel);
  digitalWrite(ioSelPin, !ioSel);
  digitalWrite(romSelPin, !romSel);

  data = (reading && romSel) ? writeDataBus(rom[address - PICO6502_ROM_BASE]) : 0x00;

  if (singleStep)
  {
    // Output current status to Serial port
    sprintf(buffer, "BUS Addr: %s : %4.4X - Data: %2.2X - DIR: %c",
            (ramSel) ? "RAM" : (ioSel) ? "IO "
                           : (romSel)  ? "ROM"
                                       : "???",
            address, (reading && romSel) ? data : 0x00, reading ? 'R' : 'W');
    Serial.println(buffer);
  }

  // Delay
  // sleep_ms((singleStep) ? 0 : STEP_DELAY);

  // Turn off LED
  digitalWrite(LED_BUILTIN, LOW);
  // Set CLK HIGH
  digitalWrite(clkPin, HIGH);
}

void setup()
{

  //
#ifdef USE_GPIO_FUNCS
  gpio_set_dir(LED_BUILTIN, GPIO_OUT);

  // IOSelect
  gpio_set_dir(ioSelPin, GPIO_OUT);
  gpio_put(ioSelPin, HIGH);

  // RAMSelect set to HIGH initially
  gpio_set_dir(ramSelPin, GPIO_OUT);
  gpio_put(ramSelPin, HIGH);

  // ROMSelect set to HIGH
  gpio_set_dir(romSelPin, GPIO_OUT);
  gpio_put(romSelPin, HIGH);

  // Setup pins for outputting to the databus via the 74HC595
  gpio_set_dir(dataPin, GPIO_OUT);
  gpio_set_dir(latchPin, GPIO_OUT);
  gpio_set_dir(serClkPin, GPIO_OUT);

  gpio_put(latchPin, HIGH);
  gpio_put(serClkPin, HIGH);
  gpio_put(dataPin, LOW);

  // Step button setup as a PULLUP Input
  gpio_set_dir(stepPin, GPIO_IN);
  gpio_pull_up(stepPin);

  for (int i = 0; i < 16; i++)
  {
    // Address pins are input
    gpio_set_dir(addressPins[i], GPIO_IN);
  }
  // CLK is an output
  gpio_set_dir(clkPin, GPIO_OUT);
  gpio_put(clkPin, HIGH);

  // RW pin is an input
  gpio_set_dir(rwPin, GPIO_IN);

#else
  // Builtin LED
  pinMode(LED_BUILTIN, OUTPUT);

  // IOSelect set to HIGH initially
  pinMode(ioSelPin, OUTPUT);
  digitalWrite(ioSelPin, HIGH);

  // RAMSelect set to HIGH initially
  pinMode(ramSelPin, OUTPUT);
  digitalWrite(ramSelPin, HIGH);

  // ROMSelect set to HIGH
  pinMode(romSelPin, OUTPUT);
  digitalWrite(romSelPin, HIGH);

  // Setup pins for outputting to the databus via the 74HC595
  pinMode(dataPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(serClkPin, OUTPUT);

  digitalWrite(latchPin, HIGH);
  digitalWrite(serClkPin, HIGH);
  digitalWrite(dataPin, LOW);

  // Step button
  pinMode(stepPin, INPUT_PULLDOWN);

  for (int i = 0; i < 16; i++)
  {
    // Address pins are input
    pinMode(addressPins[i], INPUT);
  }
  // CLK is an output
  pinMode(clkPin, INPUT);
  digitalWrite(clkPin, HIGH);

  // RW pin is an input
  pinMode(rwPin, INPUT);

  attachInterrupt(digitalPinToInterrupt(clkPin), step, FALLING);

#endif

  Serial.begin(9600);
  sleep_ms(5000);

  // Setup ROM
  rom = (byte *)malloc(ROM_SIZE);
  memset(rom, 0, ROM_SIZE);
  memcpy(rom, gromSourceData, gromSourceSize);
  word resetVectorROMAddress = PICO6502_RESETVEC - PICO6502_ROM_BASE;
  char localBuff[80];
  sprintf(localBuff, "Reset Vector address in ROM is %4.4X", resetVectorROMAddress);
  Serial.println(localBuff);
  rom[resetVectorROMAddress] = lowByte(PICO6502_ROM_BASE);
  rom[resetVectorROMAddress + 1] = highByte(PICO6502_ROM_BASE);
  sprintf(localBuff, "Copied %2.2X bytes from romSource to RAM");
  Serial.println(localBuff);

  Serial.println("Pico6502 Starting...");
}

unsigned char button_checker;

void loop()
{

  if (singleStep)
  {
    button_checker <<= 1;
    button_checker += digitalRead(stepPin);

    if (button_checker == 0x80)
    {
      step();
    }
  }
  else
  {
    step();
  }
  sleep_ms(stepDelay);
}

// // Network Code on second core
// const char *ssid = "GIoT";
// const char *password = "IC7610B4by!";

// void setSpeed(arduino::String value) {
//   Serial.print("Route matched and callback called with value:");
//   Serial.println(value);
//   stepDelay = value.toInt();
// }

// void setup1() {
//   // Start Serial Monitor
//   Serial.begin(9600);

//   // Initialize the LED as an output
//   pinMode(LED_BUILTIN, OUTPUT);

//   // Set LED off
//   digitalWrite(LED_BUILTIN, LOW);

//   setupServer(ssid, password);
//   registerRoute("GET /speed/", setSpeed);

// }

// void loop1() {

//   processServerClient();

// }
