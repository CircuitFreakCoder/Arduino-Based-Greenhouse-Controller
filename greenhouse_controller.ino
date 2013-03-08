/*************************************************************************************
  Subject:      Design Project II

  Thesis title: Controlled Environment for Vegetable Agriculture System for Farmers

  Project Description: The greenhouse controller automates the ventilation, irrigation, 
  and lighting of the greenhouse based on readings such as temperature, relative 
  humidity, soil moisture and time.

  Members:
  Caberio, Jorick
  Cacalda, Aina
  Decano, Jerico
  Gonzales, Mark Abril

  Links:
  http://www.youtube.com/watch?v=X1wTCFCZVq0

  http://www.youtube.com/watch?v=M_Y4bFY2WNw

  Arduino-based Greenhouse Controller Firmware by Jorick Caberio
  

  MIT License:

  Permission is hereby granted, free of charge, to any person obtaining a copy of this 
  software and associated documentation files (the "Software"), to deal in the Software 
  without restriction, including without limitation the rights to use, copy, modify, 
  merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
  permit persons to whom the Software is furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all copies or 
  substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


***************************************************************************************/


                                                                                                                                     
//for DHT11 temperature/humidity sensor
#include "DHT.h"
#define DHTPIN 44     // pin connected to DHT sensor
#define DHTTYPE DHT11   // model number of DHT sensor
DHT dht(DHTPIN, DHTTYPE);  //create DHT object


//for RTC
#include <Wire.h>  //library to communicate with I2C devices
#include "RTClib.h"  //library for the real time clock
RTC_DS1307 RTC; 


//for LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(24, 17, 23, 25, 27, 29);//RS,EN,D4,D5,D6,D7

//progress bar

byte pOne[] = {
  16,16,16,16,16,16,16,16};

byte pTwo[] = {
  24,24,24,24,24,24,24,24
};

byte pThree[] = {
  28,28,28,28,28,28,28,28
};

byte pFour[] = {
  30,30,30,30,30,30,30,30
};

byte pFive[] = {
  31,31,31,31,31,31,31,31
};


//keypad
#include <Keypad.h>
const byte ROWS = 4; //four rows by
const byte COLS = 4; //four columns keypad matrix
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {' ','0','#','D'}
};
byte rowPins[ROWS] = {45, 43, 42, 34}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {37, 35, 33, 31}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

char modeKey;


//for sd card
#include <SD.h>
const int chipSelect = 4;
const int SDCardDataPin = 10;


//declare analog pins for soil moisture sensor
int soilMoisturePins[] = {A0, A1, A2, A3, A4, A5};
//declare an array for the soil moisture readings
int soilMoistureReadings[6];
int averageSoilMoistureReading;

//vcc and gnd pins`
const int gndPin = 36;
const int vccPin = 38;
//transistor pins
const int solenoidPin = 14;
const int exhaustPin = 16;
const int bulbPin = 40;
//led indicator
const int powerIndicator = 30;
const int fanIndicator = 28;
const int valveIndicator = 26;
const int gndIndicator = 32;

//limits for activating the actuators
int temperatureLimit = 30;  //turn on fan when temperature is greater  than 30 degrees Celcius 
int moistureLimit = 700;  //turn on solenoid valve when soil moisture is less than 300 which corresponds to "DRY" 
int bulbSwitchOnTime = 18;  //switch on the bulb when it's 6pm



