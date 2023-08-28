#include <Arduino.h>
#include <WiFi.h>

#define RESET_VECTOR 0xfffc
#define PICO6502_ROM_BASE 0x8000
#define DELAY_74HC595 20

uint32_t stepDelay = 50;

//                         A0, A1, A2, A3, A4, A5, A6, A7
const int addressPins[] = {28, 27, 26, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10};

const int clkPin = 0;
const int rwPin = 1;
const int dataPin = 2;
const int latchPin = 3;
const int serClkPin = 4;
const int stepPin = 9;
const int ramSelPin = 5;
const int ioSelPin = 6;
const int romSelPin = 7;


byte rom[] ={ 0xa9, 0xff, 0x8d, 0x02, 0x60, 0xa9, 0x50, 0x8d,
              0x00, 0x60, 0x20, 0x10, 0x80, 0x4c, 0x0a, 0x80,
              0x6a, 0x8d, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00};

// byte rom[] = {0xa9, 0xff, 0x8d, 0x02, 0x60, 0xa9, 0x50, 0x8d, 
//               0x00, 0x60, 0x6a, 0x8d, 0x00, 0x60, 0x4c, 0x0a,
//               0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void strobeLatchPin(int value) {
  digitalWrite(serClkPin, LOW);
  sleep_us(DELAY_74HC595);
  digitalWrite(latchPin, value);
  sleep_us(DELAY_74HC595);
  digitalWrite(serClkPin, HIGH);
  sleep_us(DELAY_74HC595);
}

void writeDataBit(int value) {
  digitalWrite(serClkPin, LOW);
  sleep_us(DELAY_74HC595);
  digitalWrite(dataPin, value);
  sleep_us(DELAY_74HC595);
  digitalWrite(serClkPin, HIGH);
  sleep_us(DELAY_74HC595);
}

void writeDataBus(byte value) {
  strobeLatchPin(LOW);
  for ( int i = 7; i >= 0; i -- ) {
    int bit = (value & (1 << i)) ? 1 : 0;
    writeDataBit(bit);
  }
  strobeLatchPin(HIGH);
}

word readAddressBus() {
  word value = 0;
  for ( int i = 0; i < 16; i ++ ) {
    value |= (gpio_get(addressPins[i]) ? 1 : 0) << i;
  }
  return value;
}

bool singleStep = false;

void step() {
  
  byte data = 0xFF;
  char addrType[10];
  char buffer[200];
  // Turn on LED on to indicate we're processing
  digitalWrite(LED_BUILTIN, HIGH);
  // CLK goes LOW
  digitalWrite(clkPin, LOW);

  // Read signal pins
  bool reading = gpio_get(rwPin);
  word address = readAddressBus();
  
  gpio_put(ramSelPin, (address < 0x6000) ? false : true);
  gpio_put(ioSelPin, (address >= 0x6000 && address < 0x8000) ? false : true);
  gpio_put(romSelPin, (address >= 0x8000) ? false : true);
  boolean handle = ((address >= 0x8000) & reading);

  // Where are we reading/writing
  if ( address < 0x6000) {
    strcpy(addrType, "RAM");
  } else if ( address >= 0x6000 && address < 0x8000) {
    strcpy(addrType, "IO ");
  } else {
    strcpy(addrType, "ROM");
  }

  if ( handle ) {
    // Handle reset vector better
    if ( address == RESET_VECTOR) {
      data = 0x00;
    } else if (address == RESET_VECTOR + 1) {
      data = 0x80;
    } else {
      data = rom[address-PICO6502_ROM_BASE];
    }
    writeDataBus(data);
  }

  // Set CLK HIGH
  digitalWrite(clkPin, HIGH);

  // Output current status to Serial port  
  // sprintf(buffer, "BUS Addr: %s : %4.4X - Data: %2.2X - DIR: %c", addrType, address, data, reading ? 'R' : 'W');
  // Serial.println(buffer);

  // Turn off LED
  gpio_put(LED_BUILTIN, false);
  // digitalWrite(LED_BUILTIN, LOW);
}


void setup() {
  // Setup RAM
  gpio_put(LED_BUILTIN, true);
  // pinMode(LED_BUILTIN, OUTPUT);

  // IOSelect set to HIGH initially
  pinMode(ioSelPin, OUTPUT);
  digitalWrite(ioSelPin, HIGH);

  // RAMSelect set to HIGH initially
  pinMode(ramSelPin, OUTPUT);
  digitalWrite(ramSelPin, HIGH);

  pinMode(romSelPin, OUTPUT);
  digitalWrite(romSelPin, HIGH);

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

  if ( singleStep ) {
    button_checker <<= 1;     
    button_checker += digitalRead(stepPin);
    if ( button_checker == 0x80) {
      step();
    }
  } else {
    step();
  }
  sleep_ms(stepDelay);
}


// Network Code



const char *ssid = "GIoT";
const char *password = "IC7610B4by!";

// Set web server port number to 80
WiFiServer server(80);
 
// Variable to store the HTTP request
String header;
 
// Variable to store onboard LED state
String picoLEDState = "off";
 
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup1() {
  // Start Serial Monitor
  Serial.begin(9600);
 
  // Initialize the LED as an output
  pinMode(LED_BUILTIN, OUTPUT);
 
  // Set LED off
  digitalWrite(LED_BUILTIN, LOW);
 
  // Connect to Wi-Fi network with SSID and password
  WiFi.begin(ssid, password);
 
  // Display progress on Serial monitor
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  // Print local IP address and start web server
  Serial.println("");
  Serial.print("WiFi connected at IP Address ");
  Serial.println(WiFi.localIP());
 
  // Start Server
  server.begin();
}

const char *SPEED_SET = "GET /speed/";

void loop1() {
WiFiClient client = server.available();   // Listen for incoming clients
 
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf(SPEED_SET) >= 0 ) {
              Serial.println("Setting speed");
              int len = strlen(SPEED_SET);
              String value = header.substring(len, header.length() - len);
              value = value.substring(0, value.length() - strlen(" HTTP/1.1"));
              Serial.print("\nValue: ");
              Serial.println(value);
              stepDelay = value.toInt();
              Serial.print("Speed value now :");
              Serial.println(stepDelay);
            }
 
            // Switch the LED on and off
            if (header.indexOf("GET /led/on") >= 0) {
              Serial.println("LED on");
              picoLEDState = "on";
              digitalWrite(LED_BUILTIN, HIGH);
            } else if (header.indexOf("GET /led/off") >= 0) {
              Serial.println("LED off");
              picoLEDState = "off";
              digitalWrite(LED_BUILTIN, LOW);
            }
 
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
 
            // CSS to style the on/off buttons
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #F23A3A;}</style></head>");
 
            // Web Page Heading
            client.println("<body><h1>Pico W LED Control</h1>");
 
            // Display current state, and ON/OFF buttons for Onboard LED
            client.println("<p>Onboard LED is " + picoLEDState + "</p>");
            
            // Set buttons
            if (picoLEDState == "off") {
              
              //picoLEDState is off, display the ON button
              client.println("<p><a href=\"/led/on\"><button class=\"button\">ON</button></a></p>");
            } else {
 
              //picoLEDState is on, display the OFF button
              client.println("<p><a href=\"/led/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
 
            client.println("</body></html>");
 
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}




