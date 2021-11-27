#include <Arduino.h>
#include <Encoder.h>
#include <A4988.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

const byte IRled = 13;                // IR Output Signal to Anode of IR LED 
// Cathode of IR LED to ground through a 150 Ohm Resistor

int start[]= {0,0,0,1,0,0,1,0,1,0,1,1,1,0,0,0,1,1,1,1};

// Pin mapping for the display:
const byte LCD_RS = 12;
const byte LCD_E = 11;
const byte LCD_D4 = 8;
const byte LCD_D5 = 7;
const byte LCD_D6 = 6;
const byte LCD_D7 = 5;
//LCD R/W pin to ground
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

const byte LCD_CONTRAST = 10;
const byte LCD_BACKLIGHT = 9;

const byte HOME_SWITCH = 18;
bool homeSwitchState = false;

const byte ENC_SW = 14;
const byte ENC_A = 2;
const byte ENC_B = 3;
int16_t encPos = 0;

const byte STEPPER_DIR = 19;
const byte STEPPER_STEP = 17;
const byte STEPPER_EN = 4;
const uint8_t STEPPER_STEPS_PER_REV = 200;
const uint8_t STEPPER_MICROSTEPS_PER_STEP = 16;
bool stepperIsEnabled = true;
uint8_t currentPos = 0;

const float SCREW_PITCH_IN_MM = 1.25;
const float GEAR_RATIO = 22.0/12.0;
const float STEPS_PER_MM = (STEPPER_STEPS_PER_REV * STEPPER_MICROSTEPS_PER_STEP * GEAR_RATIO) / SCREW_PITCH_IN_MM;

Encoder enc(ENC_A, ENC_B);

A4988 stepper(STEPPER_STEPS_PER_REV, STEPPER_DIR, STEPPER_STEP, STEPPER_EN);