void setup() {
  
  //Serial monitor
  Serial.begin(9600);
  
  Wire.begin();
  RTC.begin();
  
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  //create custom char
    
  lcd.createChar(0, pOne);
  lcd.createChar(1, pTwo);
  lcd.createChar(2, pThree);
  lcd.createChar(3, pFive);
  lcd.createChar(4, pFive);
      
  // set up the LCD's number of columns and rows 
  lcd.begin(20, 4);
  //initialization message
  displayMssg("Design Project 2",0,0);
  displayMssg("Caberio Cacalda",0,1);
  displayMssg("Decano  Gonzales",0,2);
  displayMssg("Loading",0,3);
  
  //display progress bar
  
  for(int j=7; j<20; j++){
    
    for(int i=0; i<5; i++){
      lcd.setCursor(j,3);
      lcd.write(i);
      delay(100);
    }
  }
  
  lcd.clear();
  //prompt user for instructions
  displayMssg("Mode",0,0);
  displayMssg("A - Temp limit",0,1);
  displayMssg("B - Moisture limit",0,2);
  delay(3000);
  lcd.clear();
  displayMssg("C - Bulb switch time",0,1);
  displayMssg("D - Compute Harvest",0,2);
  delay(3000);

  //setup analog pins as input
  for(int i=0; i<6; i++){
    pinMode(soilMoisturePins[i],INPUT);
  }
  
  //setup vcc and gnd
  pinMode(gndPin, OUTPUT);
  pinMode(vccPin, OUTPUT);
  digitalWrite(gndPin, LOW);
  digitalWrite(vccPin, HIGH);


  pinMode(gndIndicator, OUTPUT);
  pinMode(powerIndicator, OUTPUT);
  pinMode(fanIndicator, OUTPUT);
  pinMode(valveIndicator, OUTPUT);
  
  digitalWrite(gndIndicator, LOW);
  digitalWrite(powerIndicator, HIGH);
  digitalWrite(fanIndicator, LOW);
  digitalWrite(valveIndicator, LOW);


  
  //setup transistor pins
  pinMode(solenoidPin, OUTPUT);
  pinMode(exhaustPin, OUTPUT); 
  pinMode(bulbPin, OUTPUT);

  //set up dht11 sensor
  dht.begin();

  //set up sd card data pin  
  pinMode(SDCardDataPin, OUTPUT);
  
    // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) 
  {
    lcd.clear();
    lcd.print("Card failed");
  // don't do anything more:
    return;
  }
    
  //clear the LCD
  lcd.clear();
  
}//END OF setup



void loop()
{
    
  
    //start the real time clock
    DateTime now = RTC.now();

    //get temperature and humidity readings
    float humidityReading = dht.readHumidity();
    float temperatureReading = dht.readTemperature();
          
    //get user's input mode
    getMode();
      
    //get soil moisture readings
    int totalSoilMoistureReadings = 0;
    for(int i=0; i<6; i++){
    soilMoistureReadings[i] = analogRead(soilMoisturePins[i]);
    totalSoilMoistureReadings += soilMoistureReadings[i];
    }
    averageSoilMoistureReading = totalSoilMoistureReadings/6;

    //display temperature and humidity readings on LCD
    displayMssg(" Temperature: "+String((int)temperatureReading)+" C",0,0);
    
    getMode();
    
    displayMssg(" Humidity: "+String((int)humidityReading)+" %",0,1);
    
    getMode();
    
    //display the average soil moisture reading on the LCD
    displayMssg(" Moisture: "+String(averageSoilMoistureReading)+ getMoistureLevel(averageSoilMoistureReading),0,2);
    
    getMode();
    
    //determine if it the current time is AM or PM
    AM_PM(now.hour());
    
    //display the date and time on the LCD
    displayMssg(
    String(now.year(), DEC)+
    "/"+
    String(now.month(), DEC)+
    "/"+
    String(now.day(), DEC)+
    " "+
    hourFormat(now.hour())+
    //String(now.hour(), DEC)+
    ":"+
    String(now.minute(), DEC),1,3 );
    
    getMode();
 
    //log the readings 6 times every 30 minutes    
    if((now.minute()% 30 == 0) &&(now.second()% 10 == 0))
    
    {
      File dataFile = SD.open("joric.txt", FILE_WRITE);

      // if the file is available, write to it:
      if (dataFile) {
 
        //write the date and time
        dataFile.print(now.year(), DEC);
        dataFile.print("/");
        dataFile.print(now.month(), DEC);
        dataFile.print("/");
        dataFile.print(now.day(), DEC);
        dataFile.print("  ");
        dataFile.print(now.hour());
        dataFile.print(":");
        dataFile.print(now.minute());
        dataFile.print(":");
        dataFile.println(now.second());
        dataFile.println();
        
        //write the temperature and humidity readings
        dataFile.print("Temperature: ");
        dataFile.print(temperatureReading);
        dataFile.println(" C");
        dataFile.print("Humidity: ");
        dataFile.print(humidityReading);
        dataFile.println(" %");
        dataFile.println();
    
        //write the soil moisture readings
        dataFile.print("Soil moisture sensor 1: ");
        dataFile.println(soilMoistureReadings[0]);
  
        dataFile.print("Soil moisture sensor 2: ");
        dataFile.println(soilMoistureReadings[1]);
  
        dataFile.print("Soil moisture sensor 3: ");
        dataFile.println(soilMoistureReadings[2]);
   
        dataFile.print("Soil moisture sensor 4: ");
        dataFile.println(soilMoistureReadings[3]);
   
        dataFile.print("Soil moisture sensor 5: ");
        dataFile.println(soilMoistureReadings[4]);
  
        dataFile.print("Soil moisture sensor 6: ");
        dataFile.println(soilMoistureReadings[5]);
  
        dataFile.print("Average Soil Moisture: ");
        dataFile.println(averageSoilMoistureReading);
        dataFile.print("Soil Moisture Level: ");
        dataFile.println(getMoistureLevel(averageSoilMoistureReading));    
    
        
        //close the file
        dataFile.close();
        
        //display a notification on the LCD
        displayMssg("                    ",0,3);
        displayMssg(" Data saved",0,3);
        delay(800);
        displayMssg("                    ",0,3);
        
        
    
      }//END OF if(dataFile)  
  
    }//END OF if((now.minute()%1==0) &&(now.second()%10 == 0))

    getMode();
    
    //switch on the solenoid valve
    activateValve(averageSoilMoistureReading);
  
    getMode();
    
    //switch on the fan
    activateFan(temperatureReading);
    
    getMode();
    
    //switch on the bulb
    activateBulb(now.hour());
    
    getMode();

}//END OF loop


