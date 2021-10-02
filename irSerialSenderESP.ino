
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "wifi_password.h" //Please comment this out, for storing wifi password in other file.

#ifndef STASSID
#define STASSID "YOUR_WIFI_SSID"
#define STAPSK  "YOUR_WIFI_PASSWORD"
#endif

#define MAXSIZE  8192

unsigned int localPort = 8888;      // local port to listen on
IPAddress ip(192,168,1,31);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

// buffers for receiving and sending data
char packetBuffer[MAXSIZE + 1]; //buffer to hold incoming packet,
char  ReplyBuffer[] = "acknowledged\r\n";       // a string to send back

WiFiUDP Udp;

// See the full tutorial at http://www.ladyada.net/learn/sensors/ir.html
// this code is public domain, please enjoy!

//#define IRpin_PIN      PIND
#define IRpin          16

// the maximum pulse we'll listen for - 65 milliseconds is a long time
#define MAXPULSE 65000
 
// what our timing resolution should be, larger is better
// as its more 'precise' - but too large and you wont get
// accurate timing
#define RESOLUTION 10 
 
// we will store up to 100 pulse pairs (this is -a lot-)
uint16_t pulses[100][2];  // pair is high and low pulse 
uint8_t currentpulse = 0; // index for pulses we're storing
uint8_t maxPulseCount = 99;
 
int IRledPin =  5;    // LED connected to digital pin 13
int IndLedPin = 2;

String buff;
int count = 0;
int data[200];
bool receiveMode;

// The setup() method runs once, when the sketch starts
 
void setup()   {                
  // initialize the IR digital pin as an output:
  pinMode(IRledPin, OUTPUT);    
  pinMode(IndLedPin, OUTPUT);  
  Serial.begin(115200);
  Serial.println("Ready to send.");

  //WiFi
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  WiFi.config(ip,gateway,subnet);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  Serial.printf("UDP server on port %d\n", localPort);
  Udp.begin(localPort);
  digitalWrite(IndLedPin, HIGH);
}
 
void loop()                     
{
  //Serial Port
  if(receiveMode == false) {
    if(Serial.available() > 0) {
      char c = Serial.read();
      if(c == 'x') { //Send Command
        Serial.println(count);
        sendData();
        count = 0;
        clearData();
      } else if (c == ',') { //Delimiter
        data[count] = buff.toInt();
        buff = "";
        count++;
      } else if (c == 'r') { //Switch to Receive Mode
        Serial.println("Ready to decode IR!");
        receiveMode = true;
      } else {
        buff = buff + c;
      }
      //Serial.println((int)c);
    }
  } else { //Receive Mode
    receiveData();
  }

  //Network
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Serial.printf("Received packet of size %d from %s:%d\n    (to %s:%d, free heap = %d B)\n",
                  packetSize,
                  Udp.remoteIP().toString().c_str(), Udp.remotePort(),
                  Udp.destinationIP().toString().c_str(), Udp.localPort(),
                  ESP.getFreeHeap());

    // read the packet into packetBufffer
    int n = Udp.read(packetBuffer, MAXSIZE);
    packetBuffer[n] = 0;
    Serial.println("Contents:");
    Serial.println(packetBuffer);
    receivedUdp(packetBuffer);

    // send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
  }
}

void receivedUdp(char d[]) {
  for(int i = 0; i < MAXSIZE; i++) {
    char c = d[i];
      if(c == 'x') { //Send Command
        Serial.println(count);
        sendUdpInt(count);
        sendData();
        count = 0;
        clearData();
        break;
      } else if (c == ',') { //Delimiter
        data[count] = buff.toInt();
        buff = "";
        count++;
      } else if (c == 'r') { //Switch to Receive Mode
        Serial.println("Ready to decode IR!");
        sendUdp("Ready to decode IR!");
        receiveMode = true;
        break;
      } else {
        buff = buff + c;
      }
  }
}

void sendUdp(char d[]) {
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write(d);
  Udp.endPacket();
}

void sendUdpInt(int num) {
  char c[16];
  itoa(num, c, 10);
  sendUdp(c);
}

void writeUdpInt(int num) {
  char c[16];
  itoa(num, c, 10);
  Udp.write(c);
}

void clearData() {
  for(int i=0; i<200; i++) {
    data[i] = 0;
  }
}

void sendData() {
  for(int i=0;i<count;i++) {
    if(i%2 > 0){
      pulseIR(data[i], true);
    }else{
      //delayMicroseconds(data[i]);
      pulseIR(data[i], false); //for delay only
    }
  }
}

