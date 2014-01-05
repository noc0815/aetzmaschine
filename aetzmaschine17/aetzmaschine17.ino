/*
  
 
 keyPinValue: Werte vom Bedienelement
   O O O  Links  Auf  Rechts
   O O O  ESC    Ab   OK/Menue
   
   Leer = 1023
   1.Links = 91
   3.Auf = 326
   2.Rechts = 413
   5.ESC = 767
   4.Ab = 683
   6.OK = 510
 
 This Programm code is in the public domain.

 */

// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>
#include <Servo.h>
Servo myservo;
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(0, 1, 2,3,4,5);
const int numRows = 4;
const int numCols = 20;

//----- Variablen definieren
int tmp =0;
int keyPinValue=0;
int lastKey=0;
int motorStatus=0;
int motorDownFlag=0;
int motorUpFlag=0;
int lflStatus=0;
int motorSoll=0;
int motorIst=0;
double tempSoll=50;
double tempIst=0;
int pos=0; // servo position
int heizungFlag=0;
int heizungLedStatus=0;
int heizungAutoMode=0;
int servoStatus=0;
double tpp=0;
String keyString= " ";

//-------pins definieren
int Display_Led=6;
int keyPin=A0;
int ledGruen2=A1;
int ledGruen1=A2;
int ledRot=9;
int ledGelb=13;
int motorPin=11;
int hallPin=A3;
int heizungPin=8;
int piezoPin=12;

unsigned long lflTime=0;
unsigned long motorDownTime=0;
unsigned long motorUpTime=0;
unsigned long tempTime=0;
unsigned long laufZeit=0;
unsigned long heizungLedTime=0;

