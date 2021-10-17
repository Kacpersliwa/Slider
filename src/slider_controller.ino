#include <Arduino.h>
#include <Encoder.h>
#include <A4988.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

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
uint8_t encPos = 0;

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

  int ScreenBrightnes;
  int ScreenContrast;

  int MotorSpeed;
  int MotorAccel;

  EEPROM.get(13, ScreenBrightnes);
  EEPROM.get(14, ScreenContrast);
  
  EEPROM.get(15, MotorSpeed);
  EEPROM.get(16, MotorAccel);
  
  pinMode(HOME_SWITCH, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);

  lcd.begin(16, 2);
  ScreenBrightnes = 255;
  ScreenContrast = 90;
  
  pinMode(LCD_CONTRAST, OUTPUT);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  analogWrite(LCD_CONTRAST, ScreenContrast);
  analogWrite(LCD_BACKLIGHT, ScreenBrightnes);

  stepper.setSpeedProfile(BasicStepperDriver::Mode::LINEAR_SPEED);
  stepper.setEnableActiveState(LOW);
  stepper.enable();
  stepper.begin(MotorSpeed,STEPPER_MICROSTEPS_PER_STEP);
}

void loop() {
  //static bool lastHomeSwitchState = false;
  static bool lastEncSwitchState = false;
  static bool Cursor = true;
  static uint8_t lastEncPos = 0;
  static uint8_t MenuItemsAmount = 0;
  static uint8_t MenuCursorLine = 0;
  static uint8_t MenuType = 0;
  static uint8_t MenuLine[6];
  const uint8_t MenuElements = 21;

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
  Menu[4].Name = "Travel";
  Menu[4].Type = 1;
  Menu[4].Multiplikation = 100;
  Menu[4].Limit = Menu[19].Data;

  Menu[5].Id = 5;
  Menu[5].Name = "Pictures";
  Menu[5].Type = 1;
  Menu[5].Multiplikation = 50;
  Menu[5].Limit = 30000;
  
  Menu[6].Id = 6;
  Menu[6].Name = "Start";
  Menu[6].Type = 2;
  Menu[6].Action = 1;

  Menu[7].Id = 7;
  Menu[7].Name = "Kalibrate";
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
  Menu[13].Name = "Soft lim";
  Menu[13].Type = 2;
  Menu[13].Action = 5;

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
  Menu[17].Name = "Accel";
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
  Menu[19].Multiplikation = 100;
  Menu[19].Limit = 40000;

  Menu[20].Id = 20;
  Menu[20].Name = "EEPROM Reset";
  Menu[20].Type = 2;
  Menu[20].Action = 6;

  if(MenuType == 0)
  {
     MenuLine[0] = Menu[1].Id;  //"Work"
     MenuLine[1] = Menu[2].Id;  //"Hand jog"
     MenuLine[2] = Menu[3].Id;  //"Setings"
     MenuItemsAmount = 3;
  }else if(MenuType == 1)
  {
     MenuLine[0] = Menu[0].Id;  //"Main menu"
     MenuLine[1] = Menu[4].Id;  //"Pictures count"
     MenuLine[2] = Menu[5].Id;  //"Travel distance"
     MenuLine[3] = Menu[6].Id;  //"Start"
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
     MenuLine[2] = Menu[13].Id;  //"Soft stop"
     MenuLine[3] = Menu[19].Id;  //"Movement lim"
     MenuItemsAmount = 4;
  }
  else if(MenuType == 255)
  {
    int i;
    for(i = 0; i <= MenuElements-1; i++)
    {
      if(Menu[i].Type == 1)
      {
        EEPROM.get(i, Menu[i].Data);
      }
    }
    MenuType = 0;
  }

  if((digitalRead(ENC_SW) != lastEncSwitchState) & (digitalRead(ENC_SW) == false))
  {
    if(Menu[MenuLine[MenuCursorLine+Cursor]].Type == 0)
    {
      MenuType = Menu[MenuLine[MenuCursorLine+Cursor]].SubMenu;
      MenuCursorLine = 0;
    }
    else if(Menu[MenuLine[MenuCursorLine+Cursor]].Type == 1)
    {
      while(true)
      {
        lastEncSwitchState = digitalRead(ENC_SW);
        encPos = enc.read();
        if((encPos/4) != lastEncPos)
        {
          lcd.setCursor(11 ,Cursor);
          lcd.print(Menu[MenuLine[MenuCursorLine+Cursor]].Data);
          lcd.print("    ");
        if(((encPos/4) < lastEncPos) & (Menu[MenuLine[MenuCursorLine+Cursor]].Data < Menu[MenuLine[MenuCursorLine+Cursor]].Limit))
          {
            Menu[MenuLine[MenuCursorLine+Cursor]].Data += Menu[MenuLine[MenuCursorLine+Cursor]].Multiplikation;
          }else if(((encPos/4) > lastEncPos) & (Menu[MenuLine[MenuCursorLine+Cursor]].Data > 0))
          {
            Menu[MenuLine[MenuCursorLine+Cursor]].Data -= Menu[MenuLine[MenuCursorLine+Cursor]].Multiplikation;
            Serial.println("Hełoł");
          }
          if(Menu[MenuLine[MenuCursorLine+Cursor]].Id == 14)
          {
            analogWrite(LCD_BACKLIGHT, Menu[14].Data);
          }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Id == 15)
          {
            analogWrite(LCD_CONTRAST, Menu[15].Data);
          }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Id == 17)
          {  
           stepper.setSpeedProfile(BasicStepperDriver::Mode::LINEAR_SPEED, Menu[15].Data, Menu[15].Data);
          }
        }
        if((digitalRead(ENC_SW) != lastEncSwitchState) & (digitalRead(ENC_SW) == false))
        {
          EEPROM.put(Menu[MenuLine[MenuCursorLine+Cursor]].Id, Menu[MenuLine[MenuCursorLine+Cursor]].Data);
          break;
        }
        lastEncPos = encPos/4;
        lastEncSwitchState = digitalRead(ENC_SW);
      }
    }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Type == 2)
    {
      if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 1)
      {
        
      }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 2)
      {

      }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 3)
      {

      }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 4)
      {

      }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 5)
      {

      }else if(Menu[MenuLine[MenuCursorLine+Cursor]].Action == 6)
      {
        Menu[14].Data = 255;
        Menu[15].Data = 90;
        Menu[16].Data = 200;
        Menu[17].Data = 2000;
        Menu[18].Data = 4693.33;
        Menu[19].Data = 30000;
        int i;
        for(i = 0; i <= MenuElements-1; i++)
        {
          if(Menu[i].Type == 1)
          {
            EEPROM.put(i, Menu[i].Data);
          }
        }
        Serial.println("EEPROM reseted");
      }
      
    }
    encPos -= 4;
  }
  if(encPos/4 != lastEncPos)
  {
    lcd.clear();
    if((encPos/4) > lastEncPos)
      {
        if((Cursor == true) & (MenuCursorLine < MenuItemsAmount-2))
        {
          MenuCursorLine += 1;
        }
        Cursor = true;
      }else if((encPos/4) < lastEncPos)
      {
        if((Cursor == false) & (MenuCursorLine != 0))
        {
          MenuCursorLine -= 1;
        }
        Cursor = false;
      }
    Serial.println(MenuCursorLine);
    lcd.setCursor(0,0);
    
    if(Cursor == false)
    { 
    lcd.print(">");
    }
    lcd.print(Menu[MenuLine[MenuCursorLine]].Name);
    if((Menu[MenuLine[MenuCursorLine]].Type == 1) || (Menu[MenuLine[MenuCursorLine]].Type == 2))
    {
      lcd.setCursor(11,0);
      lcd.print(Menu[MenuLine[MenuCursorLine]].Data);
    }
    lcd.setCursor(0,1);
   
    if(Cursor == true)
    { 
    lcd.print(">");
    }
    lcd.print(Menu[MenuLine[MenuCursorLine+1]].Name);
    if((Menu[MenuLine[MenuCursorLine+1]].Type == 1) || (Menu[MenuLine[MenuCursorLine+1]].Type == 2))
    {
      lcd.setCursor(11,1);
      lcd.print(Menu[MenuLine[MenuCursorLine+1]].Data);
    }
  }
    lastEncSwitchState = digitalRead(ENC_SW);
    lastEncPos = encPos/4 ;
}
  
  
  // Periodic switching to the next screen.
  // if (millis() - lastMs_nextScreen > period_nextScreen) {
  //   lastMs_nextScreen = millis();
  //   menu.next_screen();
  // }