//function to convert current time into 12 hour format
String hourFormat(int h_24){
  
  int h = h_24 % 12;
  if (h == 0) h = 12;
  return String(h); 

}//END OF hourFormat

//function to determine if the time is AM or PM
void AM_PM(int h_24){

  if(h_24<12)
  displayMssg("AM",16,3);
  else
  displayMssg("PM",16,3);

}//END OF AM_PM

//function to print a message at a specific column and row on the LCD
void displayMssg(String message, int column, int row){
  
  lcd.setCursor(column, row);
  lcd.print(message);
  
}//END OF diplayMssg

//function to determine the soil moisture level description
String getMoistureLevel(int newSoilMoistureReading){
  
  if(newSoilMoistureReading<=moistureLimit)
    return " Dry  ";
    
  else if(newSoilMoistureReading>800 && newSoilMoistureReading <=900 )
    return " Humid";
  
  else if(newSoilMoistureReading>900  )
    return " Wet  ";
    
  else
    return " Dry" ;
    
  
}//END OF getSoilMoistureLevel

//function to determine if the solenoid valve should be switched on 
void activateValve(int newSoilMoistureReading){
  
  if(newSoilMoistureReading <= moistureLimit){
    digitalWrite(valveIndicator, HIGH);
    digitalWrite(solenoidPin, LOW);//turn on the solenoid valve
  }
  else{
    digitalWrite(valveIndicator, LOW);
    digitalWrite(solenoidPin, HIGH);
  }
}//END OF activateIrrigation

//function to determine if the exhaust fan should be switched on
void activateFan(int newTemperatureReading){

  if(newTemperatureReading >= temperatureLimit){
    digitalWrite(fanIndicator, HIGH);
    digitalWrite(exhaustPin, LOW);
  }
      
  else{
    digitalWrite(fanIndicator, LOW);
    digitalWrite(exhaustPin, HIGH);
  }
}//END OF activateExhaustFan

////function to determine if the light bulb should be switched on
void activateBulb(int hours){
  //turn the bulb on if its 6pm(18)
  if(hours >= bulbSwitchOnTime )
    digitalWrite(bulbPin, HIGH);
  else
    digitalWrite(bulbPin, LOW);
    
}//END OF activateBulb

//function to determine the user's mode input
void getMode(){
  
  modeKey = keypad.getKey();
  
  switch(modeKey){
  
    case 'A':
      setTemperatureLimit();
      break;
      
    case 'B':
      setMoistureLimit();
      break;
      
    case 'C':
      setLightsOnTime();
      break;
      
    case 'D':
      computeHarvest();
      break;
    
  }//END OF switch(modeKey)

}//END OF getMode

//function to set the temperature limit that will trigger the fan to switch on
void setTemperatureLimit(){
  
  lcd.clear();
  displayMssg("Set temp limit",0,0);
  displayMssg("# Enter * Backspace",0,1);
  
  char tempDigits;
  int columnIndex=0;
  char tempCharArray[20];
  int tempCharArrayToInt;
  int counter = 0; 
  
  while(tempDigits != '#'){
    
    tempDigits = keypad.waitForKey();
    tempCharArray[counter] = tempDigits;
     
    if(tempDigits==' '){
    
    lcd.setCursor(columnIndex-1, 2);
    lcd.print(tempCharArray[counter]);
    counter--;
    columnIndex--;
    
    }
    
    else{
    lcd.setCursor(columnIndex, 2);
    lcd.print(tempCharArray[counter]);
    counter++;
    columnIndex++;
    }
  }
  tempCharArrayToInt = atoi(tempCharArray);
  temperatureLimit = tempCharArrayToInt;
  displayMssg("Temp limit = " + String(temperatureLimit)+" C", 0,3);
  delay(2000);
  lcd.clear();
}