void setup() {
  // Initialisierung 
  delay (500); 
  lcd.begin(numCols,numRows); 
  lcd.clear();
  Wire.begin(); // join i2c bus (address optional for master)
  myservo.attach(7);
  pinMode(piezoPin,OUTPUT);
  digitalWrite(piezoPin,LOW);
  pinMode(hallPin,INPUT_PULLUP); //setzt Eingang für Hallsensor mit Pullup widerstand
  pinMode(heizungPin,OUTPUT);
  digitalWrite(heizungPin,LOW);
  pinMode(motorPin,OUTPUT);
  digitalWrite(motorPin,LOW);
  pinMode(ledGruen1, OUTPUT);  
  digitalWrite(ledGruen1,LOW);
  pinMode(ledGruen2, OUTPUT);  
  digitalWrite(ledGruen2,LOW);
  pinMode(ledRot, OUTPUT);  
  digitalWrite(ledRot,LOW);
  pinMode(ledGelb, OUTPUT);  
  digitalWrite(ledGelb,LOW);
  pinMode(Display_Led, OUTPUT);  
  digitalWrite(Display_Led,HIGH);
  
  lflTime=millis();
  tempTime=millis();
  // Start Init 
  lcd.setCursor(2,0);
  lcd.print("AetzMaschine 1.0");
  lcd.setCursor(2,1);
  lcd.print("www.xrcontrol.de");
  lcd.setCursor(3,2);
  lcd.print("noc0815@gmail.com");
  lcd.setCursor(4,3);
  lcd.print("V221113.2300");
  delay (2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Motor Ist: 0/120");
  lcd.setCursor(0,1);
  lcd.print("Mode: Manuell");
  digitalWrite(7,LOW);
}

void loop() {
  if(tempTime-millis()>1000){
    leseTemp();
    lcd.setCursor(0,2);
    lcd.print("Temp Ist:");
    lcd.print(tempIst);
    lcd.print("/");
    lcd.print(tempSoll);
    tempTime=millis();  
  }
  setLfl(); //Lauflicht abarbeiten 
  readKey();
  showKeyResult();  
  diverseDinge();
  
  
  lcd.setCursor(0,3);
  lcd.print("Run:");
  lcd.print((millis()-laufZeit)/1000); 
  lcd.print("    "); 
  
} //viod loop  

void signal(){
  digitalWrite(piezoPin,HIGH);
      delay(100);
      digitalWrite(piezoPin,LOW);  
}//void signal

void leseTemp(){
 
 Wire.requestFrom(B01001111, 2); // request 2 byte from address 1001000
 while(Wire.available()){
   int temp = Wire.read(); // Read the first octet
   int lsb = Wire.read();	// Read the second octet
   tmp=temp << 1; // Vorzeichenbit entfernen, verbliebener Wert ist nun doppelt so groß
   lsb = (lsb & 0x80 ) >> 7; // nun ist lsb = 0 oder 1
   if (tmp < 0x80) { // Positiver Wert?
      tpp=(tmp + lsb)/2; // positiver Wert
      tempIst=tpp;
   }  
   else {
     tpp=(tmp + lsb)/2 - 128; // negativer Wert
     tempIst=tpp;
   }   
  } //while wire.
} //void leseTemp

void diverseDinge(){
  //Motor runter fahren
  if(motorDownFlag==1 and millis()-motorDownTime>200){
    motorIst=motorIst-5;
    analogWrite(motorPin,motorIst);
    lcd.setCursor(0,0);
    lcd.print("Motor: Ist:");
    lcd.print(motorIst);
    lcd.print("/");
    lcd.print(motorSoll);
    lcd.print("   ");
    motorDownTime=millis();
    if(motorSoll==motorIst){
      motorDownFlag=0;
      digitalWrite(ledGelb,LOW);
    } //if motorSoll
  } //if motorUpFlag
  //------------  
  //Motor hoch fahren
  if(motorUpFlag==1 and millis()-motorUpTime>200){
    motorIst=motorIst+5;
    analogWrite(motorPin,motorIst);
    lcd.setCursor(0,0);
    lcd.print("Motor: Ist:");
    lcd.print(motorIst);
    lcd.print("/");
    lcd.print(motorSoll);
    lcd.print("   ");
    motorUpTime=millis();
    if(motorSoll==motorIst){
      motorUpFlag=0;
      digitalWrite(ledGelb,LOW);
    } //if motorSoll
  } //if motorUpFlag
  
  if(heizungFlag==1 and (millis()-heizungLedTime)>50){
    if(heizungLedStatus==1 ){
      digitalWrite(ledRot,LOW);
      heizungLedStatus=0;
      heizungLedTime=millis();
    }
    else{
      digitalWrite(ledRot,HIGH);
      heizungLedStatus=1;
      heizungLedTime=millis();
    } //if heizungLedStatus
  } //if heizungFlag
  
  if(heizungAutoMode==1){                    //Temperatur Steuerung
    if(tempIst>tempSoll and heizungFlag==1){
     heizungAus();
    } //if tempSoll==tempIst
    if(tempIst<tempSoll and heizungFlag==0){
      heizungAn();
    } //if tempist<tempSoll 
  } //if heizungAutoMode
} //end void diverseDinge


void setLfl(){
  
  if(millis()-lflTime<75){
    return;
  } //end if
  
  lflTime=millis();
  lflStatus++;
  
  if(lflStatus==30){
    lflStatus=0;
  } //end iff
  
  switch (lflStatus) {
    case 0:
      digitalWrite(ledGruen1,LOW);
      digitalWrite(ledGruen2,LOW);
      break;
    case 1:
      digitalWrite(ledGruen1,HIGH);
      digitalWrite(ledGruen2,LOW);
      break;
    case 2:
      digitalWrite(ledGruen1,HIGH);
      digitalWrite(ledGruen2,HIGH);
      break;
    case 3:
      digitalWrite(ledGruen1,LOW);
      digitalWrite(ledGruen2,HIGH);
      break;
    default:
      digitalWrite(ledGruen1,LOW);
      digitalWrite(ledGruen2,LOW);
      break;
  } //end switch
} //end void setlfl

void showKeyResult(){
   
   if(keyPinValue==0){
     return;
   }
      
   switch (keyPinValue) {
    case 0:
      //lcd.print("No Key");
      break;
    case 1:  //auf
      servoup();
      break;
    case 2:  //ab
       servodown(); 
      
      break;
    case 3: //key links
      heizungAutoMode=1;
      heizungAn();
      lcd.setCursor(0,1);
      lcd.print("Mode: Automatik     ");
      break;
    case 4: //key rechts
      heizungAutoMode=0;
      heizungAus();
      lcd.setCursor(0,1);
      lcd.print("Mode: Manuell       ");
      break;
    case 5: //key ESC
       digitalWrite(ledGelb,HIGH);
       if(motorUpFlag==0 and motorDownFlag==0){
        motorDownFlag=1;
        motorSoll=0;
        motorDownTime=millis();
      } //if motorUpFlag
       motorStatus=1;
      break;
    case 6: //key ok
       digitalWrite(ledGelb,HIGH);
       if(motorUpFlag==0 and motorDownFlag==0){
        motorUpFlag=1;
        motorSoll=160;
        motorUpTime=millis();
        laufZeit=millis();
      } //if motorUpFlag
       break;
  } //end switch
  
} //end void showKeyResult
  

void heizungAn(){
 heizungFlag=1;
 digitalWrite(heizungPin,HIGH); 
 signal();
}

void heizungAus(){
  heizungFlag=0;
  digitalWrite(heizungPin,LOW);
  digitalWrite(ledRot,LOW);
  signal();
  delay(100);
  signal();  
}

void readKey(){
  keyPinValue=analogRead(keyPin);
  if(keyPinValue>1000){ 
    keyPinValue=0;
    keyString="---";
    return;
  } //end if keyPinValue
  if( keyPinValue >= 80 and keyPinValue <= 101 ){
    keyPinValue=3;
    keyString="W  ";
    delay(50);
    wait4keyFree();
    return;
  }//end if keypin1
   if( keyPinValue >= 316 and keyPinValue <= 336 ){
    keyPinValue=1;
    keyString="N  ";
    delay(50);
    wait4keyFree();
    return;
  }//end if keypin3
   if( keyPinValue >= 403 and keyPinValue <= 423 ){
    keyPinValue=4;
    keyString="O  ";
    delay(50);
    wait4keyFree();
    return;
  }//end if keypin2
   if( keyPinValue >= 757 and keyPinValue <= 777 ){
    keyPinValue=5;
    keyString="Esc";
    delay(50);
    wait4keyFree();
    return;
  }//end if keypin5
   if( keyPinValue >= 673 and keyPinValue <= 693 ){
    keyPinValue=2;
    keyString="S  ";
    delay(50);
    wait4keyFree();
    return;
  }//end if keypin4
   if( keyPinValue >= 500 and keyPinValue <= 520 ){
    keyPinValue=6;
    keyString="OK ";
    delay(50);
    wait4keyFree();
    return;
  }//end if keypin6
  
  return;  
    
} // end void readKey()

void wait4keyFree(){    //wartet  bis taste wieder losgelassen
  
  do {
    tmp=analogRead(keyPin);
     delay(50); 
  } while (tmp < 1000);
  
} //end void wait4keyFree

void servoup(){
  servoStatus=3;
  for(pos = 90; pos < 180; pos += 1)  // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
  servoStatus=1;
}

void servodown(){
  servoStatus=3;
  for(pos = 180; pos>=90; pos-=1)  // goes from 0 degrees to 180 degrees 
  {                                  // in steps of 1 degree 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
  digitalWrite(7,LOW);
  servoStatus=0;
}
