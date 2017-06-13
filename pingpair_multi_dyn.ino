/*
 Copyright (C) 2016 J Carlos Andrioli. <jandrioli@outlook.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * This is a RF24 slave. The hardware is supposed to switch a relay on
 * and off according to the RF24 master. In this sketch, we make the mc
 * listens for specific commands, possibly responds with an enquiry, and 
 * obeys what the master is asking. 
 * 
 * More specifically this will be a "schedulable" power plug, the master
 * can tell the slave when to turn on or off the plug. The slave can also
 * be programmed to turn on within a timeframe, e.g. turn on in 2h15min.
 * 
 * For my use I programmed the following commands:
 * 0 - stop  : deactivate the relay(s) and reinitialize all variables.
 * 
 * 1 - start : turn on the relays after X time. This command requires an
 *             extra exchange of messages where the master tells how 
 *             much time must pass before switching the relay. 
 *             In practice this will look like this crude interaction 
 *             diagram:
 *             Master  | Slave
 *             start --> 
 *                     <-- schedule
 *             12h   -->  
 *                     <-- starting in 12h
 
 * 2 - status: master wants to know the current state of relays and wait
 *             time.
 * 
 * This is an example of how to use payloads of a varying (dynamic) size. 
 */

#include <SPI.h>
#include "RF24.h"

//
// Hardware configuration
//
// Set up nRF24L01 radio on SPI bus plus pins 8 & 9
RF24 radio(9,10);

// Use multicast?
// sets the multicast behavior this unit in hardware.  Connect to GND to use unicast
// Leave open (default) to use multicast.
const int multicast_pin = 6 ;

// sets the role of this unit in hardware.  Connect to GND to be the 'pong' receiver
// Leave open to be the 'ping' transmitter
const int role_pin = 7;
bool multicast = true ;

//
// Topology
//
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xEEFAFDFDEELL, 0xEEFDFAF50DFLL };


//
// Controller stuff (actual business of this node)
//
long schedule1 = 0,schedule2 = 0;    // hold the amount of time in seconds until activation
bool relay1 = false, relay2 = false;  // activation state of relay number 2
#define HW_RELAY1 7
#define HW_RELAY2 8


void setup(void)
{
  pinMode(HW_RELAY1, OUTPUT);
  pinMode(HW_RELAY2, OUTPUT);
  digitalWrite(HW_RELAY1, LOW);
  digitalWrite(HW_RELAY2, LOW);
  //
  // Multicast
  //
  pinMode(multicast_pin, INPUT_PULLUP);
  delay( 20 ) ;

  // read multicast role, LOW for unicast
  if( digitalRead( multicast_pin ) )
    multicast = true ;
  else
    multicast = false ;


  //
  // Print preamble
  //
  Serial.begin(115200);
  Serial.println(F("RF24 Slave - power socket controller"));
  Serial.print(F("MULTICAST: "));
  Serial.println(multicast ? F("true (unreliable)") : F("false (reliable)"));
  
  //
  // Setup and configure rf radio
  //
  radio.begin();
  radio.enableDynamicPayloads();
  radio.setCRCLength( RF24_CRC_16 ) ;
  // optionally, increase the delay between retries & # of retries
  radio.setRetries( 15, 5 ) ;
  radio.setAutoAck( true ) ;
  //radio.setPALevel( RF24_PA_LOW ) ;

  //
  // Open pipes to other nodes for communication
  //
  // This simple sketch opens two pipes for these two nodes to communicate
  // back and forth.
  // Open 'our' pipe for writing
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)
  //if ( role == role_ping_out )
  {
    radio.openWritingPipe(pipes[0]);
    radio.openReadingPipe(1,pipes[1]);
  }
  //else
  {
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1,pipes[0]);
  }

  //
  // Dump the configuration of the rf unit for debugging
  //
  radio.printDetails();

  //
  // Start listening
  //
  radio.powerUp() ;
  radio.startListening();
}

