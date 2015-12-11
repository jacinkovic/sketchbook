#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#include <OneWire.h>

#include <VirtualWire.h> //433MHz

#include <avr/wdt.h>

#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const unsigned long Kotol_MinCasMedziDvomaZapnutiamiKotla =  2* 60*1000L;
const unsigned long Kotol_MinCasBehuKotla =  1* 60*1000L;
const unsigned long Kotol_CasDobehCerpadlo =  1* 60*1000L;
unsigned long KotolCasZapnutia, KotolCasVypnutia;
int KotolStatus, CerpadloStatus, Rele3Status;

const int Rele1_Pin = 7;
const int Rele2_Pin = 3;
const int Rele3_Pin = 6;

int DS18S20Base_Pin = 5; //DS18S20 Signal pin on digital pin
OneWire dsBase(DS18S20Base_Pin); // on digital pin 2

const int rx433MHz_pin = 4;
const int tx433MHz_pin = 2;
//byte txcount433MHz = 1;
uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;

const long int tx433MHz_UpdateTimePeriod = 20 *1000L;
long int tx433MHzUpdateTime;

const unsigned long rx433MHz_Timeout = 1 * 60 * 1000L;
unsigned long rx433MHzAgainIzba1;
unsigned long rx433MHzAgainIzba2;
unsigned long rx433MHzAgainVonku;
unsigned long rx433MHzAgainSauna;
unsigned long rx433MHzAgainRury;


const unsigned long lcd_Timeout = 3 * 1000L;
unsigned long lcdAgain;

const int TempNast_Min = 100;
const int TempNast_Max = 250;

unsigned long time;

int TempIzba1, HumidityIzba1;
int TimeoutIzba1;
int TempIzba2, HumidityIzba2;
int TimeoutIzba2;
int TempBase;
int TempNast;
int TempVonku, HumidityVonku, TimeoutVonku;
int SaunaTemp, SaunaNast, SaunaOhrev, TimeoutSauna;
int RuryKotolVystup, RuryKotolSpiatocka, RuryBojlerVystup, TimeoutRury;


int rEncPinA = 8;  // Connected to CLK on KY-040
int rEncPinB = 9;  // Connected to DT on KY-040
int rEncPinButton = 11; // Connected to SW on KY-040
int rEncValALast;  
int rEncValButtonLast;




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
  Serial.println(F("setup()"));

  lcd.init();                      // initialize the lcd 
  lcd.clear();
  lcd.backlight();

  lcd.createChar(1, znak_ohrev_on);

  lcd.setCursor(7, 2);
  lcd.print(F("izba2"));
  lcd.setCursor(15, 0);
  lcd.print(F("vonku"));
  lcd.setCursor(15, 2);
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

  init_kotol();

  //time = millis();
  //  rx433MHzAgainIzba1 = time;
  //  rx433MHzAgainIzba2 = time;
  //  ethAgain_Vonku = time;
  //  ethAgain_Sauna = time;
  //  lcdAgain = time;

  Serial.println(F("loop()"));
}

void loop()
{
  time = millis();

  wdt_reset();

  check433MHz();

  checkNastTemp();

  if(lcdAgain<time){
    if(time > rx433MHzAgainIzba1){
      float tmp = getTempBase();
      if(tmp != -1000) TempBase = tmp;
    }

    lcdAgain = time + lcd_Timeout;

    set_kotol();

    vypis_nastTemp();
    vypis_cidla();
    vypis_kotol();

    saveEEPROM();
  }

  if(millis() > tx433MHzUpdateTime){
    tx433MHz();
    tx433MHzUpdateTime = millis() + tx433MHz_UpdateTimePeriod - random(10000);
  }
}




