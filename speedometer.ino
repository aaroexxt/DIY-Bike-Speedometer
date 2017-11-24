//calculations
//tire radius ~ 13.5 inches
//circumference = pi*2*r =~85 inches
//max speed of 35mph =~ 616inches/second
//max rps =~7.25

#include <LiquidCrystal.h>
#include "DHT.h"
#include <EEPROM.h>

#define reed A0//pin connected to read switch
#define temp A1 //pin to dht11 temp
#define lowBattLED 10 //pin to low batt led1
#define temptype DHT11 //temperature sensor type
#define button A2 //pin to button

//storage variables
//SERIAL ENABLE!!
boolean SerialEnabled = true;
//SERIAL ENABLE!!^
int reedVal;
long timer;// time between one full rotation (in ms)
int mph;
int oldmph;
float radius = 13.775;// tire radius (in inches)
int maxReedTime = 2000; //maximum time between pulses
float circumference;

float odo;
float oldodo;

float oldf;
float oldhi;

float oldV;
float oldPercent;

int rpm;

long ridetime;

int maxReedCounter = 10;//min time (in ms) of one rotation (for debouncing)
int reedCounter;

int buttonState;
float buttonThreshVoltage = 3.75; //threshold voltage for button
int buttonThresh = (buttonThreshVoltage/5.0)*1024;
int trueButtonState = 0; //0 speed screen 1 temp screen 2 time screen 3 voltage screen
int maxButtonCounter = 10;
int buttonCounter = 0;
boolean justSwitched = false;

boolean initprint = false;
boolean printonce = false;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2); //lcd object
DHT dht(temp, temptype); //temp object

void setup(void) {
  reedCounter = maxReedCounter;
  circumference = 2 * 3.14 * radius;
  oldmph = -1; //setup oldmph to draw immediately
  pinMode(reed, INPUT);
  pinMode(button, INPUT);
  pinMode(lowBattLED, OUTPUT); 
  lcd.begin(8, 2);
  dht.begin();
  if (SerialEnabled == true) {Serial.begin(9600);}

  // TIMER SETUP- the timer interrupt allows precise timed measurements of the reed switch
  //for more info about configuration of arduino timers see http://arduino.cc/playground/Code/Timer1
  cli();//stop interrupts

  //set timer1 interrupt at 1kHz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;
  // set timer count for 1khz increments
  OCR1A = 1999;// = (1/1000) / ((1/(16*10^6))*8) - 1
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS11 bit for 8 prescaler
  TCCR1B |= (1 << CS11);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);

  sei();//allow interrupts
  //END TIMER SETUP

}


ISR(TIMER1_COMPA_vect) {//Interrupt at freq of 1kHz to measure reed switch
  reedVal = digitalRead(reed);//get val of A0
  if (reedVal) { //if reed switch is closed
    if (reedCounter == 0) { //min time between pulses has passed
      mph = (56.8 * float(circumference)) / float(timer); //calculate miles per hour
      rpm = (float(60000)) / (float(timer));
      odo += ((float(mph) / 60.0 / 60.0 / 1000.0) * float(timer)); //calculate miles moved and add to odometer; miles per milisecond*timer in ms
      timer = 0;//reset timer
      reedCounter = maxReedCounter;//reset reedCounter
    }
    else {
      if (reedCounter > 0) { //don't let reedCounter go negative
        //reedCounter -= 1;//decrement reedCounter
      }
    }
  }
  else { //if reed switch is open
    if (reedCounter > 0) { //don't let reedCounter go negative
      reedCounter -= 1;//decrement reedCounter
    }
  }
  if (timer > maxReedTime) {
    mph = 0;//if no new pulses from reed switch- tire is still, set mph, rpm and kph to 0
    rpm = 0;
    ridetime -= maxReedTime;
  }
  else {
    timer += 1;//increment timer
    ridetime += 1;
  }
}

void loop(void) {
  // picture loop

  if (millis() < 1000) { // The time in milli seconds, for you want to show the splash screen 6000 = 6 Seconds
    if (initprint == false) {
      if (SerialEnabled == true) { Serial.println("initv2");}
      lcd.setCursor(0,0);
      lcd.print("Welcome!");
      lcd.setCursor(0,1);
      lcd.print("B.OS. V2");
      digitalWrite(lowBattLED,HIGH);
      delay(200);
      digitalWrite(lowBattLED,LOW);
      delay(200);
      digitalWrite(lowBattLED,HIGH);
      delay(200);
      digitalWrite(lowBattLED,LOW);
      initprint = true;
    }
  } else {
    if (printonce == false) {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("MPH: 0");
      lcd.setCursor(0,1);
      lcd.print("ODO:0.00");
      lcd.print(String(odo).substring(0,4));
      printonce = true;
    }
    //button reading
  buttonState = analogRead(button);
  if (buttonState >= buttonThresh) {
    if (buttonCounter == 0) {
      buttonCounter = maxButtonCounter;
      oldodo = -1; //ensure that screen is printed to at least once
      oldmph = -1;
      oldf = -1;
      oldhi = -1;
      oldV = -1;
      oldPercent = -1;
      if (trueButtonState < 3) {
        trueButtonState++;
        justSwitched = true;
        lcd.clear();
      } else {
        trueButtonState = 0;
        justSwitched = true;
        lcd.clear();
      }
    }
    //don't count down button counter if button is pressed
  } else {
    if (buttonCounter > 0) {
      buttonCounter -= 1;
    }
  }
    if (SerialEnabled) {
      Serial.print("TBS: ");
      Serial.print(trueButtonState);
      Serial.print(", BTNV: ");
      Serial.println(buttonState);
    }

    if (trueButtonState == 0) {
      renderMPH();
      justSwitched = false;
    } else if (trueButtonState == 1) {
      renderTMP();
      justSwitched = false;
    } else if (trueButtonState == 2) {
      renderTIM();
      justSwitched = false;
    } else if (trueButtonState == 3) {
      renderVLT();
      justSwitched = false;
    } else {
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("OpMode");
      lcd.setCursor(0,1);
      lcd.print("NotFound");
    }
    //Serial.println(analogRead(A3));
    if (analogRead(A3) < 512) {
      digitalWrite(lowBattLED,HIGH);
    } else {
      digitalWrite(lowBattLED,LOW);
    }
    // rebuild the picture after some delay
    delay(50);
    //delay(25);
  }



}

