#include <Wire.h> //LCD
#include <LiquidCrystal_I2C.h>

#include <OneWire.h>  //DS18B20

#include <VirtualWire.h> //433MHz

#include <avr/wdt.h>

#include <EEPROM.h>

const byte LED_on = 255;
const byte LED_off = 0;

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

const unsigned int Kotol_MinCasMedziDvomaZapnutiamiKotla =  45* 60;
const unsigned int Kotol_MinCasBehuKotla =  15* 60;
const unsigned int Kotol_CasDobehCerpadlo =  7* 60;
unsigned int KotolCasZapnutia, KotolCasVypnutia;
unsigned char KotolStatus, CerpadloStatus, Rele3Status;

const unsigned char Rele1_Pin = 7;
const unsigned char Rele2_Pin = 3;
const unsigned char Rele3_Pin = 6;

const unsigned char DS18S20Base_Pin = 5; //DS18S20 Signal pin on digital pin
OneWire dsBase(DS18S20Base_Pin); // on digital pin 2

const unsigned char rx433MHz_pin = 4;
const unsigned char tx433MHz_pin = 2;

uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;
const unsigned char buflen_433MHz = 12;

const unsigned int tx433MHz_UpdateTimePeriod = 1 * 60;
unsigned int tx433MHzUpdateTime;

const unsigned int rx433MHz_Timeout = 3 * 60;
unsigned int rx433MHzAgainIzba1;
unsigned int rx433MHzAgainIzba2;
unsigned int rx433MHzAgainVonku;
unsigned int rx433MHzAgainSauna;
unsigned int rx433MHzAgainRury;


const unsigned int lcd_Timeout = 1;
unsigned int lcdAgain;

const int TempNast_Min = 100;
const int TempNast_Max = 250;

unsigned int time;

int TempIzba1, HumidityIzba1;
int TimeoutIzba1;
int TempIzba2, HumidityIzba2;
int TimeoutIzba2;
int TempBase;
int TempNast;
int TempVonku, HumidityVonku, TimeoutVonku;
int SaunaTemp, SaunaNast, SaunaOhrev, TimeoutSauna;
int RuryKotolVystup, RuryKotolSpiatocka, RuryBojlerVystup, TimeoutRury;


const unsigned char rEncPinA = 8;  // Connected to CLK on KY-040
const unsigned char rEncPinB = 9;  // Connected to DT on KY-040
const unsigned char rEncPinButton = 11; // Connected to SW on KY-040
unsigned char rEncValALast;  
unsigned char rEncValButtonLast;

byte znak_ohrev_on[8] = {
  B01001,
  B10010,
  B01001,
  B10010,  
  B00000,
  B11111,
  B11111,
  B00000,
};

byte znak_ohrev_off[8] = {
  B00000,
  B00000,
  B00000,
  B00000,  
  B00000,
  B11111,
  B11111,
  B00000,
};






void setup()
{
  wdt_enable(WDTO_8S);

  Serial.begin(115200);	// Debugging only
  Serial.println(F("setup();"));

  lcd.begin(20,4);                      // initialize the lcd 
  lcd.clear();
  lcd.backlight();

  lcd.createChar(1, znak_ohrev_on);

  lcd.setCursor(0, 0);
  lcd.print(F("nast."));
  lcd.setCursor(7, 0);
  lcd.print(F("izba1"));
  lcd.setCursor(14, 0);
  lcd.print(F("izba2"));
  lcd.setCursor(0, 2);
  lcd.print(F("vonku"));
  lcd.setCursor(7, 2);
  lcd.print(F("sauna"));

  loadEEPROM();
  rEncInit();

  vw_set_rx_pin(rx433MHz_pin);
  vw_set_tx_pin(tx433MHz_pin);
  vw_setup(2000);	 // Bits per sec
  vw_rx_start();       // Start the receiver PLL running

  randomSeed(analogRead(0));

  if(rEncGetButton()==1){ 
    //nothing
  }

  initKotol();

  Serial.println(F("loop();"));
}

void loop()
{
  time = (unsigned long)(millis() / 1000);

  wdt_reset();

  check433MHz();

  checkNastTemp();

  if(lcdAgain<time){
    if((time > rx433MHzAgainIzba1) || (time > rx433MHzAgainIzba2)){
      float tmp = getTempBase();
      if(tmp != -1000) TempBase = tmp;
    }

    lcdAgain = time + lcd_Timeout;

    setKotol();

    vypisNastTemp();
    vypisOstatne();
    //    vypis_kotol();

    saveEEPROM();

    //vypisRam();

  }

  if(time > tx433MHzUpdateTime){
    tx433MHz();
    tx433MHzUpdateTime = time + tx433MHz_UpdateTimePeriod - random(10);
  }

}




