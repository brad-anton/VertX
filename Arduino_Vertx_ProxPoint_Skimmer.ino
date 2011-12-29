/* HID ProxPoint Skimmer
*  Brad Antoniewicz
*  foundstone
*
* This is a prototype for a skimmer that sits between a
* HID ProxPoint reader and a VertX Controller
* 
* It will take data recieved from the Reader, print it 
* and send it to the controller
* It will also take data recieved from the Serial interface
* on the Arduino (in Binary) and send it to the controller
* 
* Arduino Pinouts:
*        2 - ProxPoint Green (DATA 0)
*        3 - ProxPoint White (DATA 1)
*        8 - Output to VertX Data 0 (Green)
*        9 - Output to VertX Data 1 (White)
*        5V - To ProxPoint Red
*        GND - To ProxPoint Black
*        GND - To VertX Ground (Black)
*
*  All of the Remaining ProxPoint wires can be plugged into the VertX
*  or left out.
*
* 26-Bit Wiegant Format: 
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
unsigned int cardValArray[CARD_LEN] = {0} ;
unsigned int InputCardValArray[CARD_LEN] = {7};

int toVertX_D0 = 8; // output DATA0 (Green) to Controller
int toVertX_D1 = 9; // output DATA1 (White) to Controller

int incomingByte = 0; 
int sInputCount = 0;

/*

  This code is for processing data from the reader
  */
volatile unsigned long reader1 = 0;
volatile int reader1Count = 0;

void reader1One(void) {
  reader1Count++;
  reader1 = reader1 << 1;
  reader1 |= 1;
  //processInput(49);
}

void reader1Zero(void) {
  reader1Count++;
  reader1 = reader1 << 1;
  //processInput(49);
}

void isr(int action) {
  if ( action == 1 ) { // Start
    /* Crazy People Timing and ISR -- Thanks Mike Cook! */
      attachInterrupt(0, reader1Zero, FALLING);//DATA0 to pin 2
      attachInterrupt(1, reader1One, FALLING); //DATA1 to pin 3
      delay(10); 
  } else { // Stop
      detachInterrupt(0);
      detachInterrupt(1);
  }
     
}

// END Reader In

void resetState() {
  Serial.println("[+] Reseting State...");
  delay(10);
  Serial.flush();
  sInputCount=0;
  for(short x=0; x<CARD_LEN; x++) {
    cardValArray[x] = 9;
  }
}

void writeCard(int inputmethod) {
  
  if ( inputmethod == 0 ) {
    Serial.println("[-] Sending Manual Input:");
    for (short x=0; x<CARD_LEN; x++) {
      if ( cardValArray[x] == 1 ) {
        Serial.print("1"); 
        digitalWrite(toVertX_D1, LOW);
        delayMicroseconds(34);
        digitalWrite(toVertX_D1, HIGH);
      } else if ( cardValArray[x] == 0 ) {
        Serial.print("0");
        digitalWrite(toVertX_D0, LOW);
        delayMicroseconds(34);
        digitalWrite(toVertX_D0, HIGH);
      }
      delay(2);
    }
    Serial.println();
  } else if ( inputmethod == 1 ) {
   Serial.println("[-] Sending Reader Input:");
    for (short x=0; x<CARD_LEN; x++) {
      if ( InputCardValArray[x] == 1 ) {
        Serial.print("1"); 
        digitalWrite(toVertX_D1, LOW);
        delayMicroseconds(34);
        digitalWrite(toVertX_D1, HIGH);
      } else if ( InputCardValArray[x] == 0 ) {
        Serial.print("0");
        digitalWrite(toVertX_D0, LOW);
        delayMicroseconds(34);
        digitalWrite(toVertX_D0, HIGH);
      }
      delay(2);
    }
    Serial.println();
    
  } else {
      Serial.println("[!] Not Sending Data - Something is wrong!");
  }
}
void processReaderData() {
 for(short i=1; i<=CARD_LEN; i++) {
   if (bitRead(reader1,i-1) == 0) {
      InputCardValArray[CARD_LEN-i] = 0;
   } else if (bitRead(reader1,i-1) == 1) {
     InputCardValArray[CARD_LEN-i] = 1;
   } else {
       Serial.println("[!] Something odd happened when processing reader data!");
   }
 }
}

