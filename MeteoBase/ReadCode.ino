int RCode()

{

  //String code = "001000000110110010101101000000000010"; //rain sensor
  //String code = "xxx110101010110111011100001101000001";  //wind direction + gust
  //String code = "xxx110101010110111011100001101000001";  //

  //code="001000000110110000000011000000000000"; //rain 48.00mm
  //      iiiiiiiivXXb 
  //code="xxx110101010110111000000000000000001"; //N -   0, 0gust
  //code="xxx110101010110111010110100000000000"; //E -  90, 0gust
  //code="xxx110101010110111001011010000000000"; //S - 180, 0gust  
  //code="xxx110101010110111011100001000000000"; //W - 270, 0gust

  //code="110101010010001110110000111001100000"; //22.0C 67%
  //code="110101010010101001110000101001100001"; //22.9C 65%

  //code="110101010110100000000000000000001100" //wavg=0
  //code="110101010110100000000000011000001011" //wavg=1.2 

  //  code = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  code = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  int i, ii;
  long startMicros = micros(), endMicros;

  if (digitalRead(ReceiverPin)) return 1;

  while(!digitalRead(ReceiverPin))
  {
    if ((micros()-startMicros)>smax)
      return 2;
  }

  if ((micros()-startMicros)<smin)
    return 3;

  startMicros = micros();
  while(digitalRead(ReceiverPin))
  {
    if ((micros()-startMicros)>semax)
      return 4;
  }

  if ((micros()-startMicros)<semin)
    return 5;

  for(i = 0; i < nobits; i++)
  {
    startMicros = micros();
    while(!digitalRead(ReceiverPin))
    { 
      if ((micros()-startMicros)>smax) //protection against deadlock
        return 6;
    }

    endMicros = micros();
    if(((endMicros-startMicros)>lmin)&&((endMicros-startMicros)<lmax)){
      code.setCharAt(i,'0');
    }
    else
      if(((endMicros-startMicros)>hmin)&&((endMicros-startMicros)<hmax)){
        code.setCharAt(i,'1');
        //Serial.print(F("&"));
      }

    startMicros = micros();
    while(digitalRead(ReceiverPin))
    {
      if ((micros()-startMicros)>semax)
        return 7;
    }

    if ((micros()-startMicros)<semin)
      return 8;
  }

  //SerialCode();

  //code = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
  //code = "101010101010100101001000010001000100xxxx";

  //remove x chars from the beginning - if exist
  for(int a=0; a<3; a++){
    //Serial.println(code.charAt(1));
    if(code.charAt(0)=='x'){
      for(int b=0; b<nobits-1; b++){ 
        code.setCharAt(b, code.charAt(b+1));
      }
    }  
  }

  //SerialCode();

  //check for x inside the code
  for(int b=0; b<36; b++){ 
    if(code.charAt(b) == 'x'){
      //Serial.println(F(" ERR >>> x in code <<<"));
      return 9;
    }
  }

  get_checksum();

  return 0;
}



//*****************************************************
int bcd_conv_dec(char n0, char n1, char n2, char n3)
{
  int ret = 
    (n0=='1'?1:0) * 1 +
    (n1=='1'?1:0) * 2 +
    (n2=='1'?1:0) * 4 +
    (n3=='1'?1:0) * 8;

  return ret;
}





//*****************************************************
int get_checksum(void)
{

  //chksum
  int n[8], i;
  int na_combsensor = 15; //0xF
  int na_raingauge = 7; //0x7

    //Serial.println(F(" "));
  for(i=0; i<8; i++){
    n[i] = bcd_conv_dec(code.charAt(i*4+0), code.charAt(i*4+1), code.charAt(i*4+2), code.charAt(i*4+3));  
    na_combsensor = na_combsensor - n[i];
    na_raingauge = na_raingauge + n[i];
    //Serial.print(n[i]); 
    //Serial.print(F("    ")); 
  }

  //Serial.println(F(" "));
  //Serial.print(F(" na_combsensor= ")); 
  //Serial.print(na_combsensor); 
  //Serial.print(F("; ")); 
  bcd_conv_bin(na_combsensor, 1); //combsensor

  //Serial.print(F(" na_raingauge= ")); 
  //Serial.print(na_raingauge); 
  //Serial.print(F("; ")); 
  bcd_conv_bin(na_raingauge, 2);  //raingauge

  return 0;
}



