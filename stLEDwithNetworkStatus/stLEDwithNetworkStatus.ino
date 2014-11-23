//*****************************************************************************
/// @file
/// @brief
///   Arduino SmartThings Shield LED Example with Network Status
/// @note
///              ______________
///             |              |
///             |         SW[] |
///             |[]RST         |
///             |         AREF |--
///             |          GND |--
///             |           13 |--X LED
///             |           12 |--
///             |           11 |--
///           --| 3.3V      10 |--
///           --| 5V         9 |--
///           --| GND        8 |--mode_change
///           --| GND          |
///           --| Vin        7 |--on_off
///             |            6 |--low_sense
///           --| A0         5 |--medium_sense
///           --| A1    ( )  4 |--high_sense
///           --| A2         3 |--X THING_RX
///           --| A3  ____   2 |--X THING_TX
///           --| A4 |    |  1 |--
///           --| A5 |    |  0 |--
///             |____|    |____|
///                  |____|
///
//*****************************************************************************
#include <SoftwareSerial.h>   //TODO need to set due to some weird wire language linker, should we absorb this whole library into smartthings
#include <SmartThings.h>

//*****************************************************************************
// Pin Definitions    | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                    V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************
#define PIN_LED         13
#define PIN_THING_RX    3
#define PIN_THING_TX    2
#define PIN_HIGHSENSE   3 //Analog pins
#define PIN_MEDIUMSENSE 4 //Analog pins
#define PIN_LOWSENSE    5 //Analog pins
#define PIN_ONOFF       8
#define PIN_MODECHANGE  7

//*****************************************************************************
// Global Variables   | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                    V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************
SmartThingsCallout_t messageCallout;    // call out function forward decalaration
SmartThings smartthing(PIN_THING_RX, PIN_THING_TX, messageCallout, "smartShield", true);  // constructor

bool isDebugEnabled;    // enable or disable debug in this example
int stateLED;           // state to track last set value of LED
int stateNetwork;       // state of the network 
byte heater_status = 0x00;
int ledChannels[3] = {PIN_LOWSENSE, PIN_MEDIUMSENSE, PIN_HIGHSENSE};
unsigned long ledAverages[3] = {0, 0, 0};
unsigned int ledAverageTracker = 0;
                          

//*****************************************************************************
// Local Functions  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                  V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************
void on()
{
  while(heater_status ==0){
    toggle_power();
    for (int i = 0; i < 64; i++) {
      heater_update();
      delay(10);
    }
  }
  smartthing.send("on");        // send message to cloud
}

//*****************************************************************************
void off()
{
  while(heater_status >0){
    toggle_power();
    for (int i = 0; i < 64; i++) {
      heater_update();
      delay(10);
    }
  }
  smartthing.send("off");       // send message to cloud
}

void heater_update()
{
  for (int i = 0; i < 3; i++) {
     if (analogRead(ledChannels[i]) < 1000) {
       ledAverages[i] |= (1 << ledAverageTracker);
     } else {
       ledAverages[i] &= ~(1 << ledAverageTracker);
     }
     if (ledAverages[i] > 0) {
       heater_status |= (1 << i);
     } else {
       heater_status &= ~(1 << i);
     }
//     Serial.println(ledAverages[i],BIN);
  }
  ledAverageTracker = (ledAverageTracker + 1) & 0x1F;
}

void toggle_mode() {
  Serial.print("Toggle Mode : ");
  Serial.println(heater_status, BIN);
  digitalWrite(PIN_MODECHANGE, LOW);
  pinMode(PIN_MODECHANGE, OUTPUT);
  delay(200);
  pinMode(PIN_MODECHANGE, INPUT);
  delay(500);
}

void toggle_power() {
  Serial.print("Toggle Power: ");
  Serial.println(heater_status, BIN);
  digitalWrite(PIN_ONOFF, LOW);
  pinMode(PIN_ONOFF, OUTPUT);
  delay(500);
  pinMode(PIN_ONOFF, INPUT);
  delay(500);
}