void processInput(int byte) {
   if (byte == 49) { // DECIMAL 1 
     cardValArray[sInputCount] = 1;
     sInputCount++; 
  } else if (byte == 48) { // DECIMAL 0
     cardValArray[sInputCount] = 0;
     sInputCount++; 
  } else {
      Serial.println("[!] Recieved invalid Value!");
      resetState();
     }
}

void setup()
{
  Serial.begin(38400);
   /* This stuff is for the Door Sensor LED */
  pinMode(13, OUTPUT); 
  pinMode(11, INPUT); // Reader 1
  pinMode(12, INPUT); // Reader 2
  // End Door Sensor Stuff
  
  /* Reader In - */
  isr(1);

   for(int i = 2; i<4; i++){
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH); // enable internal pull up causing a one
    digitalWrite(i, LOW); // disable internal pull up causing zero and thus an interrupt
    pinMode(i, INPUT);
    digitalWrite(i, HIGH); // enable internal pull up
  }
  delay(10);
  reader1 = 0;
  reader1Count = 0;

  //End Reader In

  pinMode(toVertX_D0,OUTPUT);
  digitalWrite(toVertX_D0, HIGH);

  pinMode(toVertX_D1,OUTPUT);
  digitalWrite(toVertX_D1, HIGH);
  
  sInputCount = 0; 
 
  
  Serial.println("Please provide a tag value");
  Serial.println("For Binary input, it must be 26 bits long");
  Serial.println("No parity needs, just add two 0's to the beginning");
  Serial.println("e.g. 00001001100011111110010101");
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

  // End Door Sensor Stuff
  

  if (Serial.available() > 0 ) {
    
    incomingByte = Serial.read();
    Serial.print(".");
    delay(10);
    /* Serial.print(sInputCount);
    Serial.print(":");
    Serial.print(incomingByte, DEC);
    Serial.print(" ");
    */
    
    processInput(incomingByte);
    if ( ( sInputCount > 0 ) && ( sInputCount != CARD_LEN ) && ( Serial.peek() == -1 ) ) {
          Serial.println("\n[!] Invalid Input Length!");
          resetState();
        }
     }
  if (sInputCount == CARD_LEN) {
      Serial.println("");
          Serial.println("[-] Sending current Buffer");
          /* for(short x=0; x<CARD_LEN; x++) {
            Serial.print(" ");
            Serial.print(x);
            Serial.print("-");
            Serial.print(cardValArray[x]);
          }
          Serial.println("");
          */

          writeCard(0); 
          resetState();


     } else if (sInputCount > CARD_LEN) {
          Serial.println("[!] Sorry, looks like your input was too long.. ");
          resetState();

      }
       /* Reader IN */
       
if(reader1Count >= 26){
    Serial.println("");
    Serial.println("[+] Recieved input from Reader:");
    Serial.println("-------------------------------");
    Serial.print("R1: ");
    Serial.print(reader1,HEX);
    Serial.print(" SC:");
    int serialNumber=(reader1 >> 1) & 0x3fff;
    int siteCode= (reader1 >> 17) & 0x3ff;

    Serial.print(siteCode);
    Serial.print(" C: ");
    Serial.println(serialNumber);
    Serial.println(reader1,BIN);
    processReaderData();
    /* Serial.println("From Array:");
   for(short x=0; x<CARD_LEN; x++) {
      Serial.print(InputCardValArray[x]);
     }
     Serial.println("");
     */
    Serial.println("[+] Sending input to Controller:");
     writeCard(1);
    Serial.println("-------------------------------");
 
    reader1 = 0;
    reader1Count = 0;
    sInputCount = 0;
}
   // END Reader IN
}