void setup() {
  Serial.begin(9600);
  Serial.println("START");

  uint16_t ScreenBrightnes;
  uint16_t ScreenContrast;

  uint16_t MotorSpeed;
  uint16_t MotorAccel;

  EEPROM.get(13*2, ScreenBrightnes);
  EEPROM.get(14*2, ScreenContrast);
  
  EEPROM.get(15*2, MotorSpeed);
  EEPROM.get(16*2, MotorAccel);
  
  pinMode(HOME_SWITCH, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(IRled, OUTPUT);

  lcd.begin(16, 2);
  lcd.setCursor(5,0);
  lcd.print("Slider");
  lcd.setCursor(2,1);
  lcd.print("Version 1.0");

  
  pinMode(LCD_CONTRAST, OUTPUT);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  analogWrite(LCD_CONTRAST, ScreenContrast);
  analogWrite(LCD_BACKLIGHT, ScreenBrightnes);

  stepper.begin(200, STEPPER_MICROSTEPS_PER_STEP);
  stepper.setEnableActiveState(LOW);
  stepper.enable();
  stepper.setSpeedProfile(stepper.LINEAR_SPEED, 2000 ,2000);
  Serial.println(stepper.getRPM());
  Serial.println(stepper.getAcceleration());
  stepper.startMove(-1000000);     // in microsteps
}

void loop() {
  static bool lastHomeSwitchState = false;
  static bool lastEncSwitchState = false;
  static bool Cursor = true;
  static int16_t lastEncPos = 0;
  static uint8_t MenuItemsAmount = 0;
  static uint8_t MenuCursorLine = 0;
  static uint8_t MenuType = 255;
  static uint8_t MenuLine[7];
  const uint8_t MenuElements = 23;
  static uint32_t HomeDistance;

  homeSwitchState = digitalRead(HOME_SWITCH);
  encPos = enc.read();

  struct mama
  {
    String Name;
    uint8_t Id;
    uint8_t Type; //0 = SubMenu 1 = Data 2 = action
    uint8_t SubMenu; 
    uint8_t Action;
    uint16_t Data;
    float Multiplikation;
    uint16_t Limit;
  };
  struct mama Menu[MenuElements];

  //Database
  Menu[0].Id = 0;
  Menu[0].Name = "Main menu";
  Menu[0].Type = 0;
  Menu[0].SubMenu = 0;
  
  Menu[1].Id = 1;
  Menu[1].Name = "Work";
  Menu[1].Type = 0;
  Menu[1].SubMenu = 1;

  Menu[2].Id = 2;
  Menu[2].Name = "Hand jog";
  Menu[2].Type = 0;
  Menu[2].SubMenu = 3;

  Menu[3].Id = 3;
  Menu[3].Name = "Setings";
  Menu[3].Type = 0;
  Menu[3].SubMenu = 2;

  Menu[4].Id = 4;
  Menu[4].Name = "Travel mm";
  Menu[4].Type = 1;
  Menu[4].Multiplikation = 1;
  Menu[4].Limit = Menu[19].Data;

  Menu[5].Id = 5;
  Menu[5].Name = "Pictures";
  Menu[5].Type = 1;
  Menu[5].Multiplikation = 10;
  Menu[5].Limit = 30000;
  
  Menu[6].Id = 6;
  Menu[6].Name = "Start";
  Menu[6].Type = 2;
  Menu[6].Action = 1;

  Menu[7].Id = 7;
  Menu[7].Name = "Calibrate";
  Menu[7].Type = 0;
  Menu[7].SubMenu = 6;

  Menu[8].Id = 8;
  Menu[8].Name = "Motor setings";
  Menu[8].Type = 0;
  Menu[8].SubMenu = 5;

  Menu[9].Id = 9;
  Menu[9].Name = "Screen setings";
  Menu[9].Type = 0;
  Menu[9].SubMenu = 4;

  Menu[10].Id = 10;
  Menu[10].Name = "Home";
  Menu[10].Type = 2;
  Menu[10].Action = 2;

  Menu[11].Id = 11;
  Menu[11].Name = "Move";
  Menu[11].Type = 2;
  Menu[11].Action = 3;
  
  Menu[12].Id = 12;
  Menu[12].Name = "Take a pic";
  Menu[12].Type = 2;
  Menu[12].Action = 4;

  Menu[13].Id = 13;
  Menu[13].Name = "Bf move ms";
  Menu[13].Type = 1;
  Menu[13].Multiplikation = 100;
  Menu[13].Limit = 10000;

  Menu[14].Id = 14;
  Menu[14].Name = "Brightnes";
  Menu[14].Type = 1;
  Menu[14].Multiplikation = 5;
  Menu[14].Limit = 255;

  Menu[15].Id = 15;
  Menu[15].Name = "Contrast";
  Menu[15].Type = 1;
  Menu[15].Multiplikation = 5;
  Menu[15].Limit = 255;

  Menu[16].Id = 16;
  Menu[16].Name = "Speed";
  Menu[16].Type = 1;
  Menu[16].Multiplikation = 5;
  Menu[16].Limit = 500;

  Menu[17].Id = 17;
  Menu[17].Name = "Accel mm/s";
  Menu[17].Type = 1;
  Menu[17].Multiplikation = 10;
  Menu[17].Limit = 10000;

  Menu[18].Id = 18;
  Menu[18].Name = "Steps/mm";
  Menu[18].Type = 1;
  Menu[18].Multiplikation = 0.05;
  Menu[18].Limit = 10000;
  
  Menu[19].Id = 19;
  Menu[19].Name = "Travel lim";
  Menu[19].Type = 1;
  Menu[19].Multiplikation = 1;
  Menu[19].Limit = 200;

  Menu[20].Id = 20;
  Menu[20].Name = "EEPROM Reset";
  Menu[20].Type = 2;
  Menu[20].Action = 5;

  Menu[21].Id = 21;
  Menu[21].Name = "Af move ms";
  Menu[21].Type = 1;
  Menu[21].Multiplikation = 100;
  Menu[21].Limit = 10000;

  Menu[22].Id = 22;
  Menu[22].Name = "Work setings";
  Menu[22].Type = 0;
  Menu[22].SubMenu = 7;

  if(MenuType == 0)
  {
     MenuLine[0] = Menu[1].Id;  //"Work"
     MenuLine[1] = Menu[2].Id;  //"Hand jog"
     MenuLine[2] = Menu[3].Id;  //"Setings"
     MenuItemsAmount = 3;
  }else if(MenuType == 1)
  {
     MenuLine[0] = Menu[0].Id;  //"Main menu"
     MenuLine[1] = Menu[10].Id;  //Home
     MenuLine[2] = Menu[22].Id;  //Work Setings
     MenuLine[3] = Menu[6].Id; //"Start"
     MenuItemsAmount = 4;   
  }else if(MenuType == 2)
  {
     MenuLine[0] = Menu[0].Id;  //"Main menu"
     MenuLine[1] = Menu[7].Id;  //"Kalibrate"
     MenuLine[2] = Menu[8].Id;  //"Motor Setings"
     MenuLine[3] = Menu[9].Id;  //"Screen Setings"
     MenuLine[4] = Menu[20].Id; //"EEPROM Reset"
     MenuItemsAmount = 5;
  }else if(MenuType == 3)
  {
     MenuLine[0] = Menu[0].Id;  //"Main menu"
     MenuLine[1] = Menu[10].Id;  //"Home"
     MenuLine[2] = Menu[11].Id;  //"Move"
     MenuLine[3] = Menu[12].Id;  //"Take a pic"
     MenuItemsAmount = 4;
  }else if(MenuType == 4)
  {
     MenuLine[0] = Menu[3].Id;  //"Setings"
     MenuLine[1] = Menu[14].Id;  //"Brightnes"
     MenuLine[2] = Menu[15].Id;  //"Contrast"
     MenuItemsAmount = 3;
  }else if(MenuType == 5)
  {
     MenuLine[0] = Menu[3].Id;  //"Setings"
     MenuLine[1] = Menu[16].Id;  //"Speed"
     MenuLine[2] = Menu[17].Id;  //"Accel"
     MenuItemsAmount = 3;
  }else if(MenuType == 6)
  {
     MenuLine[0] = Menu[3].Id;  //"Setings"
     MenuLine[1] = Menu[18].Id;  //"Steps/mm"
     MenuLine[2] = Menu[19].Id;  //"Movement lim"
     MenuItemsAmount = 3;
  }else if(MenuType == 7)
  {
     MenuLine[0] = Menu[1].Id;  //Work
     MenuLine[1] = Menu[4].Id;  //"Travel distance"
     MenuLine[2] = Menu[13].Id; //Time Befoure pic
     MenuLine[3] = Menu[21].Id; //Time After pic
     MenuLine[4] = Menu[5].Id;  //"Pictures count"
     MenuItemsAmount = 5;   
  }else if(MenuType == 255)
  {
    int i;
    for(i = 0; i <= MenuElements; i++)
    {
      if(Menu[i].Type == 1)
      {
        EEPROM.get(i*2, Menu[i].Data);
        Serial.print("Czytam adres nr ");
        Serial.print(i*2);
        Serial.print(" Otrzymałem \t");
        Serial.print(Menu[i].Name);
        Serial.print("\t");
        Serial.println(Menu[i].Data);
      }
      analogWrite(LCD_BACKLIGHT, Menu[14].Data);
      analogWrite(LCD_CONTRAST, Menu[15].Data);
      stepper.setRPM(Menu[16].Data/SCREW_PITCH_IN_MM);
      stepper.setSpeedProfile(stepper.LINEAR_SPEED, Menu[17].Data, Menu[17].Data);
    }
    MenuType = 0;
  }

  if((digitalRead(ENC_SW) != lastEncSwitchState) & (digitalRead(ENC_SW) == false))
  {
    lastEncSwitchState = digitalRead(ENC_SW);
    if(Menu[MenuLine[MenuCursorLine+Cursor]].Type == 0)
    {
      MenuType = Menu[MenuLine[MenuCursorLine+Cursor]].SubMenu;
      MenuCursorLine = 0;
    }
    else if(Menu[MenuLine[MenuCursorLine+Cursor]].Type == 1)
    {
      while(true)
      {
        encPos = enc.read();
        if(encPos > lastEncPos+3 || encPos < lastEncPos-3)
        {
          if((encPos > lastEncPos+3) & (Menu[MenuLine[MenuCursorLine+Cursor]].Data < Menu[MenuLine[MenuCursorLine+Cursor]].Limit))
          {
            Menu[MenuLine[MenuCursorLine+Cursor]].Data += Menu[MenuLine[MenuCursorLine+Cursor]].Multiplikation;

          }else if((encPos < lastEncPos-3) & (Menu[MenuLine[MenuCursorLine+Cursor]].Data > Menu[MenuLine[MenuCursorLine+Cursor]].Multiplikation-1))
          {
            Menu[MenuLine[MenuCursorLine+Cursor]].Data -= Menu[MenuLine[MenuCursorLine+Cursor]].Multiplikation;
          }
            uint8_t i;
            for(i = 1; i < 10 ; i++)
            {
              float Number_lenght = Menu[MenuLine[MenuCursorLine+Cursor]].Data/pow(10,i);
              if (Number_lenght < 1)
              {
                break;
              }
            }
          lcd.setCursor(16-i-1 ,Cursor);
          lcd.print(" ");
          lcd.print(Menu[MenuLine[MenuCursorLine+Cursor]].Data);
          if(Menu[MenuLine[MenuCursorLine+Cursor]].Id == 14)
          {
            analogWrite(LCD_BACKLIGHT, Menu[14].Data);
          }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Id == 15)
          {
            analogWrite(LCD_CONTRAST, Menu[15].Data);
          }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Id == 16)
          {  
            stepper.setRPM(Menu[16].Data/SCREW_PITCH_IN_MM);
          }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Id == 17)
          {  
            stepper.setSpeedProfile(stepper.LINEAR_SPEED, Menu[17].Data, Menu[17].Data);
          }
          lastEncPos = encPos;
        }
        delay(10);
        if((digitalRead(ENC_SW) != lastEncSwitchState) & (digitalRead(ENC_SW) == false))
        {
          EEPROM.put(Menu[MenuLine[MenuCursorLine+Cursor]].Id*2, Menu[MenuLine[MenuCursorLine+Cursor]].Data);
          Serial.println(Menu[MenuLine[MenuCursorLine+Cursor]].Data);
          break;
        }
        lastEncSwitchState = digitalRead(ENC_SW);
      }
    }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Type == 2)
    {
      if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 1) //Work Move
      {
        stepper.enable();
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Working");
        lcd.setCursor(0,1);
        lcd.print("Photos left ");
        int i;
        unsigned long TimeForMove; 
        for(i = 0; i < Menu[5].Data & HomeDistance/Menu[18].Data < Menu[19].Data; i++)
        {
          stepper.startMove(uint32_t(Menu[4].Data) * uint32_t(Menu[18].Data) / uint32_t(Menu[5].Data));
          HomeDistance += uint32_t(Menu[4].Data) * uint32_t(Menu[18].Data) / uint32_t(Menu[5].Data);
          TimeForMove = 5;
          while (TimeForMove > 0)
          {
            TimeForMove = stepper.nextAction();
            if((digitalRead(ENC_SW) != lastEncSwitchState) & (digitalRead(ENC_SW) == false)){
              break;
            }
          }
          lcd.setCursor(13,1);
          lcd.print(Menu[5].Data - i);
          lcd.print("    ");
          if((digitalRead(ENC_SW) != lastEncSwitchState) & (digitalRead(ENC_SW) == false)){
            break;
          }
          lastEncSwitchState = digitalRead(ENC_SW);
          delay(Menu[13].Data);
          sendbutton(start);
          delay(Menu[21].Data);
        }
        stepper.stop();
        stepper.disable();
      }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 2) //Homing
      {
        stepper.enable();
        stepper.startMove(-(uint32_t(Menu[18].Data) * uint32_t(Menu[19].Data)));
        HomeDistance -= uint32_t(Menu[18].Data) * uint32_t(Menu[19].Data);
        lcd.clear();
        lcd.setCursor(5, 0);
        lcd.print("Homing");
        lcd.setCursor(0, 1);
        lcd.print("Click to Stop");
        while(homeSwitchState == true)
        {
          stepper.nextAction();
          homeSwitchState = digitalRead(HOME_SWITCH);
          if((digitalRead(ENC_SW) != lastEncSwitchState) & (digitalRead(ENC_SW) == false)){
            break;
          }
        }
        stepper.stop();
        stepper.disable();
        Serial.println(stepper.getCurrentState());
        HomeDistance = 0;
      }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 3) //Hand Jog move
      {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Move");
        lcd.setCursor(0,1);
        lcd.print("Distance");
        int travelTime = 2;
        int8_t Move = 0;
        stepper.enable();
        while(true)
        {
          encPos = enc.read();
          if(encPos > lastEncPos+3 || encPos < lastEncPos-3){
            if(encPos > lastEncPos+3 & HomeDistance/Menu[18].Data < Menu[19].Data){
              Move++;
              stepper.move(Menu[18].Data);
              HomeDistance += Menu[18].Data;
            }else if(encPos < lastEncPos-3  & digitalRead(HOME_SWITCH) == true){
              Move--;
              stepper.move(-int16_t(Menu[18].Data));
              HomeDistance -= Menu[18].Data;
            }
            lcd.setCursor(12,1);
            lcd.print(Move);
            lcd.print("   ");
            lastEncPos = encPos;
          }
          if((digitalRead(ENC_SW) != lastEncSwitchState) & (digitalRead(ENC_SW) == false)){
            break;
          }
          lastEncSwitchState = digitalRead(ENC_SW);
        }  
        stepper.disable();
      }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 4) //Take a pic
      {
        sendbutton(start);
      }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 5) //EEPROM reset
      {
        Menu[4].Data = 125;
        Menu[5].Data = 0;
        Menu[13].Data = 500;
        Menu[14].Data = 255;
        Menu[15].Data = 90;
        Menu[16].Data = 200;
        Menu[17].Data = 2000;
        Menu[18].Data = 4693.33;
        Menu[19].Data = 160;
        Menu[21].Data = 500;
        int i;
        for(i = 0; i <= MenuElements; i++)
        {
          uint16_t Odczytywacz;
          if(Menu[i].Type == 1)
          {
            Serial.print("Wpisuję ");
            Serial.print(Menu[i].Data);
            Serial.print(" W adres ");
            Serial.println(i*2);
            EEPROM.put(i*2, Menu[i].Data);
            EEPROM.get(i*2, Odczytywacz);
            Serial.print("Otrzymałem ");
            Serial.println(Odczytywacz);
          }
        }
        MenuType = 255;
        Serial.println("EEPROM reseted");
      }
    }
    encPos += 4;
  }
  if(encPos > lastEncPos+3 || encPos < lastEncPos-3)//Menu moves
  {
    lcd.clear();
    if(encPos > lastEncPos+3)//Menu up chyba
      {
        if((Cursor == false) & (MenuCursorLine != 0))
        {
          MenuCursorLine -= 1;
        }
        Cursor = false;
      }else if(encPos < lastEncPos-3)//Menu down chyba
      {
        if((Cursor == true) & (MenuCursorLine < MenuItemsAmount-2))
        {
          MenuCursorLine += 1;
        }
        Cursor = true;
      }
    lcd.setCursor(0,0);
    
    if(Cursor == false)
    { 
    lcd.print(">");
    }
    lcd.print(Menu[MenuLine[MenuCursorLine]].Name);
    if((Menu[MenuLine[MenuCursorLine]].Type == 1))//Menu upper item
    {
      uint8_t i;
      for(i = 1; i < 10 ; i++)
      {
        float Number_lenght = (Menu[MenuLine[MenuCursorLine]].Data/pow(10,i));
        if (Number_lenght < 1)
        {
          break;
        }
      }
      lcd.setCursor(16-i,0);
      lcd.print(Menu[MenuLine[MenuCursorLine]].Data);
    }
    lcd.setCursor(0,1);
   
    if(Cursor == true)
    { 
    lcd.print(">");
    }
    lcd.print(Menu[MenuLine[MenuCursorLine+1]].Name);
    if((Menu[MenuLine[MenuCursorLine+1]].Type == 1))//Menu lover item
    {
      uint8_t i;
      for(i = 1; i < 10 ; i++)
      {
        float Number_lenght = (Menu[MenuLine[MenuCursorLine+1]].Data/pow(10,i));
        if (Number_lenght < 1)
        {
          break;
        }
      }
      lcd.setCursor(16-i,1);
      lcd.print(Menu[MenuLine[MenuCursorLine+1]].Data);
    }
    lastEncPos = encPos ;
  }
  lastEncSwitchState = digitalRead(ENC_SW);
}
// Routine to send header data burst
// This allows the IR reciever to set its AGC (Gain)
// Header Burst Timing is 96 * 0.025uS = 2.4mS
// Quiet Timing is 24 * 0.025uS = 600uS
  
