/**************************************************/
/*              Defines
/**************************************************/
#define NUM_BUTTONS 5

#define pwrLED 12
#define statLED 13
#define statBUTTON 19         //19 = A1

byte BUTTON_PIN[NUM_BUTTONS] = {
  2, 3, 4, 5, 6
};
byte LED_PIN[NUM_BUTTONS] = {
  7, 8, 9, A3, 11
};

/**************************************************/
/*              INCLUDE
/**************************************************/
#include <SPI.h>
#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <OSCMessage.h>
#include <OSCBundle.h>

#include <Bounce2.h>

#include "SerialDebug.h"

#if defined(__MK20DX128__)  //if it id a teensy include a MAC addr. read

#else
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte read_mac(byte *mac) {
 byte uniqMAC[] = {0x90, 0xA2, 0xDA, 0x10, 0x39, 0x43};
 memcpy(mac, uniqMAC, 6);
}
#endif

/**************************************************/
/*            Ethernet
/**************************************************/
//UDP communication
EthernetUDP Udp;

//the self asignd IP
IPAddress ip(192, 168, 1, 61);
byte mac[6] = {0x90, 0xA2, 0xDA, 0x10, 0x38, 0x33};

//port numbers
const unsigned int QLabPort = 53000;
const unsigned int outPort = 9009;
const unsigned int inPort = 8888;
const unsigned int NETPwrCtrl_outPort = 75;
const unsigned int NETPwrCtrl_inPort = 77;

IPAddress broardcast(255, 255, 255, 255);
IPAddress outIp(192, 168, 1, 38);

IPAddress NETPwrIP(192, 168, 1, 61);

boolean isConct = false;
boolean onoff = false;

/**************************************************/
/*            debounce
/**************************************************/
Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();
Bounce debouncer3 = Bounce();
Bounce debouncer4 = Bounce();
Bounce debouncer5 = Bounce();

void waitForPowerOn() {
  while(digitalRead(statBUTTON) == HIGH) {
    delay(100);
  }
}

void isPing(OSCMessage &msg) {
  if(msg.isInt(0)) {
    if (msg.getInt(0) == 2016) {
      isConct = 1;
      return;
    }
  }
  isConct = 0;
}

int pingBack(int timeOut) {
  OSCMessage msgOut("/ping");
  msgOut.add("powerON");
  Udp.beginPacket(outIp, QLabPort);
  msgOut.send(Udp); // send the bytes to the SLIP stream
  Udp.endPacket(); // mark the end of the OSC Packet
  Serial.println("send OSC");
  OSCMessage msgIn;
  //delay(timeOut);

  delay(100);
  for (int i = 0; i < timeOut; i++) {
    int size;
    if( (size = Udp.parsePacket())>0) {
      while(size--) {
        msgIn.fill(Udp.read());
      }
      if(!msgIn.hasError()) {
        msgIn.route("/ping", isPing);
        return isConct;
      }
    }
    delay(10);
  }

  msgOut.empty(); // free space occupied by message
  msgIn.empty();
  return 0;
}

void setup() {
   Serial.begin(9600);
  pinMode(pwrLED, OUTPUT);
  pinMode(statLED, OUTPUT);
  pinMode(statBUTTON, INPUT_PULLUP);
  digitalWrite(pwrLED, HIGH);
  digitalWrite(statLED, LOW);

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(LED_PIN[i], OUTPUT);
  }

  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);
  }
  debouncer1.attach(BUTTON_PIN[0]);
  debouncer1.interval(5); // interval in ms
  debouncer2.attach(BUTTON_PIN[1]);
  debouncer2.interval(5); // interval in ms
  debouncer3.attach(BUTTON_PIN[2]);
  debouncer3.interval(5); // interval in ms
  debouncer4.attach(BUTTON_PIN[3]);
  debouncer4.interval(5); // interval in ms
  debouncer5.attach(BUTTON_PIN[4]);
  debouncer5.interval(5); // interval in ms

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
    Serial.println(pinMatched);
    if(pinMatched){
      if (msg.isInt(0)){
        Serial.print(msg.getInt(0));
        digitalWrite(LED_PIN[pin],msg.getInt(0));
      }
    }
  }
}