void vypis_cidla(void)
{

  long ii;
  ii = millis()/lcd_Timeout;
  ii=ii%2;


  switch(ii){
  case 0:

    if(rx433MHzAgainIzba1 > time){
      lcd.setCursor(7, 0);
      lcd.print(F("izba1"));
      lcd.setCursor(7, 1);
      vypis_lcd_temp(TempIzba1);
    }
    else{ 
      lcd.setCursor(7, 0);
      lcd.print(F("nudz."));
      lcd.setCursor(7, 1);
      vypis_lcd_temp(TempBase);
    }

    lcd.setCursor(7, 3);
    if(rx433MHzAgainIzba2 > time){
      vypis_lcd_temp(TempIzba2);
    }
    else{ 
      vypis_lcd_nodata();
    }


    lcd.setCursor(15, 1);
    if(rx433MHzAgainVonku > time){
      vypis_lcd_temp(TempVonku);
    }
    else{ 
      vypis_lcd_nodata();
    }

    lcd.setCursor(15, 3);
    if(rx433MHzAgainSauna > time){
      vypis_lcd_temp(SaunaTemp);
    }
    else{ 
      vypis_lcd_nodata();
    }

    break;
  case 1:
    lcd.setCursor(7, 1);
    if(rx433MHzAgainIzba1 > time){
      vypis_lcd_humidity(HumidityIzba1);
    }
    else{ 
      vypis_lcd_nodata();
    }

    lcd.setCursor(7, 3);
    if(rx433MHzAgainIzba2 > time){
      vypis_lcd_humidity(HumidityIzba2);
    }
    else{ 
      vypis_lcd_nodata();
    }

    lcd.setCursor(15, 1);
    if(rx433MHzAgainVonku > time){
      vypis_lcd_humidity(HumidityVonku);
    }
    else{ 
      vypis_lcd_nodata();
    }

    lcd.setCursor(15, 3);
    if(rx433MHzAgainSauna > time){
      vypis_lcd_saunaNast(SaunaNast);
    }
    else{ 
      vypis_lcd_nodata();
    }

    break;
  }

}





void init_kotol(void)
{
  pinMode(Rele1_Pin, OUTPUT);
  pinMode(Rele2_Pin, OUTPUT);
  pinMode(Rele3_Pin, OUTPUT);

  set_kotol(0); 
  set_cerpadlo(0); 
  set_rele3(0); 

  KotolCasVypnutia = 0;

}

void set_kotol(int value)
{
  KotolStatus = value;
  if(value==0){
    analogWrite(Rele1_Pin, 255);  
  } 
  else {
    analogWrite(Rele1_Pin, 0);  
  }
}


void set_cerpadlo(int value)
{
  CerpadloStatus = value;
  if(value==0){
    analogWrite(Rele2_Pin, 255);  
  } 
  else {
    analogWrite(Rele2_Pin, 0);  
  }
}


void set_rele3(int value)
{
  Rele3Status = value;
  if(value==0){
    analogWrite(Rele3_Pin, 255);  
  } 
  else {
    analogWrite(Rele3_Pin, 0);  
  }
}


void vypis_kotol(void)
{

  if(KotolStatus == 1){
    //    lcd.print("k");
    //    lcd.print("k");
    //    lcd.print("k");
    lcd.createChar(0, znak_ohrev_on);
  }
  else{
    //  lcd.print("___");
    lcd.createChar(0, znak_ohrev_off);
  }
  lcd.setCursor(1, 3);
  lcd.write((byte)0);
  lcd.write((byte)0);
  lcd.write((byte)0);


  lcd.setCursor(0, 3);
  if(CerpadloStatus == 1){
    lcd.print("c");
  }
  else{ 
    lcd.print("_"); 
  }


  lcd.setCursor(4, 3);
  if(Rele3Status == 1){
    lcd.print("r");
  }
  else{ 
    lcd.print(" "); 
  }

}



