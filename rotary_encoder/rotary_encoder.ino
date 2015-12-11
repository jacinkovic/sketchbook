int rEncPinA = 9;  // Connected to CLK on KY-040
int rEncPinB = 10;  // Connected to DT on KY-040
int rEncPinButton = 11; // Connected to SW on KY-040
int rEncPosCount = 0; 
int rEncValALast;  
int rEncValButtonLast;

void setup() { 
  rEncInit();
  Serial.begin (115200);
} 

void loop() { 
  rEncGet();


} 




void rEncInit(void)
{
  pinMode(rEncPinA,INPUT);
  pinMode(rEncPinB,INPUT);
  pinMode(rEncPinButton,INPUT);

  rEncValALast = digitalRead(rEncPinA);   
}


int rEncGet(void){
  int rEncValA;
  int rEncValButton;
  boolean rEncbCW;
  int ret=0;

  rEncValA = digitalRead(rEncPinA);
  if (rEncValA != rEncValALast){ // Means the knob is rotating
    // if the knob is rotating, we need to determine direction
    // We do that by reading pin B.
    if (digitalRead(rEncPinB) != rEncValA) {  // Means pin A Changed first - We're Rotating Clockwise
      rEncPosCount ++;
      rEncbCW = true;
      ret = 1;
    } 
    else {// Otherwise B changed first and we're moving CCW
      rEncbCW = false;
      rEncPosCount--;
      ret = -1;
    }
    Serial.print(F("Position: "));
    Serial.println(rEncPosCount);

  } 
  rEncValALast = rEncValA;

  rEncValButton = 1 - digitalRead(rEncPinButton);
  if (rEncValButton != rEncValButtonLast){ 
    Serial.print (F("Button: "));
    Serial.println(rEncValButton);

  } 
  rEncValButtonLast = rEncValButton;
  
   return(ret);
}