void vypisKotol(void)
{

  if(KotolStatus == 1){
    lcd.createChar(0, znak_ohrev_on);
    lcd.setCursor(0, 1);
    lcd.write((byte)0);
  }
  //lcd.createChar(0, znak_ohrev_off);

  lcd.setCursor(0, 1);
  if((CerpadloStatus == 1) && (KotolStatus == 0)){
    lcd.createChar(0, znak_ohrev_off);
    lcd.setCursor(0, 1);
    lcd.write((byte)0);
  }

  if((CerpadloStatus == 0) && (KotolStatus == 0)){
    lcd.print(F(" "));
  }


}




void vypisOstatne(void)
{
  int ii = (millis()/1000) / (lcd_Timeout*2);
  ii = ii % 2;

  switch(ii){
  case 0:

    if(rx433MHzAgainIzba1 > time){
      lcd.setCursor(7, 1);
      vypisLcdTemp(TempIzba1);
    }
    else{ 
      lcd.setCursor(7, 1);
      vypisLcdTemp(TempBase);
    }

    lcd.setCursor(14, 1);
    if(rx433MHzAgainIzba2 > time){
      vypisLcdTemp(TempIzba2);
    }
    else{ 
      lcd.setCursor(14, 1);
      vypisLcdTemp(TempBase);
    }


    lcd.setCursor(0, 3);
    if(rx433MHzAgainVonku > time){
      vypisLcdTempInt(TempVonku);
    }
    else{ 
      vypisLcdNodata();
    }

    lcd.setCursor(7, 3);
    if(rx433MHzAgainSauna > time){
      vypisLcdTempInt(SaunaTemp);
    }
    else{ 
      vypisLcdNodata();
    }

    break;
  case 1:
    lcd.setCursor(7, 1);
    if(rx433MHzAgainIzba1 > time){
      vypisLcdHumidity(HumidityIzba1);
    }
    else{ 
      vypisLcdNodata();
    }

    lcd.setCursor(14, 1);
    if(rx433MHzAgainIzba2 > time){
      vypisLcdHumidity(HumidityIzba2);
    }
    else{ 
      vypisLcdNodata();
    }

    lcd.setCursor(0, 3);
    if(rx433MHzAgainVonku > time){
      vypisLcdHumidity(HumidityVonku);
    }
    else{ 
      vypisLcdNodata();
    }

    lcd.setCursor(7, 3);
    if(rx433MHzAgainSauna > time){
      vypisLcdSaunaNast(SaunaNast);
    }
    else{ 
      vypisLcdNodata();
    }

    break;
  }



  ii = (millis()/1000) / (lcd_Timeout*2);
  ii = ii % 3;

  lcd.setCursor(14, 2);
  switch(ii){
  case 0:
    lcd.print(F("kuren."));
    lcd.setCursor(14, 3);
    if(rx433MHzAgainRury > time){
      vypisLcdTempInt(RuryKotolVystup);
    }
    else{ 
      vypisLcdNodata();
    }

    break;

  case 1:
    lcd.print(F("spiat."));
    lcd.setCursor(14, 3);
    if(rx433MHzAgainRury > time){
      vypisLcdTempInt(RuryKotolSpiatocka);
    }
    else{ 
      vypisLcdNodata();
    }

    break;

  case 2:
    lcd.print(F("bojler"));
    lcd.setCursor(14, 3);
    if(rx433MHzAgainRury > time){
      vypisLcdTempInt(RuryBojlerVystup);
    }
    else{ 
      vypisLcdNodata();
    }

    break;

  }

}


void vypisRam(void)
{
  int fr;
  fr = freeRam();

  lcd.setCursor(0, 0);
  lcd.print(F("<"));
  lcd.print(fr);
  lcd.print(F("> "));
  //Serial.println(fr); 

  //lcd.setCursor(7, 0);
  lcd.print(F("<"));
  lcd.print(buflen);
  lcd.print(F("> "));
  //Serial.println(buflen); 

  lcd.setCursor(0, 2);
  lcd.print(F("<"));
  lcd.print(millis()/1000);
  lcd.print(F("> "));
  //Serial.println(buflen); 
}

