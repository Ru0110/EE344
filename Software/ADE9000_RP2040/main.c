#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ADE9000API_RP2040.h"
#define SPI_SPEED 5000000     //SPI Speed
ResampledWfbData resampledData; // Resampled Data

int main() {

    // define led pin
    const uint led_pin = 25;

    // Initialize LED pin
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

     // Buffer to store raw reads
    uint8_t data[6];

    // Pins
    const uint cs_pin = 17;
    const uint sck_pin = 18;
    const uint mosi_pin = 19;
    const uint miso_pin = 16;
    const uint pm1_pin = 14;
    const uint irq0_pin = 10;
    const uint irq1_pin = 11;
    const uint zx_pin = 12;
    const uint dready_pin = 13;
    const uint resetade_pin = 9;

    // Ports
    spi_inst_t *spi = spi0;

    // Initialize chosen serial port
    stdio_init_all();

    // Initialize CS pin high
    gpio_init(cs_pin);
    gpio_set_dir(cs_pin, GPIO_OUT);
    gpio_put(cs_pin, 1);

    // Initialize other pins 
    gpio_init(pm1_pin);
    gpio_init(irq0_pin);
    gpio_init(irq1_pin);
    gpio_init(zx_pin);
    gpio_init(dready_pin);
    gpio_init(resetade_pin);
    gpio_set_dir(pm1_pin, GPIO_OUT);
    gpio_set_dir(resetade_pin, GPIO_OUT);
    gpio_set_dir(irq0_pin, GPIO_IN);
    gpio_set_dir(irq1_pin, GPIO_IN);
    gpio_set_dir(zx_pin, GPIO_IN);
    gpio_set_dir(dready_pin, GPIO_IN);

    gpio_put(pm1_pin, 0);
    gpio_put(resetade_pin, 1);

    // Initialize SPI port at specified speed
    spi_init(spi, SPI_SPEED);

    // Set SPI format. We set it to transfer 16 bits every transfer
    spi_set_format( spi0,   // SPI instance
                    16,     // Number of bits per transfer
                    1,      // Polarity (CPOL)
                    1,      // Phase (CPHA)
                    SPI_MSB_FIRST);

    // Initialize SPI pins
    gpio_set_function(sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(miso_pin, GPIO_FUNC_SPI);

    // Workaround: perform throw-away read to make SCK idle high
    //reg_read(spi, cs_pin, REG_DEVID, data, 1); // CHANGE THIS -------------------------------

    /* PERFORM ADE9000 CONFIGURATIONS HERE. LIKE THE INIT ADE9000 FUNCTION IDK */
    resetADE(resetade_pin);
    sleep_ms(1000);
    SetupADE9000(spi, cs_pin);
    
    // Wait before taking measurements
    sleep_ms(2000);

    // Loop forever
    while (true) {
        // Print results
        gpio_put(led_pin, 1);
        sleep_ms(500);
        gpio_put(led_pin, 0);
        sleep_ms(500);

        uint32_t temp;
        /*Read and Print Resampled data*/
        /*Start the Resampling engine to acquire 4 cycles of resampled data*/
        SPI_Write_16(spi, cs_pin, ADDR_WFB_CFG,0x1000);
        SPI_Write_16(spi, cs_pin, ADDR_WFB_CFG,0x1010);
        sleep_ms(100); //approximate time to fill the waveform buffer with 4 line cycles  
        SPI_Burst_Read_Resampled_Wfb(spi, cs_pin, 0x800,WFB_ELEMENT_ARRAY_SIZE,&resampledData);   // Burst read function
        
        for(temp=0;temp<WFB_ELEMENT_ARRAY_SIZE;temp++)
            {
            printf("VA: ");
            printf("%d\n", resampledData.VA_Resampled[temp]);
            printf("IA: ");
            printf("%d\n", resampledData.IA_Resampled[temp]);
            printf("VB: ");
            printf("%d\n", resampledData.VB_Resampled[temp]);
            printf("IB: ");
            printf("%d\n", resampledData.IB_Resampled[temp]);
            printf("\n");
        } 

    }
}