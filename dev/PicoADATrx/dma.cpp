/*
 DMA support for PIO bitstream receiver
 */
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "dma.h"

uint32_t DMAbuffer[CHUNK_SIZE * 2];
dma_channel_t dma_channels[2] {{DMAbuffer},{DMAbuffer+CHUNK_SIZE}};


static void DMAchannelConfig(PIO pio, uint sm, dma_channel_t& channel, int chain_to)
{
    //----------------------------------------------------------------------------
    // Data transfer DMA channel
    // This is from the PIO logic analyser example:
/*
    Setting 	    Default
    =======================
    Read Increment  true
    Write Increment false
    DReq            DREQ_FORCE
    Chain to        self
    Data size       DMA_SIZE_32
    Ring            write=false, size=0 (i.e. off)
    Byte Swap       false
    Quiet IRQs      false
    High Priority   false
    Channel Enable  true
    Sniff Enable    false
*/
    dma_channel_config c = dma_channel_get_default_config(channel.channel);

    // change defaults as needed:
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

    // Trigger ctrl_chan when channel completes
    channel_config_set_chain_to(&c, chain_to);
 
    // Remix from control_blocks example and logic_analyser
    dma_channel_configure(channel.channel, &c,
        channel.addr,   // Destination pointer
        &pio->rxf[sm],  // Source pointer
        CHUNK_SIZE,     // Number of transfers
        false           // Don't start yet.
    );

    // Tell the DMA to raise IRQ line 0 when the channel finishes a block
    dma_channel_set_irq0_enabled(channel.channel, true);

}

__inline__ bool isSync(uint32_t& word) { return 0x0020'0000 == (word & 0xFFE0'0000);}

volatile uint32_t transfer_count;
static void dma_arm(dma_channel_t& ch)
{
    // reset write address, and don't trigger: chain will do that
    dma_channel_set_write_addr(ch.channel, ch.addr, false);

    // no need to set count; see 2.5.1.2 - reloads to most recent written value
    transfer_count++; // count transfers made

    // say this channel has valid data if the first
    // word contains the sync frame
    ch.valid = isSync(*ch.addr);  
}


// Handler for DMA IRQ0
static void dma_handler_0(void) 
{
    uint32_t dma_chan = dma_hw->ints0; // get all triggered requests

    dma_chan = 31 - __builtin_clz(dma_chan); // highest-numbered triggered interrupt
    int which = -1;

    // we can only deal with our DMA channels: this handler will need
    // extending if more channels are configured to use DMA_IRQ0
    if (dma_channels[0].channel == dma_chan) which = 0;
    if (dma_channels[1].channel == dma_chan) which = 1;
    if (which >= 0)
    {
        dma_arm(dma_channels[which]);
        if (0 == which)
        {
            dma_channel_t& ch = dma_channels[which];

            switch (ch.lockState)
            {
                case locked:
                    if (!dma_channels[which].valid) // sync frame is not at start
                    {
                        // try to re-lock by slipping one word at a time
                        dma_channel_set_transfer_count(ch.channel, CHUNK_SIZE - 1, false); 
                        ch.lockState = unlocked;
                    }
                    break;

                case unlocked:
                    if (isSync(ch.addr[7])) // wasn't locked...
                    {
                        ch.lockState = locking; // ... it soon will be!
                        // get all the words next time
                        dma_channel_set_transfer_count(ch.channel, CHUNK_SIZE, false);
                    }
                    break;

                case locking:
                    ch.lockState = locked;
                    break;
            }            
        }   
    }
    // Clear the interrupt request.
    dma_hw->ints0 = 1u << dma_chan;
}


void DMAinit(PIO pio, uint sm)
{
    // This is based on the control_blocks.c example:

    // Allocate two data channels for ping-pong configuration:
    dma_channels[0].channel = dma_claim_unused_channel(true);
    dma_channels[1].channel = dma_claim_unused_channel(true);

    DMAchannelConfig(pio, sm, dma_channels[0], dma_channels[1].channel);
    DMAchannelConfig(pio, sm, dma_channels[1], dma_channels[0].channel);

    // Configure the processor to run dma_handler_0() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler_0);
    irq_set_enabled(DMA_IRQ_0, true);

    //----------------------------------------------------------------------------
    // Everything is ready to go. Start the first data channel;
    // everything is automatic from here.
    dma_start_channel_mask(1u << dma_channels[0].channel);
}

