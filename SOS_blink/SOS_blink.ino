#include <avr/wdt.h>

const int K = 100;
const int D = 300;
const int medzera = 500;

//void(* resetFunc) (void) = 0;//declare reset function at address 0

void setup() {                
  pinMode(13, OUTPUT);     
  
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
//  znak(K);
  
  wdt_enable(WDTO_8S);
  //wdt_reset();

}

void loop() {
  
  //while(1);
  znak(K);
  znak(K);
  znak(K);
  znak(D);
  znak(D);
  znak(D);
  znak(K);
  znak(K);
  znak(K);
  delay(medzera*10);
//  resetFunc(); //call reset  
}

void znak(int dlzka){
  digitalWrite(13, HIGH);   // set the LED on
  delay(dlzka);              // wait for a second
  digitalWrite(13, LOW);    // set the LED off
  delay(medzera);              // wait for a second
}