void renderMPH() {
  if (oldmph != mph || oldodo != odo && SerialEnabled == true) {
    Serial.print("RPM: ");
    Serial.print(rpm);
    //Serial.print(" ");
    Serial.print(", MPH: ");
    Serial.print(mph);
    //Serial.print(" ");
    Serial.print(", ODO: ");
    Serial.println(odo, 3);
  }
  if (oldmph != mph || justSwitched == true) {
    lcd.setCursor(0,0);
    lcd.print("        ");
    lcd.setCursor(0,0);
    lcd.print("MPH: ");
    //lcd.setCursor(0,4);
    lcd.print(mph);
  }
  if (oldodo != odo || justSwitched == true) {
    lcd.setCursor(0,1);
    lcd.print("        ");
    lcd.setCursor(0,1);
    lcd.print("ODO:");
    //lcd.setCursor(0,4);
    lcd.print(String(odo).substring(0,4));
  }
  oldmph = mph;
  oldodo = odo;
}

void renderTMP() {
  //temp reading
  float h = dht.readHumidity();
  float f = dht.readTemperature(true);
  if (isnan(h)) {
    h = -1.0;
  }
  if (isnan(f)) {
    f = -1.0;
  }
  if (f > -1 && h > -1) {
    float hi = dht.computeHeatIndex(f, h);
    oldhi = hi;
    if (oldf != f || justSwitched == true) {
      lcd.setCursor(0,0);
      lcd.print("F: ");
      lcd.print(f);
    }
    if (oldhi != hi || justSwitched == true) {
      lcd.setCursor(0,1);
      lcd.print("HI: ");
      lcd.print(hi);
    }
  } else {
    lcd.clear();
    lcd.print("Error");
    lcd.setCursor(0,1);
    lcd.print("reading");
  }
  oldf = f;
  if (SerialEnabled == true) {
    Serial.print("F: ");
    Serial.print(f);
    Serial.print(", H: ");
    Serial.println(h);
  }
}

void renderTIM() { //render time screen: ride time and total time
 lcd.setCursor(0,0);
 lcd.print("H");
 lcd.print(round((millis()/1000.0)/3600));
 lcd.print("M");
 lcd.print(round((millis()/1000.0)/60));
 lcd.print("S");
 lcd.print(round(millis()/1000.0));
 lcd.setCursor(0,1);
 lcd.print("H");
 lcd.print(round((ridetime/1000.0)/3600));
 lcd.print("M");
 lcd.print(round((ridetime/1000.0)/60));
 lcd.print("S");
 lcd.print(round(ridetime/1000.0));
}

void renderVLT() { //render voltage/battery life screen
  long vcc = readVcc();
  double vccConv = round((vcc/1000.0));
  double percent = round((vccConv/4.2))*100;
  Serial.println(percent);
  if (abs(oldV-vccConv) > 0.02 || justSwitched == true) { //check if voltage change is more than 0.02 volts 
    lcd.setCursor(0,0);
    lcd.print("V: ");
    lcd.print(String(vccConv).substring(0,1));
    lcd.print(".");
    lcd.print(String(vccConv).substring(1,2));
  }
  if (abs(oldPercent-percent) > 0.05 || justSwitched == true) {
    lcd.setCursor(0,1);
    lcd.print("%: ");
    if (percent < 10) {
      lcd.print(String(percent).substring(0,1));
      lcd.print(".");
      lcd.print(String(percent).substring(1,2));
    } else if (percent < 100) {
      lcd.print(String(percent).substring(0,2));
      lcd.print(".");
      lcd.print(String(percent).substring(2,3));
    } else {
      lcd.print(String(percent).substring(0,3));
      lcd.print(".");
      lcd.print(String(percent).substring(3,4));
    }
  }
  
  oldV = vccConv;
  oldPercent = percent;
}

long readVcc() {
  long result; // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1); delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  return result;
}

String readEEPROM(int start, int amount){
   String total = "";
   int current = 0;
   for (unsigned int r = 0; r<amount; r++) {
     current = EEPROM.read(r+start);
    total = total + String(current);
    //Serial.print(current);
   }
   return total;
}

void EEPROM2SERIAL(int start, int amount){
   int current = 0;
   for (unsigned int r = 0; r<amount; r++) {
    current = EEPROM.read(r+start);
    Serial.print(current);
   }
   Serial.println("");
}

void writeEEPROM(int start, String value) {
  int tmp;
  for (unsigned int r = 0; r<value.length(); r++) {
    tmp = value.substring(r,r+1).toInt();
    EEPROM.write(r+start,tmp);
  }
}

