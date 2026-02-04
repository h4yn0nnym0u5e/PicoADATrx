#include <Arduino.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "dma.h"

extern PIO thePIO;
extern uint theSM;
//======================================================
void toggleLED(uint32_t intvl)
{
  static bool ledState = false;
  static uint32_t last = millis();

  if (millis() - last >= intvl)
  {
    last = millis();
    ledState = !ledState;
    digitalWrite(LED_BUILTIN,ledState);
    if (Serial)
    {
      int words = SAMPLE_WORDS*4;
      uint32_t buffer[words];
      memcpy(buffer,DMAbuffer+CHUNK_SIZE,sizeof buffer);

      Serial.printf("%6d: %6d ", millis(), transfer_count);
      Serial.printf("%d ",thePIO->fdebug & PIO_FDEBUG_RXSTALL_BITS);
      thePIO->fdebug = PIO_FDEBUG_RXSTALL_BITS;
      Serial.printf("%d ",dma_channels[0].lockState);
      for (int i=0;i<words;i++)
        Serial.printf("%08X ", buffer[i]);
      Serial.println();
    }
  }
}

//======================================================
void setup() 
{    
  pinMode(LED_BUILTIN,OUTPUT);

  while (!Serial)
    toggleLED(100);

  int ready = millis();
  delay(100);
  Serial.printf("\n\n=====================\nRunning after %dms...\n", ready);
  PIOinit();
  DMAinit(thePIO, theSM);

  // start the bitstream
  pio_sm_set_enabled(thePIO, theSM, true);
}


//======================================================
void loop() 
{
  toggleLED(290);
}
