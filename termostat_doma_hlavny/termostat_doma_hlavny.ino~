#include <Wire.h> //LCD
#include <LiquidCrystal_I2C.h>

#include <OneWire.h>  //DS18B20

#include <VirtualWire.h> //433MHz

#include <avr/wdt.h>

#include <EEPROM.h>

#define DEBUG

#define LED_ON  255
#define LED_OFF 0

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

const unsigned long Kotol_MinCasMedziDvomaZapnutiamiKotla =  60 * 60000L;
const unsigned long Kotol_MinCasBehuKotla =  60 * 60000L;
const unsigned long Kotol_CasDobehCerpadlo =  15 * 60000L;
unsigned long KotolCasZapnutia, KotolCasVypnutia = 0L;
unsigned char KotolStatus, CerpadloStatus;

const unsigned char LED_Pin = 13;
const unsigned char Light_Pin = A0;

const unsigned char Rele1_Pin = 5;
const unsigned char Rele2_Pin = 4;

const unsigned char DS18S20Base_Pin = 6; //DS18S20 Signal pin on digital pin
OneWire dsBase(DS18S20Base_Pin); // on digital pin 2

const unsigned char rx433MHz_pin = 2;
const unsigned char tx433MHz_pin = 3;

uint8_t buf[VW_MAX_MESSAGE_LEN];
uint8_t buflen = VW_MAX_MESSAGE_LEN;
const unsigned char buflen_433MHz = 12;

const unsigned long rx433MHzAgain_StartValue = 9999999L; //to get timeout on start

const unsigned long tx433MHzUpdateTime_Period = 30000L;
unsigned long tx433MHzUpdateTime = 0;

const unsigned long TempBaseUpdateTime_Period = 60000L;
unsigned long TempBaseUpdateTime = rx433MHzAgain_StartValue;

const unsigned long rx433MHzAgain_Timeout = 5 * 60000L;
unsigned long rx433MHzAgainIzba1 = rx433MHzAgain_StartValue;
unsigned long rx433MHzAgainIzba2 = rx433MHzAgain_StartValue;
unsigned long rx433MHzAgainVonku = rx433MHzAgain_StartValue;
unsigned long rx433MHzAgainSauna = rx433MHzAgain_StartValue;
unsigned long rx433MHzAgainRury = rx433MHzAgain_StartValue;


const unsigned long lcdUpdateTime_Period  = 1000L;
unsigned long lcdUpdateTime = 0;

const int TempNast_Min = 100;
const int TempNast_Max = 250;

int TempIzba1;
int TimeoutIzba1;
int TempIzba2;
int TimeoutIzba2;
int TempBase;
int TempNast;
int TempVonku, TimeoutVonku;
int SaunaTemp, TimeoutSauna;
int RuryKotolVystup, RuryKotolSpiatocka, RuryBojlerVystup, TimeoutRury;


const unsigned char rEncPinA = 7;  // Connected to CLK on KY-040
const unsigned char rEncPinB = 8;  // Connected to DT on KY-040
const unsigned char rEncPinButton = 9; // Connected to SW on KY-040
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

#ifdef DEBUG
  Serial.begin(115200);	// Debugging only
  Serial.println(F("setup();"));
#endif

  lcd.begin(20, 4);                     // initialize the lcd
  lcd.clear();
  lcd.backlight();

  lcd.createChar(1, znak_ohrev_on);

  lcd.setCursor(0, 0);
  lcd.print(F("starting ..."));

  loadEEPROM();
  rEncInit();

  vw_set_rx_pin(rx433MHz_pin);
  vw_set_tx_pin(tx433MHz_pin);
  vw_setup(2000);	 // Bits per sec
  vw_rx_start();   // Start the receiver PLL running

  randomSeed(analogRead(0));

  for (int i = 0; i < 10; i++) { //initialise ds18b20
    wdt_reset();
    int tmp = getTempBase();
    if (tmp != -1000) TempBase = tmp;
    delay(500);
  }

  if (rEncGetButton() == 1) {
    //nothing
  }

  initKotol();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("nast."));
  lcd.setCursor(0, 2);
  lcd.print(F("vonku"));
  lcd.setCursor(7, 2);
  lcd.print(F("sauna"));

#ifdef DEBUG
  Serial.println(F("loop();"));
#endif
}

