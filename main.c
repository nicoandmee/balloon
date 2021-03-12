/*
 Weather Shield Example Used in this code.
 This code was utilized by Nico for a High Altitutde Balloon Payload.
 Geiger Counter example used in this code.
 */

#include <Wire.h> //I2C needed for sensors
#include "MPL3115A2.h" //Pressure sensor
#include "HTU21D.h" //Humidity sensor

MPL3115A2 myPressure; //Create an instance of the pressure sensor
HTU21D myHumidity; //Create an instance of the humidity sensor

//Hardware pin definitions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// digital I/O pins
const byte WSPEED = 3;
const byte RAIN = 2;
const byte STAT1 = 7;
const byte STAT2 = 8;

// analog I/O pins
const byte REFERENCE_3V3 = A3;
const byte LIGHT = A1;
const byte BATT = A2;
const byte WDIR = A0;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
long lastSecond; //The millis counter to see when a second rolls by
byte seconds; //When it hits 60, increase the current minute
byte seconds_2m; //Keeps track of the "wind speed/dir avg" over last 2 minutes array of data
byte minutes; //Keeps track of where we are in various arrays of data
byte minutes_10m; //Keeps track of where we are in wind gust/dir over last 10 minutes array of data

long lastWindCheck = 0;
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;

int radiation = 0; //CPM value for Geiger counter
//These are all the weather values that wunderground expects:
float humidity = 0; // [%]
float tempf = 0; // [temperature F]
//float baromin = 30.03;// [barom in] - It's hard to calculate baromin locally, do this in the agent
float pressure = 0;
//float dewptf; // [dewpoint F] - It's hard to calculate dewpoint locally, do this in the agent

float batt_lvl = 11.8; //[analog value from 0 to 1023]
float light_lvl = 455; //[analog value from 0 to 1023]



int i;
int count;
int old = -1;
int check;
float CPM;
float now;
float time;
int start;
int statusled = 10;
int sketchstatusled = 12;
char OnesString[10];
char DecimalString[10];
char TimerString[10];

// volatiles are subject to modification by IRQs

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=



void setup()
{
  Serial.begin(9600); //Serial port for Output to OpenLog
  Serial1.begin(9600);
  Serial.println("MCP HAB Project");

  pinMode(STAT1, OUTPUT); //Status LED Blue
  pinMode(STAT2, OUTPUT); //Status LED Green


  pinMode(REFERENCE_3V3, INPUT);
  pinMode(LIGHT, INPUT);

  //Configure the pressure sensor
  myPressure.begin(); // Get sensor online
  myPressure.setModeBarometer(); // Measure pressure in Pascals from 20 to 110 kPa
  myPressure.setOversampleRate(7); // Set Oversample to the recommended 128
  myPressure.enableEventFlags(); // Enable all three pressure and temp event flags

  //Configure the humidity sensor
  myHumidity.begin();

  seconds = 0;
  lastSecond = millis();



  Serial.println("Weather Shield online.");

}

void loop()
{
  //Keep track of which minute it is
  if(millis() - lastSecond >= 1000)
  {
    digitalWrite(STAT1, HIGH); //Blink stat LED

    lastSecond += 1000;

    //Report all readings every second
    printWeather();

    digitalWrite(STAT1, LOW); //Turn off stat LED
  }





  delay(100);
}

//Calculates each of the variables that wunderground is expecting
void calcWeather()
{

  //Calc humidity
  humidity = myHumidity.readHumidity();
  //float temp_h = myHumidity.readTemperature();
  //Serial.print(" TempH:");
  //Serial.print(temp_h, 2);

  //Calc tempf from pressure sensor
  tempf = myPressure.readTempF();
  //Serial.print(" TempP:");
  //Serial.print(tempf, 2);


  //Calc pressure
  pressure = myPressure.readPressure();

  //Calc dewptf

  //Calc light level
  light_lvl = get_light_level();

  //Calc battery level
  batt_lvl = get_battery_level();

  //Calc CPM.
  radiation = GetCPM();
}
int GetCPM()
{

  i = 0;
  count = 0;
  start = millis();
  while (i < 30)
  {
    if(Serial1.available() > 0)
    {
      check = Serial1.read();
      if(check > 0)
      {
        count++;
      }
    }
    i = millis();
    i = i - start;
    i = i/1000;

  }

  CPM = count*2;
  old = count; //resetting the if statement
  int cpm = CPM;
  int temp = CPM*1000;
  int decimal = temp % (cpm * 1000);
  return cpm;
}




//Returns the voltage of the light sensor based on the 3.3V rail
//This allows us to ignore what VCC might be (an Arduino plugged into USB has VCC of 4.5 to 5.2V)
float get_light_level()
{
  float operatingVoltage = analogRead(REFERENCE_3V3);

  float lightSensor = analogRead(LIGHT);

  operatingVoltage = 3.3 / operatingVoltage; //The reference voltage is 3.3V

  lightSensor = operatingVoltage * lightSensor;

  return(lightSensor);
}

//Returns the voltage of the raw pin based on the 3.3V rail
//This allows us to ignore what VCC might be (an Arduino plugged into USB has VCC of 4.5 to 5.2V)
//Battery level is connected to the RAW pin on Arduino and is fed through two 5% resistors:
//3.9K on the high side (R1), and 1K on the low side (R2)
float get_battery_level()
{
  float operatingVoltage = analogRead(REFERENCE_3V3);

  float rawVoltage = analogRead(BATT);

  operatingVoltage = 3.30 / operatingVoltage; //The reference voltage is 3.3V

  rawVoltage = operatingVoltage * rawVoltage; //Convert the 0 to 1023 int to actual voltage on BATT pin

  rawVoltage *= 4.90; //(3.9k+1k)/1k - multiple BATT voltage by the voltage divider to get actual system voltage

  return(rawVoltage);
}



//Prints the various variables directly to the port
//I don't like the way this function is written but Arduino doesn't support floats under sprintf
void printWeather()
{
  calcWeather(); //Go calc all the various sensors

  Serial.println();
  Serial.print(",CPM=");
  Serial.print(CPM, 1);
  Serial.print(",humidity=");
  Serial.print(humidity, 1);
  Serial.print(",tempf=");
  Serial.print(tempf, 1);
  Serial.print(",pressure=");
  Serial.print(pressure, 2);
  Serial.print(",batt_lvl=");
  Serial.print(batt_lvl, 2);
  Serial.print(",light_lvl=");
  Serial.print(light_lvl, 2);
  Serial.print(",");
  Serial.println("#");

}