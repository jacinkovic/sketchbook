/*
VirtualWire 1.15 modifyed by http:/567.dk to fit my setup
Com speed 115200
transmit_en_pin = 3;
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!! Prints out ASCII                 !!!!!!!!!!!! 
!!!!!!!!!!!! You screen will act on the codes !!!!!!!!!!!! 
!!!!!!!!!!!! Advice DEBUG with reciver1_5_HEX !!!!!!!!!!!! 
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
*/
// transmitter.pde
//
// Simple example of how to use VirtualWire to transmit messages
// Implements a simplex (one-way) transmitter with an TX-C1 module
//
// See VirtualWire.h for detailed API docs
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2008 Mike McCauley
// $Id: transmitter.pde,v 1.3 2009/03/30 00:07:24 mikem Exp $

#include <VirtualWire.h>

const int led_pin = 13;
const int transmit_pin = 2;
const int receive_pin = 2;
const int transmit_en_pin = 3;

void setup()
{
    delay(1000);
    Serial.begin(115200);	// Debugging only
    Serial.println("setup");
    // Initialise the IO and ISR
    vw_set_tx_pin(transmit_pin);
    vw_set_rx_pin(receive_pin);
    vw_set_ptt_pin(transmit_en_pin);
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(2000);       // Bits per sec
    pinMode(led_pin, OUTPUT);
}

byte count = 1;

void loop()
{
  char msg[12] = {'5','6','7','.','d','k',' ','t','e','s','t','#'};
// replace chr 11 with count (#)
  msg[11] = count;
  digitalWrite(led_pin, HIGH); // Flash a light to show transmitting
  Serial.println(msg);
  vw_send((uint8_t *)msg, 12);
  vw_wait_tx(); // Wait until the whole message is gone
  digitalWrite(led_pin, LOW);
  delay(1000);
  count = count + 1;
}
