#include <Wire.h>
#include <LiquidCrystal_I2C.h>


/* Water Flow sensor Arduino sketch by Developer Cats */
volatile unsigned long nbTopsFan; // measuring the rising edges of the signal
unsigned long nbTopsFan_old;

float calc;

int hallSensor = 2; // sensor digital pin
int i;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);


void setup()
{

  lcd.begin(16, 2);              // initialize the lcd
  lcd.noCursor();
  lcd.backlight();


  lcd.home();                   // go home
  lcd.print("WaterFlow sensor");

  pinMode(hallSensor, INPUT); // sets the digital pin 2 as the sensor's input
  attachInterrupt(digitalPinToInterrupt(hallSensor), rpm, RISING); // attach interrupt to trigger when value rises

}

void loop()
{
  if (nbTopsFan != nbTopsFan_old) {
    lcd.setCursor ( 0, 1 );        // go to the next line
    calc = ((float)nbTopsFan * 1000 / 60 / 5.5); // (Pulse frequency x 60) / 5.5Q, = flow rate in L/hour (as in vendor specifications)
    lcd.print(calc);
    lcd.print (" ml");
    nbTopsFan_old = nbTopsFan;
  }
}



void rpm() // function that interupt triggers
{
  nbTopsFan++; // hall effect sensors signal measure
}