void check433MHz(void){
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    //Message with a good checksum received, dump it.
    //Serial.println(F("VW_GET_MESSAGE"));
    //      int i;
    //      Serial.println(F("Received433MHz: "));	
    //      Serial.print(F(" ASC: "));	
    //      for (i = 0; i < buflen; i++)
    //      {
    //        char c;
    //        c = buf[i];
    //        Serial.print(c);
    //        Serial.print(" ");
    //      }
    //      Serial.println();
    //         Serial.print(F(" DEC: "));	
    //    for (i = 0; i < buflen; i++)
    //    {
    //      Serial.print(buf[i], DEC);
    //      Serial.print(" ");
    //    }
    //    Serial.println();


    if((buf[0]=='T') && 
      (buf[1]=='H') &&
      (buf[2]=='S') &&
      (buf[3]=='1') &&
      (buf[10] == 0)){

      rx433MHzAgainIzba1 = time + rx433MHz_Timeout;

      TempIzba1 = (10*buf[4]+buf[5]); 
      HumidityIzba1 = (10*buf[6]+buf[7]); 
      TimeoutIzba1 = buf[11];
    }

    if((buf[0]=='T') && 
      (buf[1]=='H') &&
      (buf[2]=='S') &&
      (buf[3]=='2') &&
      (buf[10] == 0)){

      rx433MHzAgainIzba2 = time + rx433MHz_Timeout;

      TempIzba2 = (10*buf[4]+buf[5]); 
      HumidityIzba2 = (10*buf[6]+buf[7]); 
      TimeoutIzba2 = buf[11];
    }


    if((buf[0]=='T') && 
      (buf[1]=='H') &&
      (buf[2]=='E') &&
      (buf[3]=='1')){

      rx433MHzAgainVonku = time + rx433MHz_Timeout;

      TempVonku = (10*buf[4]+buf[5]); 
      HumidityVonku = (10*buf[6]+buf[7]); 
      TimeoutVonku = buf[10];
    }



    if((buf[0]=='T') && 
      (buf[1]=='H') &&
      (buf[2]=='E') &&
      (buf[3]=='2')){

      rx433MHzAgainSauna = time + rx433MHz_Timeout;

      SaunaTemp = (10*buf[4]+buf[5]); 
      SaunaNast = (10*buf[6]+buf[7]); 
      SaunaOhrev = buf[8];
      TimeoutSauna = buf[10];
    }


 if((buf[0]=='T') && 
      (buf[1]=='H') &&
      (buf[2]=='E') &&
      (buf[3]=='3')){

      rx433MHzAgainRury = time + rx433MHz_Timeout;

      RuryKotolVystup = (10*buf[4]+buf[5]); 
      RuryKotolSpiatocka = (10*buf[6]+buf[7]); 
      RuryBojlerVystup = (10*buf[8]+buf[9]);
      TimeoutRury = buf[10];
    }


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
    Serial.print (F("Button: "));
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
    vypis_nastTemp();
  }
}


void loadEEPROM(void)
{
  //format
  //TempNast 10
  //wifiName 20-29
  //wifiPass 30-39

  TempNast = constrain(EEPROM.read(10), TempNast_Min, TempNast_Max);
  Serial.print(F("TempNast= "));
  Serial.println(TempNast);

  for(int i=0; i<=9; i++){
    //wifiNamePass[0][i] = EEPROM.read(i+20);
    //wifiNamePass[1][i] = EEPROM.read(i+30);
  }

}



void saveEEPROM(void)
{
  //set initials
  //  for(int i=0; i<=9; i++){
  //    wifiNamePass[0][i]=char(i+65);
  //    wifiNamePass[1][i]=char(i+48);
  //  }


  EEPROMupdate(10, TempNast);  
  for(int i=0; i<=9; i++){
    //EEPROMupdate(i+20, wifiNamePass[0][i]);
    //EEPROMupdate(i+30, wifiNamePass[1][i]);
  }

}


//user function
void EEPROMupdate(int address, byte value)
{
  byte old = EEPROM.read(address);

  if(old!=value){
    EEPROM.write(address, value);  
    //    Serial.print(F("EEPROM saved at: "));
    //    Serial.print(address);
    //    Serial.print(F(" value: "));
    //    Serial.println(value);
  }
}



float getTempBase(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

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

  byte present = dsBase.reset();
  dsBase.select(addr);
  dsBase.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = dsBase.read();
  }

  dsBase.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

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






void vypis_lcd_temp(int input){
  if((input<100) && (input!=0)){
    lcd.print(F(" "));
  }
  lcd.print(input/10);
  lcd.print(F("."));
  lcd.print(input%10);
  lcd.write(B11011111); //stupen
  //lcd.print(F(" "));
}




void vypis_lcd_humidity(int input){
  lcd.print(F(" "));
  if((input<100) && (input!=0)){
    lcd.print(F(" ")); 
  }
  lcd.print(input/10);
  lcd.print(F("% "));
}






