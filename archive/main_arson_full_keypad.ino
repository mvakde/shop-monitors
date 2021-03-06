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
byte keypadrows[4] = {19,18,17,6};

//global constants
const byte timerperiod = 60; //We will sound alarm after these many Seconds
char keymatrix[4][3] = {
    {'1','2','3'},
    {'4','5','6'},
    {'7','8','9'},
    {'*','0','#'}
};
Keypad keypad1 = Keypad( makeKeymap(keymatrix), keypadrows, keypadcols, 4, 3 );

//global variables
unsigned long int input_string = 1; //User entered string
char lastchar = '\0'; //Last character entered by user, default is \0 i.e. no character
unsigned long int correctpwd = 11234;//Correct pwd initialized as '1234'
volatile uint8_t current_count = timerperiod;//Current count of countdown timer in seconds
bool entered = false;
bool pwd_chng = false;
bool pwd_default = true;

//global state variables
volatile bool fire = false;
volatile bool burglar = false;
volatile bool armed = false;
volatile bool interruptedT1 = false;

//EEPROM addresses
const int pwd_address = 0;
const int flag_address = pwd_address + sizeof(correctpwd);

//Setup
void setup() {
  lcd.begin(16, 2);
  attachInterrupt(digitalPinToInterrupt(PIRpin), PIRisr, FALLING);
  attachInterrupt(digitalPinToInterrupt(Firepin), TEMPisr, FALLING);
  Timer1.initialize(1000000);//A second long timer
  Timer1.attachInterrupt(TIMERONEisr);
  Timer1.stop();
  EEPROM.get(flag_address,pwd_default);
  if(!pwd_default) EEPROM.get(pwd_address,correctpwd);//Load the right password
}

//ISRs
void PIRisr(){
  if(armed) burglar = true;
}
  
void TEMPisr(){
  fire = true;
}
  
void TIMERONEisr(){
  current_count--;
  interruptedT1=true;
}

//buzzer functions
void burglar_alarm(){
  tone(buzzerpin,1000);
}

void fire_alarm(){
  tone(buzzerpin,700,500);
}

void arson_alarm(){
  tone(buzzerpin,600,500);
}

//correctpwd functions
void enter_pwd() {
  byte failure = 0;
  byte max_fail = 3;
  bool reenter = false;
  current_count = timerperiod;
  entered = false;
  input_string = 1;
  clear_lcd();
  lcd.setCursor(0,0);
  lcd.print("Time Left:   s  ");
  Timer1.resume();
  while(true){
    if(fire) return;    //Fire is a greater danger than pesky thieves
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
      lcd.print("Cops called!    ");
      lcd.setCursor(0,1);
      lcd.print("                ");
      while(true){
        burglar_alarm();//Rings the alarm forever unless fire detected, then the fire brigade is also called
        if(fire) arson();
      }
    }
    if(input_string > 1 && reenter){
      lcd.setCursor(0,1);
      lcd.print("                ");
      reenter = false;
    }
    if(input_string > 1 && !entered){
      lcd.setCursor(0,1);
      print_input(input_string);
    }
    if(entered && input_string == correctpwd){
      clear_lcd();
      lcd.setCursor(0,0);
      lcd.print("All safe :)      ");
      burglar = false;
      armed = false;
      break;
    }
    if(entered && input_string != correctpwd && failure < max_fail){
      lcd.setCursor(0,1);
      lcd.print("Wrong pwd!      ");
      failure++;
      input_string = 1;
      entered = false;
      reenter = true;
    }
    if(entered && input_string != correctpwd && failure >= max_fail) current_count = 0;
  }
}

