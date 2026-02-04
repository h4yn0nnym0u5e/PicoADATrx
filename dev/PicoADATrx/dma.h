enum ls {locked, unlocked, locking};
typedef struct 
{
    uint32_t* addr;
    ls  lockState;
    uint32_t channel;
    bool valid;
} dma_channel_t;

#define BLOCK_SAMPLES 128
#define SAMPLE_WORDS    8 

#define CHUNK_SIZE (BLOCK_SAMPLES * SAMPLE_WORDS) // 1024 32-bit words

extern volatile uint32_t transfer_count;
extern uint32_t DMAbuffer[];
extern dma_channel_t dma_channels[2];
extern void DMAinit(PIO pio, uint sm);