//*****************************************************
void bcd_conv_bin(int in, int ver)
{
  String chksum = "0000";

  //negative values
  while(in<=-16){ 
    in = in + 16; 
  }    
  //two's complement
  if(in<0){
    in = 16 + in; 
  } 


  //positive values
  if(in>=0){

    while(in>=16){ 
      in = in - 16; 
    }

    if (in>=8){ 
      chksum.setCharAt(3,'1'); 
      in = in - 8;
    } 
    if (in>=4){ 
      chksum.setCharAt(2,'1');
      in = in - 4;
    } 
    if (in>=2){ 
      chksum.setCharAt(1,'1'); 
      in = in - 2;
    } 
    if (in>=1){ 
      chksum.setCharAt(0,'1');
      in = in - 1;
    } 
  }


  //two type of sensors
  if(ver==1){ 
    //Serial.print(F("chksum_combsens= "));
    chksum_combsensor = chksum; 
  }
  if(ver==2){ 
    //Serial.print(F("chksum_raingauge= "));
    chksum_raingauge = chksum; 
  }
  //Serial.print(chksum);
  //Serial.println(F("; "));


}


int check_checksum(String in)
{
  //checksum
  if
    ((code.charAt(32)==in.charAt(0))&&
    (code.charAt(33)==in.charAt(1))&&
    (code.charAt(34)==in.charAt(2))&&
    (code.charAt(35)==in.charAt(3)))
  { 
    //Serial.println(F("  Checksum OK "));
    return 1;
  }
  else
  { 
    Serial.println(F("  Checksum ERR "));
    return 4;
  }
}



//************************************************************
void getGeneral(void)
{
  //ID
  ID = 
    (code.charAt(7)=='1'?1:0)*128 +
    (code.charAt(6)=='1'?1:0)*64 +
    (code.charAt(5)=='1'?1:0)*32 +
    (code.charAt(4)=='1'?1:0)*16 +
    (code.charAt(3)=='1'?1:0)*8 +
    (code.charAt(2)=='1'?1:0)*4 +
    (code.charAt(1)=='1'?1:0)*2 +
    (code.charAt(0)=='1'?1:0)*1;
  //Serial.print(F(" ID="));
  //Serial.println(ID);

  //Serial.print(F(" batt="));
  //if(batt==0) Serial.println(F("OK")); 
  //else Serial.println(F("LOW"));
}



//************************************************************
void getTempHumid(void)
{
  if 
    ((code.charAt(9)!='1')||
    (code.charAt(10)!='1'))  
  {
    Serial.println(F("TempHumid"));
    if(check_checksum(chksum_combsensor)==1)
    { 
      int tempOut_tmp= 
        (code.charAt(23)=='1'?1:0)*2048 +
        (code.charAt(22)=='1'?1:0)*1024 +
        (code.charAt(21)=='1'?1:0)*512 +
        (code.charAt(20)=='1'?1:0)*256 +
        (code.charAt(19)=='1'?1:0)*128 +
        (code.charAt(18)=='1'?1:0)*64 +
        (code.charAt(17)=='1'?1:0)*32 +
        (code.charAt(16)=='1'?1:0)*16 +
        (code.charAt(15)=='1'?1:0)*8 +
        (code.charAt(14)=='1'?1:0)*4 +
        (code.charAt(13)=='1'?1:0)*2 +
        (code.charAt(12)=='1'?1:0)*1;

      // Calculate negative temperature
      if((tempOut_tmp & 0x800) == 0x800)
      {  
        tempOut_tmp = tempOut_tmp | 0xF000;
      }
      tempOut = (float)(tempOut_tmp) / 10;

      humidOut = 
        (code.charAt(31)=='1'?1:0)*8 +
        (code.charAt(30)=='1'?1:0)*4 +
        (code.charAt(29)=='1'?1:0)*2 +
        (code.charAt(28)=='1'?1:0)*1;
      humidOut = humidOut * 10 +
        (code.charAt(27)=='1'?1:0)*8 +
        (code.charAt(26)=='1'?1:0)*4 +
        (code.charAt(25)=='1'?1:0)*2 +
        (code.charAt(24)=='1'?1:0)*1;

      //battery state
      batt_combsensor=(code.charAt(8)=='1'?1:0)*1;
      //button
      tempOutButton=(code.charAt(11)=='1'?1:0);

      rec_time_TempHumid = millis();

      //      Serial.print(F(" tempOutButton="));
      //      Serial.println(tempOutButton);
      //      Serial.print(F(" tempOut="));
      //      Serial.println(tempOut);
      //      Serial.print(F(" humidOut="));
      //      Serial.println(humidOut);
    }
  }
}