void loop()
{
  wdt_reset();

  checkNastTemp();

  if ((long)(millis() - TempBaseUpdateTime > TempBaseUpdateTime_Period)) {
    TempBaseUpdateTime = millis();
    int tmp = getTempBase();
    if (tmp != -1000) TempBase = tmp;
  }

  if ((long)(millis() - lcdUpdateTime > lcdUpdateTime_Period)) {
    lcdUpdateTime = millis();

    setKotol();

    vypisNastTemp();
    vypisOstatne();
    vypisKotol();

    saveEEPROM();
    //vypisRam();

  }

  check433MHz();

  if ((long)(millis() - tx433MHzUpdateTime > tx433MHzUpdateTime_Period)) {
    tx433MHzUpdateTime = millis() - random(1000);
    tx433MHz();
  }

}




void vypisKotol(void)
{

  lcd.setCursor(19, 0);
  if (KotolStatus == 1) {
    lcd.print(F("k"));
  } else {
    lcd.print(F("_"));
  }


  lcd.setCursor(18, 0);
  if (CerpadloStatus == 1) {
    lcd.print(F("c"));
  } else {
    lcd.print(F("_"));
  }

  //lcd.setCursor(16, 1);
  //lcd.print(getLight());

}



void vypisOstatne(void)
{
  lcd.setCursor(0, 3);
  if ((long)(millis() - rx433MHzAgainVonku < rx433MHzAgain_Timeout)) {
    if (TimeoutVonku == 1) {
      vypisLcdNodata2();
    } else {
      vypisLcdTempInt(TempVonku);
    }
  }
  else {
    vypisLcdNodata();
  }


  lcd.setCursor(7, 3);
  if ((long)(millis() - rx433MHzAgainSauna < rx433MHzAgain_Timeout)) {
    if (TimeoutSauna == 1) {
      vypisLcdNodata2();
    } else {
      if (SaunaTemp == -444) {
        lcd.print(F(" vyp "));
      } else {
        vypisLcdTempInt(SaunaTemp);
      }
    }
  }
  else {
    vypisLcdNodata();
  }



  long ii = millis() / (lcdUpdateTime_Period  * 2);
  ii = ii % 3;

  switch (ii) {
    case 0:
      lcd.setCursor(7, 0);
      lcd.print(F("chodba"));
      lcd.setCursor(7, 1);
      vypisLcdDecimal(TempBase);

      lcd.setCursor(14, 2);
      lcd.print(F("kuren."));
      lcd.setCursor(14, 3);
      if ((long)(millis() - rx433MHzAgainRury < rx433MHzAgain_Timeout)) {
        if (TimeoutRury == 1) {
          vypisLcdNodata2();
        } else {
          vypisLcdTempInt(RuryKotolVystup);
        }
      }
      else {
        vypisLcdNodata();
      }

      break;

    case 1:
      lcd.setCursor(7, 0);
      lcd.print(F("izba1 "));
      if ((long)(millis() - rx433MHzAgainIzba1 < rx433MHzAgain_Timeout)) {
        lcd.setCursor(7, 1);
        vypisLcdDecimal(TempIzba1);
      }
      else {
        lcd.setCursor(7, 1);
        vypisLcdNodata();
      }

      lcd.setCursor(14, 2);
      lcd.print(F("spiat."));
      lcd.setCursor(14, 3);
      if ((long)(millis() - rx433MHzAgainRury < rx433MHzAgain_Timeout)) {
        if (TimeoutRury == 1) {
          vypisLcdNodata2();
        } else {
          vypisLcdTempInt(RuryKotolSpiatocka);
        }
      }
      else {
        vypisLcdNodata();
      }

      break;

    case 2:
      lcd.setCursor(7, 0);
      lcd.print(F("izba2 "));
      lcd.setCursor(7, 1);
      if ((long)(millis() - rx433MHzAgainIzba2 < rx433MHzAgain_Timeout)) {
        vypisLcdDecimal(TempIzba2);
      }
      else {
        lcd.setCursor(7, 1);
        vypisLcdNodata();
      }

      lcd.setCursor(14, 2);
      lcd.print(F("bojler"));
      lcd.setCursor(14, 3);
      if ((long)(millis() - rx433MHzAgainRury < rx433MHzAgain_Timeout)) {
        if (TimeoutRury == 1) {
          vypisLcdNodata2();
        } else {
          vypisLcdTempInt(RuryBojlerVystup);
        }
      }
      else {
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
  lcd.print(millis() / 1000);
  lcd.print(F("> "));
  //Serial.println(buflen);
}

void initKotol(void)
{
  pinMode(Rele1_Pin, OUTPUT);
  pinMode(Rele2_Pin, OUTPUT);

  setKotol(0);
  setCerpadlo(0);

  KotolCasVypnutia = 0;
}

void setKotol(unsigned char value)
{
  KotolStatus = value;
  if (value == 0) {
    analogWrite(Rele1_Pin, 255);
  }
  else {
    analogWrite(Rele1_Pin, 0);
  }
}


void setCerpadlo(unsigned char value)
{
  CerpadloStatus = value;
  if (value == 0) {
    analogWrite(Rele2_Pin, 255);
  }
  else {
    analogWrite(Rele2_Pin, 0);
  }
}





void check433MHz(void) {
  buflen = buflen_433MHz;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    //Message with a good checksum received, dump it.
    //Serial.println(F("VW_GET_MESSAGE"));
    int i;

    led(LED_ON);

#ifdef DEBUG
    Serial.println(F("Received433MHz: "));
    Serial.print(F(" buflen="));
    Serial.println(buflen);
    Serial.print(F(" ASC: "));
#endif

    for (i = 0; i < buflen; i++)
    {
      char c;
      c = buf[i];
#ifdef DEBUG
      Serial.print(c);
      Serial.print(F(" "));
#endif
    }
#ifdef DEBUG
    Serial.println();
#endif
    //         Serial.print(F(" DEC: "));
    //    for (i = 0; i < buflen; i++)
    //    {
    //      Serial.print(buf[i], DEC);
    //      Serial.print(F(" "));
    //    }
    //    Serial.println();


    if ((buf[0] == 'T') && (buf[1] == 'H') &&
        (buf[2] == 'S') && (buf[10] == 0)) {

      if (buf[3] == '1') {
        rx433MHzAgainIzba1 = millis();
        TempIzba1 = convNumSigned(buf[4], buf[5]);
        TimeoutIzba1 = buf[11];
      }

      if (buf[3] == '2') {
        rx433MHzAgainIzba2 = millis();
        TempIzba2 = convNumSigned(buf[4], buf[5]);
        TimeoutIzba2 = buf[11];
      }
    }


    if ((buf[0] == 'T') && (buf[1] == 'H') &&
        (buf[2] == 'E')) {

      if (buf[3] == '1') {
        rx433MHzAgainVonku = millis();
        TempVonku = convNumSigned(buf[4], buf[5]);
        TimeoutVonku = buf[10];
      }

      if (buf[3] == '2') {
        rx433MHzAgainSauna = millis();
        SaunaTemp = convNumSigned(buf[4], buf[5]);
        TimeoutSauna = buf[10];
      }

      if (buf[3] == '3') {
        rx433MHzAgainRury = millis();
        RuryKotolVystup = convNumSigned(buf[4], buf[5]);
        RuryKotolSpiatocka = convNumSigned(buf[6], buf[7]);
        RuryBojlerVystup = convNumSigned(buf[8], buf[9]);
        TimeoutRury = buf[10];
      }
    }

    led(LED_OFF);

  }

}


void rEncInit(void)
{
  pinMode(rEncPinA, INPUT);
  pinMode(rEncPinB, INPUT);
  pinMode(rEncPinButton, INPUT);

  rEncValALast = digitalRead(rEncPinA);
}


int rEncGetValue(void) {
  int rEncValA;
  int rEncValB;
  int ret = 0;


  rEncValA = digitalRead(rEncPinA);
  rEncValB = digitalRead(rEncPinB);

  if (rEncValA != rEncValALast) { // Means the knob is rotating
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

  return (ret);
}

int rEncGetButton(void) {
  wdt_reset();
  int rEncValButton = 1 - digitalRead(rEncPinButton);
  if (rEncValButton != rEncValButtonLast) {
#ifdef DEBUG
    Serial.print (F("Button= "));
    Serial.println(rEncValButton);
#endif
  }
  rEncValButtonLast = rEncValButton;
  return rEncValButton;
}



void checkNastTemp(void) {
  int tmp_rEncGetValue = rEncGetValue();

  if (tmp_rEncGetValue != 0) {
#ifdef DEBUG
    Serial.print (F("tmp_rEncGetValue= "));
    Serial.println(tmp_rEncGetValue);
#endif
    if (rEncGetButton() == 1) { //is the button pressed?

      if (tmp_rEncGetValue == 1) {
        TempNast--;
      }

      if (tmp_rEncGetValue == -1) {
        TempNast++;
      }

      TempNast = constrain(TempNast, TempNast_Min, TempNast_Max);
      vypisNastTemp();
    }
  }
}


void loadEEPROM(void)
{
  //format
  //TempNast 10

  TempNast = constrain(EEPROM.read(10), TempNast_Min, TempNast_Max);
#ifdef DEBUG
  Serial.print(F("TempNast= "));
  Serial.println(TempNast);
#endif
}



void saveEEPROM(void)
{
  EEPROMupdate(10, TempNast);
}


//user function
void EEPROMupdate(int address, unsigned char value)
{
  unsigned char old = EEPROM.read(address);

  if (old != value) {
    EEPROM.write(address, value);
  }
}



int getTempBase() {
  //returns the temperature from one DS18S20 in DEG Celsius

  unsigned char data[12];
  unsigned char addr[8];

  if ( !dsBase.search(addr)) {
    //no more sensors on chain, reset search
    dsBase.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
#ifdef DEBUG
    Serial.println(F("CRC is not valid!"));
#endif
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
#ifdef DEBUG
    Serial.print(F("Device is not recognized"));
#endif
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

  int tempRead = ((MSB << 8) | LSB); //using two's compliment
  tempRead = tempRead * 10;
  int TemperatureSum = tempRead / 16;

  if (data[8] != OneWire::crc8(data, 8)) {
#ifdef DEBUG
    Serial.print(F("ERROR: CRC didn't match\n"));
#endif
    return -1000;
  }

  return TemperatureSum;

}






char rEncGetChar( int direct, int input) {
  int old_input = input;

#ifdef DEBUG
  Serial.println(input);
#endif
  if (direct == 1) {
    input--;
    if (input == 97 - 1) input = 90;
    if (input == 65 - 1) input = 57;
    if (input == 48 - 1) input = 32;
    if (input == 32 - 1) input = 122;
  }
  else if (direct == -1) {
    input++;
    if (input == 32 + 1) input = 48;
    if (input == 57 + 1) input = 65;
    if (input == 90 + 1) input = 97;
    if (input == 122 + 1) input = 32;
  }

  if (old_input == input) { //no change as input is out of range, spacing
    input = 32;
  }

#ifdef DEBUG
  Serial.println(input);
#endif
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






void vypisLcdDecimal(int input) {
  if ((input < 100) && (input != 0)) {
    lcd.print(F(" "));
  }
  lcd.print(input / 10);
  lcd.print(F("."));
  lcd.print(input % 10);
  lcd.write(B11011111); //stupen
}





void vypisLcdTempInt(int input) {
  input = input / 10;
  //lcd.print(F(" "));
  if (abs(input) < 10) {
    lcd.print(F(" "));
  }
  if (input >= 0) {
    lcd.print(F(" "));
  }
  lcd.print(input);
  lcd.write(B11011111); //stupen
  lcd.print(F(" "));
}


void vypisNastTemp(void) {

  lcd.setCursor(0, 1);
  if (TempNast < 100) {
    lcd.print(F(" "));
  }
  lcd.print(TempNast / 10);
  lcd.print(F("."));
  lcd.print(TempNast % 10);
  lcd.write(B11011111); //stupen
}


void vypisLcdNodata(void) {
  lcd.print(F(" --  "));
}


void vypisLcdNodata2(void) {
  lcd.print(F(" __  "));
}

void vypisLcdNodata3(void) {
  lcd.print(F("     "));
}



void tx433MHz(void)
{
  led(LED_ON);
#ifdef DEBUG
  Serial.println(F("tx433MHz();"));
#endif
  tx433MHzPacket(1);
  tx433MHzPacket(2);
  tx433MHzPacket(3);
  led(LED_OFF);

}


void tx433MHzPacket(unsigned int packetNum)
{

  char msg[12] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
  };

  msg[0] = 'T';
  msg[1] = 'H';
  msg[2] = 'B';
  msg[3] = packetNum + 48;

  if (packetNum == 1) {
    msg[4] = TempIzba1 / 10;
    msg[5] = TempIzba1 % 10;

    if ((long)(millis() - rx433MHzAgainIzba1 > rx433MHzAgain_Timeout)) {
      msg[10] = 1;
    }
    else {
      msg[10] = 0;
    }
  }

  if (packetNum == 2) {
    msg[4] = TempIzba2 / 10;
    msg[5] = TempIzba2 % 10;

    if ((long)(millis() - rx433MHzAgainIzba2 > rx433MHzAgain_Timeout)) {
      msg[10] = 1;
    }
    else {
      msg[10] = 0;
    }
  }

  if (packetNum == 3) {
    msg[4] = TempBase / 10;
    msg[5] = TempBase % 10;
    msg[6] = TempNast / 10;
    msg[7] = TempNast % 10;
    msg[8] = KotolStatus;
    msg[9] = CerpadloStatus;
  }

  vw_send((uint8_t *)msg, buflen_433MHz);
  vw_wait_tx(); // Wait until the whole message is gone
}






void setKotol(void)
{
  int tmp_base, tmp_base1, tmp_base2;

  tmp_base1 = TempIzba1;
  if ((long)(millis() - rx433MHzAgainIzba1 > rx433MHzAgain_Timeout)) {
    tmp_base1 = TempBase;
#ifdef DEBUG
    Serial.println(F("nebezi Izba1"));
#endif
  }

  tmp_base2 = TempIzba2;
  if ((long)(millis() - rx433MHzAgainIzba2 > rx433MHzAgain_Timeout)) {
    tmp_base2 = TempBase;
#ifdef DEBUG
    Serial.println(F("nebezi Izba2"));
#endif
  }

  //vyberie nizsiu teplotu z oboch senzorov, resp. nahrada za hlavny senzor
  tmp_base = min(tmp_base1, tmp_base2); //vyssiu z oboch izieb
  tmp_base = min(TempBase, tmp_base);  //porovnat aj s bazou

#ifdef DEBUG
  if (TempNast > tmp_base) {
    Serial.println("+");
  }
  else {
    Serial.println("_");
  }
#endif

  if (TempNast > tmp_base) {
    if (KotolStatus == 0) {
      if ((long)(millis() - KotolCasVypnutia > Kotol_MinCasMedziDvomaZapnutiamiKotla) || (long)(KotolCasVypnutia == 0)) {
        setKotol(1);
        KotolCasZapnutia = millis();
#ifdef DEBUG
        Serial.println(F("Kotol zapnutie"));
#endif
      }
      else {
#ifdef DEBUG
        Serial.println(F("Kotol kratky cas medzi dvoma zapnutiami"));
#endif
      }
    }
  }
  else {
    if (KotolStatus == 1) {
      if ((long)(millis() - KotolCasZapnutia > Kotol_MinCasBehuKotla)) {
        setKotol(0);
        KotolCasVypnutia = millis();
#ifdef DEBUG
        Serial.println(F("Kotol vypnutie"));
#endif
      }
      else {
#ifdef DEBUG
        Serial.println(F("Kotol kratky cas zapnutia"));
#endif
      }
    }
  }

  if (KotolStatus == 1) {
    if (CerpadloStatus == 0) {
      setCerpadlo(1);
#ifdef DEBUG
      Serial.println(F("Cerpadlo zapnutie"));
#endif
    }
  }
  else {
    if (CerpadloStatus == 1) {
      if ((long)(millis() - KotolCasVypnutia > Kotol_CasDobehCerpadlo)) {
        setCerpadlo(0);
#ifdef DEBUG
        Serial.println(F("Cerpadlo vypnutie"));
#endif
      }
      else {
#ifdef DEBUG
        Serial.println(F("Cerpadlo dobeh"));
#endif
      }
    }
  }
}




int convNumSigned(int bufh, int bufl) {
  if (bufh > 127) {
    bufh = - ( 256 - bufh );
  }

  if (bufl > 127) {
    bufl = - ( 256 - bufl );
  }

  return (10 * (int)bufh + (int)bufl);
}





void led(byte value) {
  if (value > 0) {
    value = LED_ON;
  }
  pinMode(LED_Pin, OUTPUT);
  analogWrite(LED_Pin, value);
}



int getLight(void) {
  return analogRead(Light_Pin);;
}


int freeRam()
{
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}









