void set_heater(unsigned int heater)
{
  if (heater_status == 0) {
    Serial.print("No LED : ");
  Serial.println(heater_status, BIN);
    return;
  }
  while (heater_status != heater) {
    toggle_mode();
    for (int i = 0; i < 64; i++) {
      heater_update();
      delay(10);
    }
  }
}

//*****************************************************************************
void setNetworkStateLED()
{
  SmartThingsNetworkState_t tempState = smartthing.shieldGetLastNetworkState();
  if (tempState != stateNetwork)
  {
    switch (tempState)
    {
      case STATE_NO_NETWORK:
        if (isDebugEnabled) Serial.println("NO_NETWORK");
        smartthing.shieldSetLED(2, 0, 0); // red
        break;
      case STATE_JOINING:
        if (isDebugEnabled) Serial.println("JOINING");
        smartthing.shieldSetLED(2, 0, 0); // red
        break;
      case STATE_JOINED:
        if (isDebugEnabled) Serial.println("JOINED");
        smartthing.shieldSetLED(0, 0, 0); // off
        break;
      case STATE_JOINED_NOPARENT:
        if (isDebugEnabled) Serial.println("JOINED_NOPARENT");
        smartthing.shieldSetLED(2, 0, 2); // purple
        break;
      case STATE_LEAVING:
        if (isDebugEnabled) Serial.println("LEAVING");
        smartthing.shieldSetLED(2, 0, 0); // red
        break;
      default:
      case STATE_UNKNOWN:
        if (isDebugEnabled) Serial.println("UNKNOWN");
        smartthing.shieldSetLED(0, 2, 0); // green
        break;
    }
    stateNetwork = tempState; 
  }
}

//*****************************************************************************
// API Functions    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
//                  V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
//*****************************************************************************
void setup()
{
  // setup default state of global variables
  isDebugEnabled = true;
  stateLED = 0;                 // matches state of hardware pin set below
  stateNetwork = STATE_JOINED;  // set to joined to keep state off if off
  
  // setup hardware pins 
  pinMode(PIN_LED, OUTPUT);     // define PIN_LED as an output
  pinMode(PIN_ONOFF, INPUT);
  pinMode(PIN_MODECHANGE, INPUT);
  digitalWrite(PIN_LED, LOW);   // set value to LOW (off) to match stateLED=0

  if (isDebugEnabled)
  { // setup debug serial port
    Serial.begin(9600);         // setup serial with a baud rate of 9600
    Serial.println("setup..");  // print out 'setup..' on start
  }
}

//*****************************************************************************
void loop()
{
  // run smartthing logic
  smartthing.run();
  setNetworkStateLED();
  for (int i = 0; i < 64; i++) {
    heater_update();
    delay(10);
  }
//  if (temp) {
//    set_heater(4);
//    temp = 0;
//  } else {
//    set_heater(1);
//    temp = 1;
//  }
}

//*****************************************************************************
void messageCallout(String message)
{
  // if debug is enabled print out the received message
  if (isDebugEnabled)
  {
    Serial.print("Received message: '");
    Serial.print(message);
    Serial.println("' ");
  }

  // if message contents equals to 'on' then call on() function
  // else if message contents equals to 'off' then call off() function
  if (message.equals("on"))
  {
    on();
    set_heater(0x01);
    smartthing.send("low_power"); 
  }
  else if (message.equals("off"))
  {
    off();
  }
  else if (message.equals("siren"))
  {
    on();
    set_heater(0x04);
    smartthing.send("low"); 
    Serial.println("LOWPOWER");
  }
  else if (message.equals("strobe"))
  {
    on();
    set_heater(0x02);
    smartthing.send("medium power"); 
  }
  else if (message.equals("both"))
  {
    on();
    set_heater(0x01);
    smartthing.send("high power"); 
  }
 
  
}
