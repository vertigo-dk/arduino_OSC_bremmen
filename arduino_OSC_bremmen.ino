////////////////////////////////////////////////////
//              INCLUDE                           //
////////////////////////////////////////////////////
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
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
int timeOut = 0;
////////////////////////////////////////////////////
//            Ethernet
////////////////////////////////////////////////////
//UDP communication
EthernetUDP Udp;

//the self asignd IP
IPAddress ip(192, 168, 1, 61);
byte mac[6] = {0x90, 0xA2, 0xDA, 0x10, 0x38, 0x33};

//port numbers
const unsigned int QLabPort = 53000;
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

void displayError(int errorCode, int time) {
  if (errorCode >= 0) {
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if(i <= errorCode){
        digitalWrite(LED_PIN[i], HIGH);
      }else{
        digitalWrite(LED_PIN[i], LOW);
      }
    }
  }else {
    errorCode = -errorCode;
    for (int i = 0; i < NUM_BUTTONS; i++) {
      if(i <= errorCode){
        digitalWrite(LED_PIN[i], LOW);
      }else{
        digitalWrite(LED_PIN[i], HIGH);
      }
    }
  }
  delay(time);
}

void setup() {
  Serial.begin(9600);
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
  main:
  displayError(0, 1);
  if (onoff == 1) {
    boolean change = false;
    OSCBundle button;

    if (rotoStat != map(analogRead(A0)+4, 0, 900, 0, 9)) {
      if(map(analogRead(A0)+4, 0, 900, 0, 9) != 0) {
        rotoStat = map(analogRead(A0)+4, 0, 900, 0, 9);
        button.add("/rot").add((int)rotoStat);
        Serial.print("rotoStat   ");
        Serial.println(rotoStat);
        change = true;
      }
    }

    // Update the Bounce instances :
    for (int i = 0; i < NUM_BUTTONS; i++) {
      debouncer[i].update();
    }

    for (int i = 0; i < NUM_BUTTONS; i++) {
      if ( debouncer[i].fell() || debouncer[i].rose() ) {
        char str[9] = "/button/";
        str[8] = 48 + i;
        button.add(str).add((int)debouncer[i].read());
        Serial.println(debouncer[i].read());
        change = true;
      }
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
        displayError(1, 10);
        blink(100, 10);

        char UdpPacket[] = "net-PwrCtrl";
        Udp.begin(NETPwrCtrl_inPort);
        int packetSize = 0;
        timeOut = 0;
        while (memcmp(UdpPacket, "NET-PwrCtrl:NET-CONTROL", sizeof(UdpPacket)) != 0) {
          timeOut++;
          Udp.beginPacket(NETPwrIP, NETPwrCtrl_outPort);
          Udp.write("wer da?");
          Udp.write(0x0D);
          Udp.write(0x0A);
          Udp.endPacket();
          delay(10);
          packetSize = Udp.parsePacket();
          Udp.read(UdpPacket, sizeof(UdpPacket));
          if (timeOut > 20) { // if can't connect to relay
            Serial.println("can't connect to NETPwrCtrl"); //<-------Serial print
            displayError(2, 4000);
            goto main;
          }
        }
        Serial.println("NETPwrCtrl conected"); //<-------Serial print
        displayError(3, 10);
        //turn on the computer with the realy
        Serial.println("NETPwrCtrl PC on"); //<-------Serial print
        Udp.beginPacket(NETPwrIP, NETPwrCtrl_outPort);
        Udp.write("Sw");
        Udp.write(0b10000100);
        Udp.write("user1");
        Udp.write("1234");
        Udp.write(0x0D);
        Udp.write(0x0A);
        Udp.endPacket();

        blink(500, 60);

        Udp.begin(inPort);
        timeOut = 0;
        while(isConct != 1){
          OSCMessage msgOut("/ping");
          msgOut.add("powerON");
          Udp.beginPacket(outIp, QLabPort);
          msgOut.send(Udp); // send the bytes to the SLIP stream
          Udp.endPacket(); // mark the end of the OSC Packet
          Serial.println("sending OSC ping"); //<-------Serial print
          OSCMessage msgIn;
          digitalWrite(LED_BUILTIN, HIGH);
          delay(60);
          digitalWrite(LED_BUILTIN, LOW);
          delay(40);
          for (int i = 0; i < 10; i++) {
            int size;
            if( (size = Udp.parsePacket())>0) {
              while(size--) {
                msgIn.fill(Udp.read());
              }
              if(!msgIn.hasError()) {
                msgIn.route("/ping", isPing, 0);
              }
            }
            delay(10);
          }
          timeOut++;
          if (timeOut > 60) {
            Serial.println("ping timeOut"); //<-------Serial print
            displayError(4, 4000);
            goto main;
          }
        }
        Serial.println("OSC pong recived"); //<-------Serial print
        onoff = 1;
        digitalWrite(LED_BUILTIN, HIGH);
        displayError(5, 4000);

      }else{
        Serial.println("starting shutdown sequence");
        displayError(-5, 10);
        Serial.println("sending ping OFF");
        timeOut = 0;
        while(isConct != 1){
          OSCMessage msgOut("/ping");
          msgOut.add("powerOFF");
          Udp.beginPacket(outIp, QLabPort);
          msgOut.send(Udp); // send the bytes to the SLIP stream
          Udp.endPacket(); // mark the end of the OSC Packet
          OSCMessage msgIn;
          digitalWrite(LED_BUILTIN, HIGH);
          delay(70);
          digitalWrite(LED_BUILTIN, LOW);
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
          timeOut++;
          if (timeOut >= 60) {
            Serial.println("Ping timeOut"); //<-------Serial print
            displayError(-4, 4000);
          }

        }
        Serial.println("pong recived"); //<-------Serial print
        displayError(-3, 10);
        blink(500, 500); //480 sec. delay
        displayError(-2, 10);
        char UdpPacket[] = "net-PwrCtrl";
        Udp.begin(NETPwrCtrl_inPort);
        int packetSize = 0;
        int timeOut = 0;
        while (memcmp(UdpPacket, "NET-PwrCtrl:NET-CONTROL", sizeof(UdpPacket)) != 0) {
          timeOut++;
          Udp.beginPacket(NETPwrIP, NETPwrCtrl_outPort);
          Udp.write("wer da?");
          Udp.write(0x0D);
          Udp.write(0x0A);
          Udp.endPacket();
          delay(10);
          packetSize = Udp.parsePacket();
          Udp.read(UdpPacket, sizeof(UdpPacket));
          if (timeOut > 20) { // if can't connect to relay
            Serial.println("can't connect to NETPwrCtrl"); //<-------Serial print
            displayError(-1, 4000);
            blink(200, 4);
            goto main;
          }
        }
        Serial.println("connected to NETPwrCtrl"); //<-------Serial print
        Serial.println("turning off"); //<-------Serial print
        Udp.begin(NETPwrCtrl_inPort);
        Udp.beginPacket(NETPwrIP, NETPwrCtrl_outPort);
        Udp.write("Sw");
        Udp.write(0b10000000);
        Udp.write("user1");
        Udp.write("1234");
        Udp.write(0x0D);
        Udp.write(0x0A);
        Udp.endPacket();
        Udp.stop();
        Serial.println("NETPwrCtrl OFF"); //<-------Serial print
        onoff = 0;
        displayError(0, 4000);
      }
    }
  }
}
