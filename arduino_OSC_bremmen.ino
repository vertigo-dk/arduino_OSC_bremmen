//              INCLUDE                           //
////////////////////////////////////////////////////
#include <SPI.h>
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <OSCMessage.h>
#include <OSCBundle.h>

#include <Bounce2.h>

////////////////////////////////////////////////////
//               Defines
////////////////////////////////////////////////////
#define NUM_BUTTONS 5

#define pwrLED 12
#define statBUTTON 19         //19 = A1

char BUTTON_PIN[NUM_BUTTONS] = { 2, 3, 4, 5, 6 };
char LED_PIN[NUM_BUTTONS] = {8, 9, A3, 11, 7 };
int rotoStat = 0;
////////////////////////////////////////////////////
//            Ethernet
////////////////////////////////////////////////////
//UDP communication
EthernetUDP Udp;

//the self asignd IP
byte mac[6] = {0x90, 0xA2, 0xDA, 0x10, 0x38, 0x33};

//port numbers
const unsigned int outPort = 53000;
const unsigned int inPort = 8888;
const unsigned int NETPwrCtrl_outPort = 75;
const unsigned int NETPwrCtrl_inPort = 77;

IPAddress broadcast(255, 255, 255, 255);
IPAddress outIp(192, 168, 1, 110);
IPAddress inIp(192, 168, 1, 150);
IPAddress NETPwrIP(192, 168, 1, 50);

boolean isConct = false;
boolean onoff = false;

int timeOut = 0;

////////////////////////////////////////////////////
//            debounce
////////////////////////////////////////////////////

Bounce debouncer[NUM_BUTTONS] = {Bounce(), Bounce(), Bounce(), Bounce(), Bounce()};

void blink(int pause, int reapet) {
  for (int i = 0; i < reapet; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(pause);
    digitalWrite(LED_BUILTIN, LOW);
    delay(pause);
  }
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  // while (!Serial) {
  //    // wait for serial port to connect. Needed for native USB
  // }
  Serial.println("Setup");
  pinMode(pwrLED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(statBUTTON, INPUT_PULLUP);
  digitalWrite(pwrLED, HIGH);
  digitalWrite(LED_BUILTIN, LOW);

  Ethernet.begin(mac, inIp);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(LED_PIN[i], OUTPUT);
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);
    debouncer[i].attach(BUTTON_PIN[i]);
    debouncer[i].interval(5);
  }
}

void isPing(OSCMessage &msg, int addrOffset) {
  Serial.print("ping  ");
  Serial.println(msg.getInt(0));
  if(msg.isInt(0)) {
    if (msg.getInt(0) == 2016) {
      Serial.println("ping good  ");
      isConct = 1;
      return;
    }
  }
  isConct = 0;
}

char * numToOSCAddress( int pin){
    static char s[10];
    int i = 9;

    s[i--]= '\0';
  do
    {
    s[i] = "0123456789"[pin % 10];
                --i;
                pin /= 10;
    }
    while(pin && i);
    s[i] = '/';
    return &s[i];
}

void LEDset(OSCMessage &msg, int addrOffset) {
  Serial.print("in LEDset");
  Serial.println(addrOffset);
  for(byte pin = 0; pin < NUM_BUTTONS; pin++){
    int pinMatched = msg.match(numToOSCAddress(pin), addrOffset);
//    Serial.println(pinMatched);
    if(pinMatched){
      if (msg.isInt(0)){
        Serial.print(msg.getInt(0));
        digitalWrite(LED_PIN[pin],msg.getInt(0));
      }
    }
  }
}

void loop() {
  boolean change = false;
  OSCBundle button;
  int sensorValue = analogRead(A0);
  if(sensorValue > 1000) {
    sensorValue = 9;
  }else if (sensorValue > 850) {
    sensorValue = 8;
  }else if (sensorValue > 700) {
    sensorValue = 7;
  }else if (sensorValue > 650) {
    sensorValue = 6;
  }else if (sensorValue > 520) {
    sensorValue = 5;
  }else if (sensorValue > 400) {
    sensorValue = 4;
  }else if (sensorValue > 300) {
    sensorValue = 3;
  }else if (sensorValue > 180) {
    sensorValue = 2;
  }else if (sensorValue > 60) {
    sensorValue = 1;
  }else {
    sensorValue = 0;
  }
  if (rotoStat != sensorValue) {
    rotoStat = sensorValue;
    button.add("/rot").add((int)rotoStat);
    Serial.print("rotoStat   ");
    Serial.println(rotoStat);
    change = true;
  }

  // Update the Bounce instances :
  for (int i = 0; i < NUM_BUTTONS; i++) {
    debouncer[i].update();
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    if ( debouncer[i].fell() || debouncer[i].rose() ) {
      char str[9] = "/button/";
      str[8] = 49 + i;
      button.add(str).add((int)debouncer[i].read());
      Serial.println(debouncer[i].read());
      change = true;
    }
  }
  if (change == true) {
    Serial.println("send osc");
    Udp.beginPacket(outIp, outPort);
    button.send(Udp); // send the bytes to the SLIP stream
    Udp.endPacket(); // mark the end of the OSC Packet
    button.empty();
  }
  OSCMessage msgIn;
  char size;
  if( (size = Udp.parsePacket())>0) {
    while(size--) {
      msgIn.fill(Udp.read());
    }
    Serial.println("");
    if(!msgIn.hasError()) {
      Serial.println("recive OSC");
      msgIn.route("/LED", LEDset);
    }
  }
  msgIn.empty();
}