void initKotol(void)
{
  pinMode(Rele1_Pin, OUTPUT);
  pinMode(Rele2_Pin, OUTPUT);
  pinMode(Rele3_Pin, OUTPUT);

  setKotol(0); 
  setCerpadlo(0); 
  setRele3(0); 

  KotolCasVypnutia = 0;

}

void setKotol(unsigned char value)
{
  KotolStatus = value;
  if(value==0){
    analogWrite(Rele1_Pin, 255);  
  } 
  else {
    analogWrite(Rele1_Pin, 0);  
  }
}


void setCerpadlo(unsigned char value)
{
  CerpadloStatus = value;
  if(value==0){
    analogWrite(Rele2_Pin, 255);  
  } 
  else {
    analogWrite(Rele2_Pin, 0);  
  }
}


void setRele3(unsigned char value)
{
  Rele3Status = value;
  if(value==0){
    analogWrite(Rele3_Pin, 255);  
  } 
  else {
    analogWrite(Rele3_Pin, 0);  
  }
}





void check433MHz(void){
  buflen = buflen_433MHz;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    //Message with a good checksum received, dump it.
    //Serial.println(F("VW_GET_MESSAGE"));
    int i;

    led(LED_on);
    Serial.println(F("Received433MHz: "));	
    Serial.print(F(" buflen="));        
    Serial.println(buflen);   
    Serial.print(F(" ASC: "));	
    for (i = 0; i < buflen; i++)
    {
      char c;
      c = buf[i];
      Serial.print(c);
      Serial.print(F(" "));
    }
    Serial.println();
    //         Serial.print(F(" DEC: "));	
    //    for (i = 0; i < buflen; i++)
    //    {
    //      Serial.print(buf[i], DEC);
    //      Serial.print(F(" "));
    //    }
    //    Serial.println();


    if((buf[0]=='T') && (buf[1]=='H') &&
      (buf[2]=='S') && (buf[10] == 0)){

      if(buf[3]=='1'){
        rx433MHzAgainIzba1 = time + rx433MHz_Timeout;
        TempIzba1 = convrxnum(buf[4], buf[5]);
        HumidityIzba1 = convrxnum(buf[6], buf[7]); 
        TimeoutIzba1 = buf[11];
      }

      if(buf[3]=='2'){
        rx433MHzAgainIzba2 = time + rx433MHz_Timeout;
        TempIzba2 = convrxnum(buf[4], buf[5]); 
        HumidityIzba2 = convrxnum(buf[6], buf[7]);
        TimeoutIzba2 = buf[11];
      }
    }


    if((buf[0]=='T') && (buf[1]=='H') &&
      (buf[2]=='E')){

      if(buf[3]=='1'){
        rx433MHzAgainVonku = time + rx433MHz_Timeout;
        TempVonku = convrxnum(buf[4], buf[5]); 
        HumidityVonku = convrxnum(buf[6], buf[7]);
        TimeoutVonku = buf[10];
      }

      if(buf[3]=='2'){
        rx433MHzAgainSauna = time + rx433MHz_Timeout;
        SaunaTemp = convrxnum(buf[4], buf[5]);
        SaunaNast = convrxnum(buf[6], buf[7]);
        SaunaOhrev = buf[8];
        TimeoutSauna = buf[10];
      }

      if(buf[3]=='3'){
        rx433MHzAgainRury = time + rx433MHz_Timeout;
        RuryKotolVystup = convrxnum(buf[4], buf[5]);
        RuryKotolSpiatocka = convrxnum(buf[6], buf[7]);
        RuryBojlerVystup = convrxnum(buf[8], buf[9]);
        TimeoutRury = buf[10];
      }
    }

    led(LED_off);

  }

}


void rEncInit(void)
{
  pinMode(rEncPinA,INPUT);
  pinMode(rEncPinB,INPUT);
  pinMode(rEncPinButton,INPUT);

  rEncValALast = digitalRead(rEncPinA);   
}


int rEncGetValue(void){
  int rEncValA;
  int rEncValB;
  int ret=0;


  rEncValA = digitalRead(rEncPinA);
  rEncValB = digitalRead(rEncPinB);

  if (rEncValA != rEncValALast){ // Means the knob is rotating
    // if the knob is rotating, we need to determine direction
    // We do that by reading pin B.
    if (rEncValB != rEncValA) {  // Means pin A Changed first - We're Rotating Clockwise
      ret = -1;
    } 
    else {// Otherwise B changed first and we're moving CCW
      ret = 1;
    }
  } 
  rEncValALast = rEncValA;

  return(ret);
}