/*void receiveData() {
  uint16_t highpulse, lowpulse;  // temporary storage timing
  highpulse = lowpulse = 0; // start out with no pulse length
  
  while (digitalRead(IRpin)) { // this is too slow!
    //while (IRpin_PIN & (1 << IRpin)) {
     // pin is still HIGH
     // count off another few microseconds
     highpulse++;
     delayMicroseconds(RESOLUTION);

     if(highpulse > MAXPULSE && currentpulse == 0) return;
     
     // If the pulse is too long, we 'timed out' - either nothing
     // was received or the code is finished, so print what
     // we've grabbed so far, and then reset
     if ((highpulse >= MAXPULSE) && (currentpulse != 0)) {
      Serial.println("timeoutHigh");
       printpulses();
       currentpulse=0;
       return;
     }
  }
  // we didn't time out so lets stash the reading
  pulses[currentpulse][0] = highpulse;
 
  // same as above
  digitalWrite(IndLedPin, LOW);
  while (!digitalRead(IRpin)) {
  //while (! (IRpin_PIN & _BV(IRpin))) {
     // pin is still LOW
     lowpulse++;
     delayMicroseconds(RESOLUTION);
     
     if(lowpulse > MAXPULSE && currentpulse == 0) return;
     
     if ((lowpulse >= MAXPULSE)  && (currentpulse != 0)) {
      Serial.println("timeoutLow");
       printpulses();
       currentpulse=0;
       return;
     }
  }
  digitalWrite(IndLedPin, HIGH);
  pulses[currentpulse][1] = lowpulse;
 
  // we read one high-low pulse successfully, continue!
  currentpulse++;
  if(currentpulse > maxPulseCount) {
    currentpulse = 0;
    return;
  }
}*/

void receiveData() {
  uint16_t highpulse, lowpulse;  // temporary storage timing
  
  currentpulse = 0; 

  while (true) {
    highpulse = lowpulse = 0; // start out with no pulse length
    
    while (digitalRead(IRpin)) {
       // pin is still HIGH
       // count off another few microseconds
       highpulse++;
       delayMicroseconds(RESOLUTION - 2);
       if(highpulse > MAXPULSE && currentpulse == 0) return;
       
       // If the pulse is too long, we 'timed out' - either nothing
       // was received or the code is finished, so print what
       // we've grabbed so far, and then reset
       if ((highpulse >= MAXPULSE) && (currentpulse != 0)) {
        Serial.println("timeoutHigh");
         printpulses();
         currentpulse=0;
         return;
       }
    }
    // we didn't time out so lets stash the reading
    digitalWrite(IndLedPin, LOW);
    pulses[currentpulse][0] = highpulse;
   
    // same as above
    
    while (!digitalRead(IRpin)) {
    //while (! (IRpin_PIN & _BV(IRpin))) {
       // pin is still LOW
       lowpulse++;
       delayMicroseconds(RESOLUTION - 2);
       //if(lowpulse > MAXPULSE && currentpulse == 0) return;
       
       if ((lowpulse >= MAXPULSE)  && (currentpulse != 0)) {
        Serial.println("timeoutLow");
         printpulses();
         currentpulse=0;
         return;
       }
    }
    digitalWrite(IndLedPin, HIGH);
    pulses[currentpulse][1] = lowpulse;
    
    // we read one high-low pulse successfully, continue!
    currentpulse++;
    if(currentpulse > maxPulseCount) {
      currentpulse = 0;
      printpulses();
      return;
    }
  }
}
 
// This procedure sends a 38KHz pulse to the IRledPin 
// for a certain # of microseconds. We'll use this whenever we need to send codes
void pulseIR(long microsecs, bool ir) {
  // we'll count down from the number of microseconds we are told to wait
 
  cli();  // this turns off any background interrupts
  digitalWrite(IndLedPin, LOW);
  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
    if(ir) {
      digitalWrite(IRledPin, HIGH);  // this takes about 3 microseconds to happen
    }else{
      digitalWrite(IRledPin, LOW); //dummy
    }
   delayMicroseconds(10+2);         // hang out for 10 microseconds
   digitalWrite(IRledPin, LOW);   // this also takes about 3 microseconds
   delayMicroseconds(10+1);         // hang out for 10 microseconds
 
   // so 26 microseconds altogether
   microsecs -= 23+5;//26;
  }
  digitalWrite(IndLedPin, HIGH);
 
  sei();  // this turns them back on
}

void printpulses(void) {
  //sendUdp("\n\r\n\rReceived: \n\rOFF \tON");
  //sendUdpInt(currentpulse);
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write("[");
  for (uint8_t i = 0; i < currentpulse; i++) {   
    //Serial.print("delayMicroseconds(");
    writeUdpInt(pulses[i][0] * RESOLUTION); //OFF time
    Udp.write(",");
    writeUdpInt(pulses[i][1] * RESOLUTION); //ON time
    Udp.write(",");
  }
  Udp.write("x]");
  Udp.endPacket();
  receiveMode = false;
  //sendUdp("Ready to send.");
  
  Serial.println("\n\r\n\rReceived: \n\rOFF \tON");
  Serial.println(currentpulse, DEC);
  Serial.print("[");
  for (uint8_t i = 0; i < currentpulse; i++) {   
    //Serial.print("delayMicroseconds(");
    Serial.print(pulses[i][0] * RESOLUTION, DEC); //OFF time
    Serial.print(",");
    Serial.print(pulses[i][1] * RESOLUTION, DEC); //ON time
    Serial.print(",");
  }
  Serial.println("x]");
  receiveMode = false;
  Serial.println("Ready to send.");
}