void loop(void)
{
  /*//
  // Ping out role.  Repeatedly send the current time
  //

  if (role == role_ping_out)
  {
    // The payload will always be the same, what will change is how much of it we send.
    static char send_payload[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ789012";

    // First, stop listening so we can talk.
    radio.stopListening();

    // Take the time, and send it.  This will block until complete
    Serial.print(F("Now sending length "));
    Serial.println(next_payload_size);
    radio.write( send_payload, next_payload_size, multicast );

    // Now, continue listening
    radio.startListening();

    // Wait here until we get a response, or timeout
    unsigned long started_waiting_at = millis();
    bool timeout = false;
    while ( ! radio.available() && ! timeout )
      if (millis() - started_waiting_at > 500 )
        timeout = true;

    // Describe the results
    if ( timeout )
    {
       Serial.println(F("Failed, response timed out."));
    }
    else
    {
      // Grab the response, compare, and send to debugging spew
      uint8_t len = radio.getDynamicPayloadSize();
      radio.read( receive_payload, len );

      // Put a zero at the end for easy printing
      receive_payload[len] = 0;

      // Spew it
      Serial.print(F("Got response size="));
      Serial.print(len);
      Serial.print(F(" value="));
      Serial.println(receive_payload);
    }
    
    // Update size for next time.
    next_payload_size += payload_size_increments_by;
    if ( next_payload_size > max_payload_size )
      next_payload_size = min_payload_size;

    // Try again 1s later
    delay(250);
  }

  //
  // Pong back role.  Receive each packet, dump it out, and send it back
  //
  if ( role == role_pong_back )*/
  {
    // if there is data ready
    if ( radio.available() )
    {
      // Dump the payloads until we've gotten everything
      bool done = false;
      uint8_t len = 0;
      String s1;
      while (radio.available())
      {
        // Fetch the payload, and see if this was the last one.
      	len = radio.getDynamicPayloadSize();
        char* rx_data = NULL;
        rx_data = (char*)calloc(len, sizeof(char));
        radio.read( rx_data, len );
        
      	// Put a zero at the end for easy printing
        rx_data[len] = 0;
        
      	// Spew it
        Serial.print(F("Got msg size="));
        Serial.print(len);
        Serial.print(F(" value="));
        Serial.println(rx_data);
        
        s1 = String(rx_data);
        free(rx_data);
      }


      if (s1.indexOf("activate")>=0)
      {
        /*char query[] = "schedule1";
        if (queryInteger(query, 8, schedule1))
          s1 = " schedule1:" + String(schedule1);
        char query2[] = "schedule1";
        if (queryInteger(query2, 8, schedule2))
          s1 = " schedule2:" + String(schedule2);*/
        String sched = s1.substring(s1.indexOf(" ")+1);
        int i1 = 0;
        if (sched.indexOf("h")>=0)
        {
          i1 = sched.substring(0, sched.indexOf("h")).toInt()*3600;
          sched = sched.substring(sched.indexOf("h")+1);
        }
        if (sched.indexOf("min")>=0)
        {
          i1 = sched.substring(0, sched.indexOf("min")).toInt()*60;
        }
        if (s1.indexOf("activate1")>=0)
          schedule1 = i1;
        else if (s1.indexOf("activate2")>=0)
          schedule2 = i1;
      }
      else if (s1.indexOf("stop")>=0)
      {
        relay1 = false;
        relay2 = false;
        digitalWrite(HW_RELAY1, LOW);
        digitalWrite(HW_RELAY2, LOW);
        schedule1 = 0;
        schedule2 = 0;
        s1 = "stopped";
      }
      else if (s1.indexOf("status")>=0)
        s1 = "Relay1:" + String((relay1?"ON":"OFF")) + String(" Relay2:") + String((relay2?"ON":"OFF")) + String(" Schedule:") + String(schedule1)  + String(" Schedule:") + String(schedule2) ;
      else
        s1 = "You fail.";

      // convert the response string into a char[]
      len = s1.length();
      char rt_data[len];
      s1.toCharArray(rt_data, len);
      
      // First, stop listening so we can talk
      radio.stopListening();

      // Send the final one back.
      radio.write( rt_data, len, multicast );
      Serial.println(F("Sent response."));

      // Now, resume listening so we catch the next packets.
      radio.startListening();
    }
  }

  if (schedule1 > 0 && ( millis() <= schedule1*1000 ) )
  {
    relay1 = true;
  }

  if (schedule2 > 0 && ( millis() <= schedule2*1000 ) )
  {
    relay2 = true;
  }
}

/*
 * This function sends out a message (theQuery) and expects
 * to receive a response with a number (return value). 
 * It expects the radio be not in listening mode.
 * It will send the query, wait for 500ms, and return. 
 *//*
bool queryInteger(char theQuery[], int queryLength, long& returnValue)
{
  radio.write( theQuery, queryLength, multicast );

  // Now, continue listening
  radio.startListening();

  // Wait here until we get a response, or timeout
  unsigned long started_waiting_at = millis();
  bool timeout = false;
  while ( ! radio.available() && ! timeout )
    if (millis() - started_waiting_at > 500 )
      timeout = true;

  // Describe the results
  if ( timeout )
  {
     Serial.println(F("Failed, response timed out."));
     return false;
  }
  else
  {
    // Fetch the payload, and see if this was the last one.
    uint8_t len = radio.getDynamicPayloadSize();
    char* rx_data = NULL;
    rx_data = (char*)calloc(len, sizeof(char));
    radio.read( rx_data, len );
    
    // Put a zero at the end for easy printing
    rx_data[len] = 0;
    
    // Spew it
    Serial.print(F("Got msg size="));
    Serial.print(len);
    Serial.print(F(" value="));
    Serial.println(rx_data);
    
    String text(rx_data);
    free(rx_data);
    returnValue = text.toInt();
  }
  return true;
}
*/