//************************************************************
void getWindAverage(void)
{
  if 
    ((code.charAt(9)=='1')&&
    (code.charAt(10)=='1')&&
    (code.charAt(12)=='1')&&
    (code.charAt(13)=='0')&&
    (code.charAt(14)=='0')&&
    (code.charAt(15)=='0')&&
    (code.charAt(16)=='0')&&
    (code.charAt(17)=='0')&&
    (code.charAt(18)=='0')&&
    (code.charAt(19)=='0')&&
    (code.charAt(20)=='0')&&
    (code.charAt(21)=='0')&&
    (code.charAt(22)=='0')&&
    (code.charAt(23)=='0'))  
  {
    Serial.println(F("WindAverage"));
    if(check_checksum(chksum_combsensor)==1)
    { 
      windAvg = 
        (code.charAt(31)=='1'?1:0)*128 +
        (code.charAt(30)=='1'?1:0)*64 +
        (code.charAt(29)=='1'?1:0)*32 +
        (code.charAt(28)=='1'?1:0)*16 +
        (code.charAt(27)=='1'?1:0)*8 +
        (code.charAt(26)=='1'?1:0)*4 +
        (code.charAt(25)=='1'?1:0)*2 +
        (code.charAt(24)=='1'?1:0)*1;
      windAvg = windAvg/5;

      //battery state
      batt_combsensor=(code.charAt(8)=='1'?1:0)*1;
      //button
      windButton=(code.charAt(11)=='1'?1:0);

      rec_time_WindAverage = millis();

      //      Serial.print(F(" windButton="));
      //      Serial.println(windButton);
      //      Serial.print(F(" windAvg="));
      //      Serial.println(windAvg);
    }
  }
}


//************************************************************
void getWindDirectionGust(void)
{
  if 
    ((code.charAt(9)=='1')&&
    (code.charAt(10)=='1')&&
    (code.charAt(12)=='1')&&
    (code.charAt(13)=='1')&&
    (code.charAt(14)=='1'))
  {     
    Serial.println(F("WindDirectionGust"));
    if(check_checksum(chksum_combsensor)==1)
    {    
      windDir = 
        (code.charAt(23)=='1'?1:0)*256 +
        (code.charAt(22)=='1'?1:0)*128 +
        (code.charAt(21)=='1'?1:0)*64 +
        (code.charAt(20)=='1'?1:0)*32 +
        (code.charAt(19)=='1'?1:0)*16 +
        (code.charAt(18)=='1'?1:0)*8 +
        (code.charAt(17)=='1'?1:0)*4 +
        (code.charAt(16)=='1'?1:0)*2 +
        (code.charAt(15)=='1'?1:0)*1;
      windGust = 
        (code.charAt(31)=='1'?1:0)*128 +
        (code.charAt(30)=='1'?1:0)*64 +
        (code.charAt(29)=='1'?1:0)*32 +
        (code.charAt(28)=='1'?1:0)*16 +
        (code.charAt(27)=='1'?1:0)*8 +
        (code.charAt(26)=='1'?1:0)*4 +
        (code.charAt(25)=='1'?1:0)*2 +
        (code.charAt(24)=='1'?1:0)*1;

      windGust = windGust/5;

      //battery state
      batt_combsensor=(code.charAt(8)=='1'?1:0)*1;
      //button
      windButton=(code.charAt(11)=='1'?1:0);

      rec_time_WindDirectionGust = millis();

      //      Serial.print(F(" batt_combsensor="));
      //      Serial.println(batt_combsensor);
      //      Serial.print(F(" windButton="));
      //      Serial.println(windButton);
      //      Serial.print(F(" windDir="));
      //      Serial.println(windDir);
      //      Serial.print(F(" windGust="));
      //      Serial.println(windGust);
    }
  }
}


