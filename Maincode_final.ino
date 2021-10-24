#include <Key.h>
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <TimerOne.h>


//Pin allocation
//const byte LCDpins[6] = {8,9,10,11,12,13};//LCD module for display
LiquidCrystal lcd(8,9,10,11,12,13);
const byte PIRpin = 2; //burglar sensor PIR read
const byte Firepin = 3; //temperature sensor read
const byte buzzerpin = 5; //write to buzzerpin to play sound
const byte armpin = 7; //push button to arm the burglar functionality
const byte TX = 1;//Serial comm or bluetooth
const byte RX = 0;//Serial comm or bluetooth
byte keypadcols[3] = {16,15,14};
byte keypadrows[3] = {19,18,17};

//global constants
const byte timerperiod = 60; //We will sound alarm after these many Seconds
char keymatrix[3][3] = {
    {'1','2','3'},
    {'4','5','6'},
    {'*','0','#'}
};
Keypad keypad1 = Keypad( makeKeymap(keymatrix), keypadrows, keypadcols, 3, 3 );

//global variables
String input_string = ""; //User entered string
char lastchar = '\0'; //Last character entered by user, default is \0 i.e. no character
String correctpwd = "";//Correct correctpwd
volatile uint8_t current_count = 60;//Current count of countdown timer in seconds
bool entered = false;
bool pwd_chng = false;

//global state variables
volatile bool fire = false;
volatile bool burglar = false;
volatile bool armed = false;
volatile bool count_change = false;

//Setup
void setup() {
  lcd.begin(16, 2);
  Timer1.initialize(1000000);//A second long timer
  Timer1.attachInterrupt(TIMERONEisr);
  Timer1.stop();
  EEPROM.get(0,correctpwd);
  Serial.begin(9600);
}

//ISRs
void PIRisr(){
  armed = false;
  burglar = true;
}
  
void TEMPisr(){
  fire = true;
}
  
void TIMERONEisr(){
  current_count--;
  count_change = true;
}

//buzzer functions
void burglaralarm(){
  tone(buzzerpin,1000);
}

void firealarm(){
  tone(buzzerpin,700,500);
  delay(1000);
}

//correctpwd functions
void enter_pwd() {
  lcd.print("Time Left:   s");
  Timer1.resume();
  while(true){
    readKeypad(keypad1);
    if(current_count > 9){
      lcd.setCursor(11,0);
      lcd.print(current_count);
    }
    else if(current_count > 0){
      lcd.setCursor(11,0);
      lcd.print(" "); 
      lcd.setCursor(12,0);
      lcd.print(current_count);
    }
    else{
      Timer1.detachInterrupt();
      lcd.setCursor(0,0);
      lcd.print("Game Over!    ");
      lcd.setCursor(0,1);
      lcd.print("              ");
      while(true) burglaralarm();//Rings the alarm forever
    }
    if(input_string != correctpwd && !entered && current_count>0){
      lcd.setCursor(0,1);
      lcd.print(input_string);
    }
    if(entered && input_string == correctpwd && current_count>0){
      lcd.setCursor(0,0);
      lcd.print("All safe :)    ");
      lcd.setCursor(0,1);
      lcd.print("              ");
      burglar = false;
      break;
    }
    if(entered && input_string != correctpwd && current_count>0){
      current_count = 0;
    }
  }
}

void change_pwd(){
  lcd.setCursor(0,0);
  lcd.print("Enter new pwd ");
  entered = false;
  while(true){
    readKeypad(keypad1);
    lcd.setCursor(0,1);
    lcd.print(input_string);
    if(entered){
      correctpwd = input_string;
      EEPROM.put(0,correctpwd);
      lcd.print("Pwd changed   ");
      input_string = "";
      break;
    }
  }
}


//Keyboard functions
void clear_string(){
  input_string = "";
}

void force_append(char character){
  input_string.concat(character);
}

void readKeypad(Keypad k){
  char key = k.getKey();
  if (lastchar=='\0'){
    switch(key){
      case '#':
        clear_string();
        if(!burglar) pwd_chng = true;
        break;
      case '*':
        entered = true;
        break;
//    case '\0': //Null character, no key was pressed
//      break; //do nothing 
      default:
        force_append(key);
    }
  }
  lastchar = key;
}
  
//Main loop
void loop() {
  if(burglar && !armed) enter_pwd();
  readKeypad(keypad1);
  if(pwd_chng && !burglar) change_pwd();      
}