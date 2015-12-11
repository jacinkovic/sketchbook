#include <SoftwareSerial.h>

SoftwareSerial esp8266(2,3); // make RX Arduino line is pin 2, make TX Arduino line is pin 3.
// This means that you need to connect the TX line from the esp to the Arduino's pin 2
// and the RX line from the esp to the Arduino's pin 3


char serialbuffer[1000];//serial buffer for request url

void setup()
{
  Serial.begin(9600); //serial debug
  esp8266.begin(9600);//connection to ESP8266
  
  
  //set mode needed for new boards
  esp8266.println("AT+RST");
  esp8266.println("AT+CWMODE=1");
  delay(500);//delay after mode change
  esp8266.println("AT+RST");
  
  //connect to wifi network
  esp8266.println("AT+CWJAP=\"lab\",\"93051866\"");
}

void loop()
{
  //output everything from ESP8266 to the Arduino Micro Serial output
  while (esp8266.available() > 0) {
    Serial.write(esp8266.read());
  }
  
  if (Serial.available() > 0) {
     //read from serial until terminating character
     int len = Serial.readBytesUntil('\n', serialbuffer, sizeof(serialbuffer));
  
     //trim buffer to length of the actual message
     String message = String(serialbuffer).substring(0,len-1);
     Serial.println("message: " + message);
 
     //check to see if the incoming serial message is a url or an AT command
     if(message.substring(0,2)=="AT"){
       //make command request
       Serial.println("COMMAND REQUEST");
       esp8266.println(message); 
     }else{
      //make webrequest
       Serial.println("WEB REQUEST");
       WebRequest(message);
     }
  }
}

//web request needs to be sent without the http for now, https still needs some working
void WebRequest(String request){
 //find the dividing marker between domain and path
     int slash = request.indexOf('/');
     
     //grab the domain
     String domain;
     if(slash>0){  
       domain = request.substring(0,slash);
     }else{
       domain = request;
     }

     //get the path
     String path;
     if(slash>0){  
       path = request.substring(slash);   
     }else{
       path = "/";          
     }
     
     //output domain and path to verify
     Serial.println("domain: |" + domain + "|");
     Serial.println("path: |" + path + "|");     
     
     //create start command
     String startcommand = "AT+CIPSTART=\"TCP\",\"" + domain + "\", 80"; //443 is HTTPS, still to do
     
     esp8266.println(startcommand);
     Serial.println(startcommand);
     
     
     //test for a start error
     if(esp8266.find("Error")){
       Serial.println("error on start");
       return;
     }
     
     //create the request command
     String sendcommand = "GET http://"+ domain + path + " HTTP/1.0\r\n\r\n\r\n";//works for most cases
     
     Serial.print(sendcommand);
     
     //send 
     esp8266.print("AT+CIPSEND=");
     esp8266.println(sendcommand.length());
     
     //debug the command
     Serial.print("AT+CIPSEND=");
     Serial.println(sendcommand.length());
     
     //delay(5000);
     if(esp8266.find(">"))
     {
       Serial.println(">");
     }else
     {
       esp8266.println("AT+CIPCLOSE");
       Serial.println("connect timeout");
       delay(1000);
       return;
     }
     
     //Serial.print(getcommand);
     esp8266.print(sendcommand); 
}