//************************************************************
void getRain(void)
{
  if 
    ((code.charAt(9)=='1')&&
    (code.charAt(10)=='1')&&
    (code.charAt(11)=='0')&&
    (code.charAt(12)=='1')&&
    (code.charAt(13)=='1')&&
    (code.charAt(14)=='0')&&
    (code.charAt(15)=='0'))
  {
    Serial.println(F("Rain"));
    if(check_checksum(chksum_raingauge)==1)
    {
      rain = (code.charAt(31)=='1'?1:0)*32768 +
        (code.charAt(30)=='1'?1:0)*16384 +
        (code.charAt(29)=='1'?1:0)*8192 +
        (code.charAt(28)=='1'?1:0)*4096 +
        (code.charAt(27)=='1'?1:0)*2048 +
        (code.charAt(26)=='1'?1:0)*1024 +
        (code.charAt(25)=='1'?1:0)*512 +
        (code.charAt(24)=='1'?1:0)*256 +
        (code.charAt(23)=='1'?1:0)*128 +
        (code.charAt(22)=='1'?1:0)*64 +
        (code.charAt(21)=='1'?1:0)*32 +
        (code.charAt(20)=='1'?1:0)*16 +
        (code.charAt(19)=='1'?1:0)*8 +
        (code.charAt(18)=='1'?1:0)*4 +
        (code.charAt(17)=='1'?1:0)*2 +
        (code.charAt(16)=='1'?1:0)*1;
      rain = rain/4;

      //battery state
      batt_raingauge=(code.charAt(8)=='1'?1:0)*1;

      rec_time_Rain = millis();

      //substract initial rain value
      if(rain_initialshift == -1){
        rain_initialshift = rain;
      };
      rain = rain - rain_initialshift;


      //      Serial.print(F(" batt_raingauge="));
      //      Serial.println(batt_raingauge);
      //      Serial.print(F(" rain="));
      //      Serial.println(rain);
    }
  }  
}  



void SerialCode(void)
{
  //len vypis
  Serial.println();
  Serial.print(F("<"));
  for(int i = 0; i<9+0; i++){ //+1 for debug
    for(int ii = 0; ii<4; ii++){
      Serial.print(F(""));
      Serial.print(code.charAt(ii+i*4));
    }
    Serial.print(F(" "));
  }    
  Serial.print(F(">"));
  Serial.println();  
}




void getVBat()
{
  float Vcc = readVcc();
  delay(2);
  float volt = analogRead(VBatPin);
  volt = (volt / 1023.0) * Vcc; // only correct if Vcc = 5.0 volts

  //R VCC 4.7 a 1k GND
  //VBat = volt * 5.73; //5.7
  VBat = volt / 175;
}







float getTempOut2(){
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !ds_TempOut2.search(addr)) {
    //no more sensors on chain, reset search
    ds_TempOut2.reset_search();
    //Serial.println(F("no more sensors on chain, reset search!"));
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    //Serial.println(F("CRC is not valid!"));
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    //Serial.print(F("Device is not recognized"));
    return -1000;
  }

  ds_TempOut2.reset();
  ds_TempOut2.select(addr);
  ds_TempOut2.write(0x44); // start conversion

  byte present = ds_TempOut2.reset();
  ds_TempOut2.select(addr);
  ds_TempOut2.write(0xBE); // Read Scratchpad


  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ds_TempOut2.read();
  }

  ds_TempOut2.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;


  if (data[8] != OneWire::crc8(data,8)) {
    Serial.print(F("ERROR: CRC didn't match\n"));
    return -1000;
  }

  TempOut2 = TemperatureSum;

  return TemperatureSum;

}









void SerialAll ()
{
  //moj vyber
  Serial.print(F("  tempOut="));
  Serial.print(tempOut);
  Serial.print(F(", humidOut="));
  Serial.print(humidOut);
  Serial.print(F(", rain="));
  Serial.println(rain);

  Serial.print(F("  windDir="));
  Serial.print(windDir);
  Serial.print(F(", windGust=")); 
  Serial.print(windGust);
  Serial.print(F(", windAvg="));
  Serial.println(windAvg);


  Serial.print(F("  TempOut2="));
  Serial.print(TempOut2);

  Serial.print(F("  batt_combsensor="));
  Serial.print(batt_combsensor);
  Serial.print(F(", batt_raingauge="));
  Serial.println(batt_raingauge);

  Serial.print(F("  recTimeoutStat="));
  Serial.print(recTimeoutStat);  
  Serial.print(F(", testcounter="));
  Serial.print(testcounter);
  Serial.print(F(", VBat="));
  Serial.println(VBat);  
}





long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1099560L / result; // Back-calculate AVcc in mV
  return result;
}














