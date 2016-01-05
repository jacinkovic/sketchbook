int TempVonku1;
int TempVonku2;
int TempVonku3;

void setup() {
  Serial.begin(115200);  // Debugging only
  Serial.println(F("setup();"));
}

void loop() {

  char msg[12] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
  };



  for (int i = -1000; i < 1000; i++) {
    int in = i;
    Serial.print(F("in="));
    Serial.print(in);


    msg[4] = in / 10;
    msg[5] = in % 10;

    TempVonku1 = convNumSigned(msg[4], msg[5]);


    Serial.print(F(" out1="));
    Serial.print(TempVonku1);
    //Serial.print(F(" out2="));
    //Serial.print(TempVonku2);
    //Serial.print(F(" out3="));
    //Serial.print(TempVonku3);

    if (in != TempVonku1) {
      Serial.print(F("  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  "));
    }
    if (in != TempVonku2) {
      //  Serial.print(F("  222  "));
    }
    if (in != TempVonku3) {
      //  Serial.print(F("  333  "));
    }

    Serial.println();

  }

  while (1);
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