// Routine to give the 40kHz burst signal
void burst()                   // 40KHz burst
{
  digitalWrite(IRled, HIGH);   // sets the pin on
  delayMicroseconds(10);       // pauses for 13 microseconds  (fudged to 10uS Delay)   
  digitalWrite(IRled, LOW);    // sets the pin off
  delayMicroseconds(8);        // pauses for 12 microseconds   (fudged to 8uS Delay)
}
// Routine to give a quiet period of data
void quiet()                   // Quiet burst
{
  digitalWrite(IRled, LOW);    // sets the pin off
  delayMicroseconds(10);       // pauses for 13 microseconds   (fudged to 10uS Delay)  
  digitalWrite(IRled, LOW);    // sets the pin off
  delayMicroseconds(8);        // pauses for 12 microseconds    (fudged to 8uS Delay)
}


void header() 
{
    for (int i=1; i <= 96; i++){
      burst();                // 40kHz burst
    }
    for (int i=1; i <= 24; i++){
      quiet();                // No 40 kHz
    }
}

// Routine to send one data burst
// Burst Timing is 48 * 0.025uS = 1.2mS
// Quiet Timing is 24 * 0.025uS = 600uS
void Data_is_One()
{
    for (int i=1; i <= 48; i++){
      burst();                // 40kHz burst
    }
    for (int i=1; i <= 24; i++){
      quiet();                // No 40 kHz
    }
}

// Routine to send zero data burst
// Burst Timing is 24 * 0.025uS = 600uS
// Quiet Timing is 24 * 0.025uS = 600uS
void Data_is_Zero()
{
    for (int i=1; i <= 24; i++){
      burst();                // 40 kHz burst
    }
    for (int i=1; i <= 24; i++){
      quiet();                // No 40 kHz 
    }
}

void sendbutton(int CodeBits[])
{
  for (int i=1; i <= 3; i++)  // Send Command 3 times as per Sony Specs
  {
    header();                    // Send the Start header
    for (int i=0; i <= 19; i++)  // Loop to send the bits
    {
          if(CodeBits[i] == 1)  // Is Data_is_One to be sent ?
          {
            Data_is_One();              // Yes, send a Data_is_One bit
          }
          else                  // No, Data_is_Zero to be sent
          {
            Data_is_Zero();              // Send a Data_is_Zero bit
          }
    }
    delay(11);                  // Delay Padding to give approx 45mS between command starts
  }
}