//function to set the soil moisture limit that will trigger the solenoid valve to switch on
void setMoistureLimit(){
  
  lcd.clear();
  displayMssg("Set moisture limit",0,0);
  displayMssg("# Enter * Backspace",0,1);
  char moistDigits;
  int columnIndex=0;
  char moistCharArray[20];
  int moistCharArrayToInt;
  int counter = 0; 
  
  while(moistDigits != '#'){
    
    moistDigits = keypad.waitForKey();
    moistCharArray[counter] = moistDigits;
    
    if(moistDigits==' '){
    
    lcd.setCursor(columnIndex-1, 2);
    lcd.print(moistCharArray[counter]);
    counter--;
    columnIndex--;
    
    }
    
    else{
    lcd.setCursor(columnIndex, 2);
    lcd.print(moistCharArray[counter]);
    counter++;
    columnIndex++;
    }
  }
  moistCharArrayToInt = atoi(moistCharArray);
  moistureLimit = moistCharArrayToInt;
  displayMssg("Moist. limit = " + String(moistureLimit), 0,3);
  delay(2000);
  lcd.clear();
}

//function to set the bulb's switch on time
void setLightsOnTime(){
  
  lcd.clear();
  displayMssg("Set bulb switch on" ,0,0);
  displayMssg("hour in 24hr format",0,1);
  delay(1500);
  lcd.clear();
  
  displayMssg("Set hour",0,0);
  displayMssg("# Enter * Backspace",0,1);
  
  char bulbDigits;
  int columnIndex=0;
  char bulbCharArray[20];
  int bulbCharArrayToInt;
  int counter = 0; 
  
  while(bulbDigits != '#'){
    
    bulbDigits = keypad.waitForKey();
    bulbCharArray[counter] = bulbDigits;
    
    if(bulbDigits==' '){
    
    lcd.setCursor(columnIndex-1, 2);
    lcd.print(bulbCharArray[counter]);
    counter--;
    columnIndex--;
    
    }
    
    else{
    lcd.setCursor(columnIndex, 2);
    lcd.print(bulbCharArray[counter]);
    counter++;
    columnIndex++;
    }
  }
  
  bulbCharArrayToInt = atoi(bulbCharArray);
  bulbSwitchOnTime = bulbCharArrayToInt;
  displayMssg("Sw.On Hour = ", 0,3);
  displayMssg(hourFormat(bulbSwitchOnTime),13,3 );
  AM_PM(bulbSwitchOnTime);
  delay(2000);
  lcd.clear();
}

//function to compute the projected harvest
void computeHarvest(){

  lcd.clear();
  displayMssg("Enter area in sq.m.",0,0);
  displayMssg("# Enter * Backspace",0,1);
  char harvestDigits;
  int columnIndex=0;
  char harvestCharArray[20];
  int harvestCharArrayToInt;
  int counter = 0; 
  
  while(harvestDigits != '#'){
    
    harvestDigits = keypad.waitForKey();
    harvestCharArray[counter] = harvestDigits;
    
    if(harvestDigits==' '){
    
    lcd.setCursor(columnIndex-1, 2);
    lcd.print(harvestCharArray[counter]);
    counter--;
    columnIndex--;
    
    }
    
    else{
    lcd.setCursor(columnIndex, 2);
    lcd.print(harvestCharArray[counter]);
    counter++;
    columnIndex++;
    }
  }
   
  counter = columnIndex = 0;
  
  harvestCharArrayToInt = atoi(harvestCharArray);
  int harvest = harvestCharArrayToInt;
  displayMssg("harvest in kg = "+String( harvest),0,3);
  delay(1500);
  lcd.clear();
  displayMssg("Enter price/kg",0,0);
  displayMssg("# Enter * Backspace",0,1);
  char priceDigits;
  char priceCharArray[20];
  int priceCharArrayToInt;
  int price;
  int compute;
  
  do{
    
    priceDigits = keypad.waitForKey();
    priceCharArray[counter] = priceDigits;
    
    if(priceDigits==' '){
    
    lcd.setCursor(columnIndex-1, 2);
    lcd.print(priceCharArray[counter]);
    counter--;
    columnIndex--;
    
    }
    
    else{
    lcd.setCursor(columnIndex, 2);
    lcd.print(priceCharArray[counter]);
    counter++;
    columnIndex++;
    }
  }while (priceDigits != '#');
  
  priceCharArrayToInt = atoi(priceCharArray);
  price = priceCharArrayToInt;
  compute = price * harvest;
  delay(800);
  lcd.clear();
  displayMssg("Harvest = " + String(harvest) + " kg", 0,0);
  displayMssg("Price = P"+String(compute),0,1);
  delay(2000);
  lcd.clear();

}