int rEncGetButton(void){
  wdt_reset();
  int rEncValButton = 1 - digitalRead(rEncPinButton);
  if (rEncValButton != rEncValButtonLast){ 
    //Serial.print (F("Button: "));
    Serial.println(rEncValButton);
  } 
  rEncValButtonLast = rEncValButton;
  return rEncValButton;
}



void checkNastTemp(void){
  int tmp_rEncGetValue = rEncGetValue();

  if(tmp_rEncGetValue!=0){ 
    //Serial.println(tmp);

    if(tmp_rEncGetValue == 1){ 
      TempNast--;      

    }
    if(tmp_rEncGetValue == -1){ 
      TempNast++;      
    }

    TempNast = constrain(TempNast,TempNast_Min,TempNast_Max);
    vypisNastTemp();
  }
}


void loadEEPROM(void)
{
  //format
  //TempNast 10

  TempNast = constrain(EEPROM.read(10), TempNast_Min, TempNast_Max);
  Serial.print(F("TempNast= "));
  Serial.println(TempNast);
}



void saveEEPROM(void)
{
  EEPROMupdate(10, TempNast);  
}


//user function
void EEPROMupdate(int address, unsigned char value)
{
  unsigned char old = EEPROM.read(address);

  if(old!=value){
    EEPROM.write(address, value);  
  }
}



float getTempBase(){
  //returns the temperature from one DS18S20 in DEG Celsius

  unsigned char data[12];
  unsigned char addr[8];

  if ( !dsBase.search(addr)) {
    //no more sensors on chain, reset search
    dsBase.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println(F("CRC is not valid!"));
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print(F("Device is not recognized"));
    return -1000;
  }

  dsBase.reset();
  dsBase.select(addr);
  dsBase.write(0x44); // start conversion

  unsigned char present = dsBase.reset();
  dsBase.select(addr);
  dsBase.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = dsBase.read();
  }

  dsBase.reset_search();

  unsigned char MSB = data[1];
  unsigned char LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  if (data[8] != OneWire::crc8(data,8)) {
    Serial.print(F("ERROR: CRC didn't match\n"));
    return -1000;
  }

  return TemperatureSum * 10;

}






char rEncGetChar( int direct, int input){
  int old_input = input;

  Serial.println(input);
  if(direct==1){
    input--;
    if(input==97-1) input=90;
    if(input==65-1) input=57;
    if(input==48-1) input=32;
    if(input==32-1) input=122;
  }
  else if(direct==-1){
    input++;
    if(input==32+1) input=48;
    if(input==57+1) input=65;
    if(input==90+1) input=97;
    if(input==122+1) input=32;
  }

  if(old_input == input){ //no change as input is out of range, spacing
    input = 32; 
  }

  Serial.println(input);
  return input;
}




int StringContains(String s, String search) {
  int max = s.length() - search.length();
  int lgsearch = search.length();

  for (int i = 0; i <= max; i++) {
    if (s.substring(i, i + lgsearch) == search) return i;
  }

  return -1;
}






void vypisLcdTemp(int input){
  if((input<100) && (input!=0)){
    lcd.print(F(" "));
  }
  lcd.print(input/10);
  lcd.print(F("."));
  lcd.print(input%10);
  lcd.write(B11011111); //stupen
}




void vypisLcdHumidity(int input){
  lcd.print(F(" "));
  if((input<100) && (input!=0)){
    lcd.print(F(" ")); 
  }
  lcd.print(input/10);
  lcd.print(F("% "));
}






void vypisLcdSaunaNast(int input){
  lcd.write((byte)1);
  if((input<100) && (input!=0)){
    lcd.print(F(" ")); 
  }
  lcd.print(input/10);
  lcd.write(B11011111); //stupen
  lcd.print(F(" "));
}





void vypisLcdTempInt(int input){
    lcd.print(F(" "));
  if((input<100) and (input>-100)){
    lcd.print(F(" "));
  }
  if(input>0){
    lcd.print(F(" "));
  } 
  lcd.print(input/10);
  lcd.write(B11011111); //stupen
  lcd.print(F(" "));
}


void vypisNastTemp(void){

  lcd.setCursor(0, 1);
  if(TempNast<100){
    lcd.print(F(" "));
  }
  lcd.print(TempNast/10);
  lcd.print(F("."));
  lcd.print(TempNast%10);  
  lcd.write(B11011111); //stupen
}