void change_pwd(){
  input_string = 1;
  clear_lcd();
  lcd.setCursor(0,0);
  lcd.print("Enter old pwd   ");
  entered = false;
  while(true){
    if(fire) return;//Fire safety is more important
    readKeypad(keypad1);
    lcd.setCursor(0,1);
    if(input_string > 1) print_input(input_string);
    if(entered && correctpwd == input_string){
      input_string = 1;
      clear_lcd();
      break;
    }
    if(entered && correctpwd != input_string){
      clear_lcd();
      return;
    }
  }
  lcd.setCursor(0,0);
  lcd.print("Enter new pwd   ");
  entered = false;
  while(true){
    if(fire) return;//Fire safety is more important
    readKeypad(keypad1);
    lcd.setCursor(0,1);
    if(input_string > 1) print_input(input_string);
    if(entered){
      correctpwd = input_string;
      EEPROM.put(pwd_address,correctpwd);
      if(pwd_default){
        pwd_default = false;
        EEPROM.put(flag_address,pwd_default); 
      }
      lcd.setCursor(0,0);
      lcd.print("Pwd changed     ");
      lcd.setCursor(0,1);
      lcd.print("                ");
      input_string = 1;
      pwd_chng = false;
      break;
    }
  }
}

void arson(){
  clear_lcd();
  lcd.setCursor(0,0);
  lcd.print("Fire + burglar!!Emrgncy srvces called");
  noTone(buzzerpin);
  while(true) arson_alarm();
}
void fire_here(){
  Timer1.resume();
  bool reenter = false;//Passwords can be re-entered any number of times
  input_string = 1;
  clear_lcd();
  lcd.setCursor(0,0);
  lcd.print("Fire here!!     ");
  entered = false;
  while(fire){
    if (interruptedT1) {interruptedT1 =false; fire_alarm();}
    if(digitalRead(Firepin) == HIGH){//Fire not detected now
      readKeypad(keypad1);
      if(input_string > 1 && reenter){
        lcd.setCursor(0,1);
        lcd.print("                ");
        reenter = false;
      }
      if(input_string > 1){
        lcd.setCursor(0,1);
        print_input(input_string);
      }
      if(input_string == correctpwd && entered){
        clear_lcd();
        lcd.setCursor(0,0);
        lcd.print("All safe :)      ");
        fire = false;
      }
      if(input_string != correctpwd && entered){
        lcd.setCursor(0,1);
        lcd.print("Re-enter pwd    ");
        entered = false;
        reenter = true;
        input_string = 1;
      }
    }
  }
  Timer1.stop();
}

void arm_detector(){
  clear_lcd();
  current_count = 10;
  lcd.setCursor(0,0);
  lcd.print("Arming in   s   ");
  Timer1.resume();
  while(true){
    if(fire) return;//Why arm during a fire?
    if(current_count > 9){
      lcd.setCursor(10,0);
      lcd.print(current_count);
    }
    else if(current_count > 0){
      lcd.setCursor(10,0);
      lcd.print(" "); 
      lcd.setCursor(11,0);
      lcd.print(current_count);
    }
    else{
      Timer1.stop();
      lcd.setCursor(0,0);
      lcd.print("Armed!          ");
      break;
    }
  }
  armed = true;
  current_count = 60;
  return;
}

void clear_lcd(){
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("                ");
}

//Keyboard functions
void print_input(unsigned long int input){
  String s = String(input).substring(1);
  lcd.print(s);
}

void clear_string(){
  input_string = 1;
  lcd.setCursor(0,1);
  lcd.print("                ");
}

void force_append(char character){
  input_string = input_string*10 + int(character) - int('0');
}

void readKeypad(Keypad k){
  char key = k.getKey();
  if (lastchar=='\0'){
    switch(key){
      case '#':
        clear_string();
        if(!burglar && !fire) pwd_chng = true;
        break;
      case '*':
        entered = true;
        break;
      case '\0': //Null character, no key was pressed
        break; //do nothing 
      default:
        force_append(key);
    }
  }
  lastchar = key;
}
  
//Main loop
void loop() {
  if(burglar && armed) enter_pwd();
  readKeypad(keypad1);
  if(pwd_chng && !burglar && !fire) change_pwd(); 
  if(fire) fire_here();
  if(digitalRead(armpin) == HIGH){
    arm_detector();
  }
}