void loop() {
  if (onoff == 1) {
    boolean change = false;
    // Update the Bounce instances :
    debouncer1.update();
    debouncer2.update();
    debouncer3.update();
    debouncer4.update();
    debouncer5.update();
    OSCBundle button;
    if ( debouncer1.fell() || debouncer1.rose() ) {
      button.add("/button/1").add((int)debouncer1.read());
      Serial.println(debouncer1.read());
      change = true;
    }
    if ( debouncer2.fell() || debouncer2.rose() ) {
      button.add("/button/2").add((int)debouncer2.read());
      change = true;
    }
    if ( debouncer3.fell() || debouncer3.rose() ) {
      button.add("/button/3").add((int)debouncer3.read());
      change = true;
    }
    if ( debouncer4.fell() || debouncer4.rose() ) {
      button.add("/button/4").add((int)debouncer4.read());
      change = true;
    }
    if ( debouncer5.fell() || debouncer5.rose() ) {
      button.add("/button/5").add((int)debouncer5.read());
      change = true;
    }
    if (change == true) {
      Serial.println("send osc");
      Udp.beginPacket(outIp, QLabPort);
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
  }else{

  }

  if (digitalRead(statBUTTON) == LOW) {
    delay(100);
    if (digitalRead(statBUTTON) == LOW) {
      isConct = 0;
      if (onoff == 0) {
        Serial.println("turning ON"); //<-------Serial print
        for (int i = 0; i < 10; i++) {
          digitalWrite(statLED, HIGH);
          delay(100);
          digitalWrite(statLED, LOW);
          delay(100);
        }
        while(Ethernet.begin(mac, 1000) == 0) {
          for (int i = 0; i < 4; i++) {
            digitalWrite(statLED, HIGH);
            delay(50);
            digitalWrite(statLED, LOW);
            delay(100);
          }
        }
        Serial.println("ethernet ON"); //<-------Serial print
        char UdpPacket[] = "net-PwrCtrl";
        Udp.begin(NETPwrCtrl_inPort);
        int packetSize = 0;
        while (memcmp(UdpPacket, "NET-PwrCtrl:NET-CONTROL", sizeof(UdpPacket)) != 0) {
          digitalWrite(statLED, HIGH);
          Udp.beginPacket(broardcast, NETPwrCtrl_outPort);
          Udp.write("wer da?");
          Udp.write(0x0D);
          Udp.write(0x0A);
          Udp.endPacket();
          delay(100);
          digitalWrite(statLED, LOW);
          packetSize = Udp.parsePacket();
          Udp.read(UdpPacket, sizeof(UdpPacket));
        }
        Serial.println("NETPwrCtrl conctet"); //<-------Serial print
        NETPwrIP = Udp.remoteIP();
        Udp.beginPacket(NETPwrIP, NETPwrCtrl_outPort);
        Udp.write("Sw");
        Udp.write(0b10000111);
        Udp.write("user1");
        Udp.write("1234");
        Udp.write(0x0D);
        Udp.write(0x0A);
        Udp.endPacket();
        Udp.stop();
        Serial.println("NETPwrCtrl ON"); //<-------Serial print
        Udp.begin(inPort);
        while(isConct != 1){
          OSCMessage msgOut("/ping");
          msgOut.add("powerON");
          Udp.beginPacket(outIp, QLabPort);
          msgOut.send(Udp); // send the bytes to the SLIP stream
          Udp.endPacket(); // mark the end of the OSC Packet
          OSCMessage msgIn;
          digitalWrite(statLED, HIGH);
          delay(70);
          digitalWrite(statLED, LOW);
          delay(50);
          for (int i = 0; i < 10; i++) {
            int size;
            if( (size = Udp.parsePacket())>0) {
              while(size--) {
                msgIn.fill(Udp.read());
              }
              if(!msgIn.hasError()) {
                msgIn.route("/ping", isPing);
              }
            }
            delay(10);
          }
        }
        Serial.println("OSC confirm"); //<-------Serial print
        onoff = 1;
        digitalWrite(statLED, HIGH);
      }else{
        Serial.println("turning OFF");
        while(isConct != 1){
          OSCMessage msgOut("/ping");
          msgOut.add("powerOFF");
          Udp.beginPacket(outIp, QLabPort);
          msgOut.send(Udp); // send the bytes to the SLIP stream
          Udp.endPacket(); // mark the end of the OSC Packet
          OSCMessage msgIn;
          //delay(timeOut);
          digitalWrite(statLED, HIGH);
          delay(70);
          digitalWrite(statLED, LOW);
          delay(50);
          for (int i = 0; i < 10; i++) {
            int size;
            if( (size = Udp.parsePacket())>0) {
              while(size--) {
                msgIn.fill(Udp.read());
              }
              if(!msgIn.hasError()) {
                msgIn.route("/ping", isPing);
              }
            }
            delay(10);
          }
        }
        Serial.println("OSC confirm"); //<-------Serial print
        delay(1000);
        Udp.stop();
        Udp.begin(NETPwrCtrl_inPort);
        Udp.beginPacket(NETPwrIP, NETPwrCtrl_outPort);
        Udp.write("Sw");
        Udp.write(0b10000);
        Udp.write("user1");
        Udp.write("1234");
        Udp.write(0x0D);
        Udp.write(0x0A);
        Udp.endPacket();
        Udp.stop();
        Serial.println("NETPwrCtrl OFF"); //<-------Serial print
        onoff = 0;
        digitalWrite(statLED, LOW);
      }
    }
  }
}
