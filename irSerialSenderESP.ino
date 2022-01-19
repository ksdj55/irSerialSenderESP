
#include <ESP8266WiFi.h>
#include "wifi_password.h" //Please comment this out, used for storing wifi password in other file.

#ifndef STASSID
#define STASSID "YOUR_WIFI_SSID"
#define STAPSK  "YOUR_WIFI_PASSWORD"
#endif

#define MAXSIZE  8192

unsigned int localPort = 8888;      //Local port to listen on
IPAddress ip(192,168,1,31);         //Fixed IP address
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WiFiServer server(localPort);
WiFiClient client;
bool isconnected = false;

// Modified from this IR code.
// See the full tutorial at http://www.ladyada.net/learn/sensors/ir.html
// this code is public domain, please enjoy!

//#define IRpin_PIN      PIND
#define IRpin          16  //IR Receiver Photo Transistor Pin

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
 
int IRledPin =  5;    //IR LED connected to digital pin GPIO5
int IndLedPin = 2;    //On-board indicicator LED pin

String buff;
int count = 0;
int data[200];
bool recordMode;

// The setup() method runs once, when the sketch starts
 
void setup()   {                
  // initialize the IR digital pin as an output:
  pinMode(IRledPin, OUTPUT);    
  pinMode(IndLedPin, OUTPUT);  
  pinMode(IRpin, INPUT);
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
  server.begin();
  digitalWrite(IndLedPin, HIGH);
}
 
void loop()                     
{
  //Serial Port
  if(recordMode == false) {
    if(Serial.available() > 0) {
      char c = Serial.read();
      receivedChar(c);
    }
  } else { //Record Mode
    record();
  }

  //Network
  // if there's data available, read a packet
    
    //TCP Network
    if(client) {
      if(client.connected()) {
        isconnected = true;
        while(client.available()>0) {
          char c = client.read();
          Serial.write(c);
          receivedChar(c);
        }
    }else{
      if(isconnected) {
        client.stop();
        Serial.println("Client Disconnected");
        isconnected = false;
      }
    }
  } else {
    client = server.available();
  }
}

void receivedChar(char c) {
  if(c == 'x') { //Send IR Command
        Serial.println(count);
        writeTcpInt(count);
        sendData();
        count = 0;
        clearData();
        //break;
      } else if (c == ',') { //Delimiter
        data[count] = buff.toInt();
        buff = "";
        count++;
      } else if (c == 'r') { //Switch to Record Mode
        Serial.println("Ready to decode IR!");
        sendTcp("Ready to decode IR!\n");
        recordMode = true; 
      } else {
        buff = buff + c;
      }
}

void sendTcp(char d[]) {
  if(client) {
    if(client.connected()) {
      client.write(d);
    }
  }
}

void writeTcpInt(int num) {
  char c[16];
  itoa(num, c, 10);
  sendTcp(c);
}

void clearData() {
  for(int i=0; i<200; i++) {
    data[i] = 0;
  }
}

void sendData() {
  for(int i=0;i<count;i++) {
    if(i%2 > 0){  //Odd number
      pulseIR(data[i], true);
    }else{
      //delayMicroseconds(data[i]);
      pulseIR(data[i], false); //for delay only
    }
  }
}

void record() {
  Serial.println("record()");
  uint16_t highpulse, lowpulse;  // temporary storage timing
  
  currentpulse = 0; 
  
  while (true) {
    highpulse = lowpulse = 0; // start out with no pulse length
    while (digitalRead(IRpin)) {
       // pin is still HIGH
       // count off another few microseconds
       highpulse++;
       delayMicroseconds(RESOLUTION - 2);
       if(highpulse > MAXPULSE && currentpulse == 0) {
          return;
       }
       
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
    digitalWrite(IndLedPin, LOW); //Light the on-board LED
    pulses[currentpulse][0] = highpulse;
   
    // same as above
    while (!digitalRead(IRpin)) {
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
  Serial.println("printpulses()");

  client.write("[");
  for (uint8_t i = 0; i < currentpulse; i++) {   
    writeTcpInt(pulses[i][0] * RESOLUTION); //OFF time
    client.write(",");
    writeTcpInt(pulses[i][1] * RESOLUTION); //ON time
    client.write(",");
  }
  client.write("x]\n");

  recordMode = false;
  //sendTcp("Ready to send.");
  
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
  recordMode = false;
  Serial.println("Ready to send.");
}
