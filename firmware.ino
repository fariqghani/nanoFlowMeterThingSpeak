#include "ThingSpeak.h"
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiEsp.h>

//Define variables 

#define I2C_ADDR          0x27        //Define I2C Address where the PCF8574A is
#define BACKLIGHT_PIN      3
#define En_pin             2
#define Rw_pin             1
#define Rs_pin             0
#define D4_pin             4
#define D5_pin             5
#define D6_pin             6
#define D7_pin             7
/*
#ifndef HAVE_HWEspSerial
#include "SoftwareSerial.h"
SoftwareSerial EspSerial(15, 16); // RX, TX
#endif
*/
//Initialise the LCD
LiquidCrystal_I2C      lcd(I2C_ADDR, En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);
byte statusLed    = 13;

byte sensorInterrupt = 0;  // 0 = analog pin 0
byte sensorPin       = 0;

// The hall-effect flow sensor outputs approximately 4.5 pulses per second per
// litre/minute of flow.
float calibrationFactor = 4.5;

volatile byte pulseCount;  

float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
unsigned long oldTime;
float totalLitres;
float totalMeterCube;
float bill;

char ssid[] = "wifiSSID";
char pass[] = "wifiPass";
int status = WL_IDLE_STATUS;

unsigned long myChannelNumber = 123456;
const char * myWriteAPIKey = 1234567890;

void setup()
{
 /*
    Serial.begin(115200);
    while (!Serial) ; // wait for serial port to connect. Needed for native USB
    Serial.println("start");
    delay(2000);
    EspSerial.begin(9600); // your esp's baud rate might be different 9600, 57600, 76800 or 115200
    delay(2000);
    WiFi.init(&EspSerial);

    if (WiFi.status() == WL_NO_SHIELD) {
      Serial.println("WiFi shield not present");
      // don't continue
      while (true);
      }

      // attempt to connect to WiFi network
      while ( status != WL_CONNECTED) {
      Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
    }
*/
  // you're connected now, so print out the data
  Serial.println("You're connected to the network");
  
  // Set up the status LED line as an output  
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH); 
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);

  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  totalLitres       = 0;
  totalMeterCube    = 0;
  bill              = 0;
  oldTime           = 0;

  // The Hall-effect sensor is connected to pin 2 which uses interrupt 0.
  // Configured to trigger on a FALLING state change (transition from HIGH
  // state to LOW state)
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  
  //ThingSpeak.begin(client);  // Initialize Thingspeak

}

/**
 * Main program loop
 */
void loop()
{
   
   if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);
        
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
    
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
    totalLitres = totalMilliLitres * 0.001;
    totalMeterCube = totalLitres * 0.001;

     unsigned int frac;

  // Print the flow rate for this second in litres / minute
  Serial.print("Flow rate: ");
  Serial.print(int(flowRate)); // Print the integer part of the variable
  Serial.print("."); // Print the decimal point
  // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
  frac = (flowRate - int(flowRate)) * 10;
  Serial.print(frac, DEC) ; // Print the fractional part of the variable
  Serial.print("L/min");
  // Print the number of litres flowed in this second
  Serial.print(" Current Liquid Flowing: "); // Output separator
  Serial.print(flowMilliLitres);
  Serial.print("mL/Sec");
  // Print the cumulative total of litres flowed since starting
  Serial.print(" Output Liquid Quantity: "); // Output separator
  Serial.print(totalMilliLitres);
  Serial.println("mL");
  Serial.println();
  //delay(1000);

  // Reset the pulse counter so we can start incrementing again
  pulseCount = 0;

  // Enable the interrupt again now that we've finished sending output
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
      
    
if(totalLitres <= 20000)
{
  bill = totalLitres * 0.42;  
}
else if(totalLitres >= 20100 && totalLitres <= 40000)
{
  bill = 8400 + ((totalLitres - 20000) * 0.65);
}
else if(totalLitres >= 40100 && totalLitres <= 60000)
{
  bill = 8400 + 12935 + ((totalLitres -40000 ) * 0.90) ;
}
else if(totalLitres >= 60100)
{
  bill = 8400 + 12935 + 17910 + ((totalLitres - 60000 ) * 1.00);
}
else 
{
  bill = 0.0;
}

  
  //Switch on the backlight
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
    
  // set the cursor to column 0, line 0
  // (note: line 0 is the first row, since counting begins with 0):
  lcd.begin(16, 2);
  lcd.print("Total : ");
  //lcd.print(totalMeterCube, DEC); 
  // print the Flow Rate
  lcd.print(totalLitres, 2);
  //String liter = String(totalLitres);
  //lcd.print(" m3");
  lcd.print(" L");
  lcd.setCursor(0, 1);
  lcd.print("Bill: RM");
  lcd.print(bill, 2);
  //String bil = String(bill);
  }

  ThingSpeak.setField(1, flowRate);
  ThingSpeak.setField(2, totalLitres);
  ThingSpeak.setField(3, bill);
  
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }

}

/*
I.S.R.
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