void vypisLcdNodata(void){
  lcd.print(F(" --  "));
}




void tx433MHz(void)
{
  led(LED_on);
  Serial.println(F("tx433MHz();"));
  tx433MHzPacket(1);
  tx433MHzPacket(2);
  tx433MHzPacket(3);
  led(LED_off);

}


void tx433MHzPacket(unsigned int packetNum)
{

  char msg[12] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '                                                                                                                       };

  msg[0]='T';
  msg[1]='H';
  msg[2]='B';
  msg[3]= packetNum + 48;

  if(packetNum == 1){
    msg[4]= TempIzba1 / 10;
    msg[5]= TempIzba1 % 10;
    msg[6]= HumidityIzba1 / 10;
    msg[7]= HumidityIzba1 % 10;

    if(rx433MHzAgainIzba1 > time){ 
      msg[10]= 1; 
    } 
    else { 
      msg[10]= 0; 
    } 
  }

  if(packetNum == 2){
    msg[4]= TempIzba2 / 10;
    msg[5]= TempIzba2 % 10;
    msg[6]= HumidityIzba2 / 10;
    msg[7]= HumidityIzba2 % 10;

    if(rx433MHzAgainIzba2 > time){ 
      msg[10]= 1; 
    } 
    else { 
      msg[10]= 0; 
    }  
  }

  if(packetNum == 3){
    msg[4]= TempBase / 10;
    msg[5]= TempBase % 10;
    msg[6]= TempNast / 10;
    msg[7]= TempNast % 10;
    msg[8]= KotolStatus;
    msg[9]= CerpadloStatus;
    msg[10]= Rele3Status;  
  }

  vw_send((uint8_t *)msg, buflen_433MHz);
  vw_wait_tx(); // Wait until the whole message is gone
}






void setKotol(void)
{
  int tmp_base, tmp_base1, tmp_base2;

  tmp_base1 = TempIzba1;
  if(time > rx433MHzAgainIzba1){
    tmp_base1 = TempBase;
    //Serial.println(F("nebezi Izba1"));
  }

  tmp_base2 = TempIzba2;
  if(time > rx433MHzAgainIzba2){
    tmp_base2 = TempBase;
    //Serial.println(F("nebezi Izba2"));
  } 

  //vyberie nizsiu teplotu z oboch senzorov, resp. nahrada za hlavny senzor
  tmp_base = min(tmp_base1, tmp_base2); //vyssiu z oboch izieb
  tmp_base = min(TempBase, tmp_base);  //porovnat aj s bazou


  if(TempNast > tmp_base){
    //Serial.println("+"); 
  }
  else {
    //Serial.println("_"); 
  }    


  if(TempNast > tmp_base){
    if(KotolStatus == 0){
      if((time - KotolCasVypnutia > Kotol_MinCasMedziDvomaZapnutiamiKotla) || (KotolCasVypnutia == 0)){
        setKotol(1); 
        KotolCasZapnutia = time;
        Serial.println(F("Kotol zapnutie"));
      }
      else {   
        Serial.println(F("Kotol kratky cas medzi dvoma zapnutiami"));
      }
    }
  } 
  else {
    if(KotolStatus == 1){
      if(time - KotolCasZapnutia > Kotol_MinCasBehuKotla){
        setKotol(0); 
        KotolCasVypnutia = time;
        Serial.println(F("Kotol vypnutie"));
      } 
      else {
        Serial.println(F("Kotol kratky cas zapnutia"));
      }
    }
  }

  if(KotolStatus == 1){
    if(CerpadloStatus == 0){ 
      setCerpadlo(1);
      Serial.println(F("Cerpadlo zapnutie"));
    }
  }
  else {
    if(CerpadloStatus == 1){
      if(time - KotolCasVypnutia > Kotol_CasDobehCerpadlo){
        setCerpadlo(0);
        Serial.println(F("Cerpadlo vypnutie"));
      }
      else {   
        Serial.println(F("Cerpadlo dobeh"));
      } 
    }
  }
}




int convrxnum(uint8_t bufh, uint8_t bufl){
  if(bufh > 127){ 
    return -( 10 * (256 - (int)bufh ) + (256 - (int)bufl) );
  } 
  else {
    return (10* (int)bufh + (int)bufl);  
  }
}



void led(byte value){
  const unsigned char LED_pin=13;

  if(value>0){
    value=LED_on;
  }
  pinMode(LED_pin, OUTPUT);
  analogWrite(LED_pin, value);
}




int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}









































