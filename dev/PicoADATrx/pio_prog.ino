/*
 * PIO stuff from Wokwi on-line pioasm is static
 */

#include "delay.pio.h"
#include "bitstream.pio.h"

PIO thePIO = pio0;

void PIOinit(void)
{
  uint end;

  // re-sync ADAT signal
  if ((end = delay_program_init(thePIO,16,4))) // loaded OK
    Serial.printf("echo PIO started: %d words remaining\n", end);
  
  // delay to make a high pulse on edge, signalling
  // set bits in the datastream. Uses hardware XOR 
  // gate, inputs on 4+5, output on 6
  if ((end = delay_program_init(thePIO,4,5))) // loaded OK
    Serial.printf("toggle PIO started: %d words remaining\n", end);
  
  // create bitstream from pulses
  if ((end = bitstream_program_init(thePIO, 14, 6))) // set GPIO15, pulses in on GPIO11
    Serial.printf("bitstream PIO started: %d words remaining\n", end);      

}