void vypis_lcd_saunaNast(int input){
  lcd.write((byte)1);
  if((input<100) && (input!=0)){
    lcd.print(F(" ")); 
  }
  lcd.print(input/10);
  lcd.write(B11011111); //stupen
  lcd.print(F(" "));
}








void vypis_nastTemp(void){

  lcd.setCursor(0, 0);
  lcd.print(F("nast."));
  lcd.setCursor(0, 1);
  if(TempNast<100){
    lcd.print(" ");
  }
  lcd.print(TempNast/10);
  lcd.print(F("."));
  lcd.print(TempNast%10);  
  lcd.write(B11011111); //stupen

}


void vypis_lcd_nodata(void){
  lcd.print(F(" --  "));
}




void tx433MHz(void)
{
  //Serial.println(F("tx433MHz();"));
  tx433MHz_packet1();
  tx433MHz_packet2();
  tx433MHz_packet3();
}


void tx433MHz_packet1(void)
{

  char msg[12] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '                                                     };

  msg[0]='T';
  msg[1]='H';
  msg[2]='B';
  msg[3]='1';

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

  // replace chr 11 with count (#)
  //msg[11] = txcount433MHz;
  //Serial.println(msg,DEC);
  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
  //txcount433MHz++;
}



void tx433MHz_packet2(void)
{

  char msg[12] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '                                                     };

  msg[0]='T';
  msg[1]='H';
  msg[2]='B';
  msg[3]='2';

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

  // replace chr 11 with count (#)
  //msg[11] = txcount433MHz;
  //Serial.println(msg,DEC);
  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
  //txcount433MHz++;
}




void tx433MHz_packet3(void)
{

  char msg[12] = {
    ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '                                                     };

  msg[0]='T';
  msg[1]='H';
  msg[2]='B';
  msg[3]='3';

  msg[4]= TempBase / 10;
  msg[5]= TempBase % 10;
  msg[6]= TempNast / 10;
  msg[7]= TempNast % 10;
  msg[8]= KotolStatus;
  msg[9]= CerpadloStatus;
  msg[10]= Rele3Status;

  // replace chr 11 with count (#)
  //msg[11] = txcount433MHz;
  //Serial.println(msg,DEC);
  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
  //txcount433MHz++;
}






void set_kotol(void)
{
  int tmp_base;

  if(time > rx433MHzAgainIzba1){
    tmp_base = TempBase;
    //Serial.println("using TempBase");
  } 
  else { 
    tmp_base = TempIzba1; 
    //Serial.println("using TempIzba1");
  }


  //  Serial.print(TempNast);
  //  Serial.print(" > ");
  //  Serial.println(tmp_base);

  if(TempNast > tmp_base){
    Serial.println("+"); 
  }
  else {
    Serial.println("_"); 
  }    


  if(TempNast > tmp_base){
    if(KotolStatus == 0){
      if((time - KotolCasVypnutia > Kotol_MinCasMedziDvomaZapnutiamiKotla) || (KotolCasVypnutia == 0)){
        set_kotol(1); 
        KotolCasZapnutia = time;
        Serial.println("kotol zapnutie");
      }
      else {   
        Serial.println("kotol kratky cas medzi dvoma zapnutiami");
      }
    }
  } 
  else {
    if(KotolStatus == 1){
      if(time - KotolCasZapnutia > Kotol_MinCasBehuKotla){
        set_kotol(0); 
        KotolCasVypnutia = time;
        Serial.println("kotol vypnutie");
      } 
      else {
        Serial.println("kotol kratky cas zapnutia");
      }
    }
  }

  if(KotolStatus == 1){
    if(CerpadloStatus == 0){ 
      set_cerpadlo(1);
      Serial.println("cerpadlo zapnutie");
    }
  }
  else {
    if(CerpadloStatus == 1){
      if(time - KotolCasVypnutia > Kotol_CasDobehCerpadlo){
        set_cerpadlo(0);
        Serial.println("cerpadlo vypnutie");
      }
      else {   
        Serial.println("cerpadlo dobeh");
      } 
    }
  }
}



















