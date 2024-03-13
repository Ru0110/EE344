#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ADE9000API_RP2040.h"

/* SPI DEFINITIONS */
#define SPI_SPEED 5000000  // 5 MHz
#define SCK_PIN 18
#define MOSI_PIN 19
#define MISO_PIN 16
#define CS_PIN 17

/* GPIO DEFINITIONS */
#define RESETADE_PIN 9
#define IRQ0_PIN 10
#define IRQ1_PIN 11
#define ZX_PIN 12
#define DREADY_PIN 13
#define PM1_PIN 14
#define BUZZER 15


ResampledWfbData resampledData; // Resampled Data
ActivePowerRegs powerRegs;     // Declare powerRegs of type ActivePowerRegs to store Active Power Register data
CurrentRMSRegs curntRMSRegs;   //Current RMS
VoltageRMSRegs vltgRMSRegs;    //Voltage RMS
VoltageTHDRegs voltageTHDRegsnValues; //Voltage THD

int main() {

    // Initialize chosen serial port (USB for now)
    stdio_init_all();

    // define led pin
    const uint led_pin = 25;

    // Initialize LED pin
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    // Ports
    spi_inst_t *spi = spi0;

    // Initialize SPI port at specified speed
    spi_init(spi, SPI_SPEED);

    // Set SPI format. We set it to transfer 16 bits every transfer
    spi_set_format( spi0,   // SPI instance
                    16,     // Number of bits per transfer
                    1,      // Polarity (CPOL)
                    1,      // Phase (CPHA)
                    SPI_MSB_FIRST);

    // Initialize SPI pins
    gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);

    // Initialize CS pin high
    gpio_init(CS_PIN);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    gpio_put(CS_PIN, 1);

    // Initialize other pins 
    gpio_init(PM1_PIN);
    gpio_init(IRQ0_PIN);
    gpio_init(IRQ1_PIN);
    gpio_init(ZX_PIN);
    gpio_init(DREADY_PIN);
    gpio_init(RESETADE_PIN);
    gpio_set_dir(PM1_PIN, GPIO_OUT);
    gpio_set_dir(RESETADE_PIN, GPIO_OUT);
    gpio_set_dir(IRQ0_PIN, GPIO_IN);
    gpio_set_dir(IRQ1_PIN, GPIO_IN);
    gpio_set_dir(ZX_PIN, GPIO_IN);
    gpio_set_dir(DREADY_PIN, GPIO_IN);

    gpio_put(PM1_PIN, 0); // start in normal power mode
    gpio_put(RESETADE_PIN, 1); // reset is active low

    // Workaround: perform throw-away read to make SCK idle high. idk, just copied
    SPI_Read_16(spi, CS_PIN, ADDR_VERSION);

    /* PERFORM ADE9000 CONFIGURATIONS HERE. LIKE THE INIT ADE9000 FUNCTION IDK */
    resetADE(RESETADE_PIN);
    sleep_ms(1000);
    SetupADE9000(spi, CS_PIN);
    
    // Wait before taking measurements
    sleep_ms(2000);

    // Loop forever
    while (true) {
        // toggle led
        gpio_put(led_pin, 1);
        sleep_ms(500);
        gpio_put(led_pin, 0);
        sleep_ms(500);

        uint32_t temp;
        /*Read and Print Resampled data*/
        /*Start the Resampling engine to acquire 4 cycles of resampled data*/
        SPI_Write_16(spi, CS_PIN, ADDR_WFB_CFG,0x1000);
        SPI_Write_16(spi, CS_PIN, ADDR_WFB_CFG,0x1010);
        sleep_ms(100); //approximate time to fill the waveform buffer with 4 line cycles  
        SPI_Burst_Read_Resampled_Wfb(spi, CS_PIN, 0x800,WFB_ELEMENT_ARRAY_SIZE,&resampledData);   // Burst read function
        
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

        ReadVoltageRMSRegs(spi, CS_PIN, &vltgRMSRegs);    //Template to read Power registers from ADE9000 and store data in Arduino MCU
        ReadActivePowerRegs(spi, CS_PIN, &powerRegs);
        printf("AVRMS:");        
        printf("%d\n", vltgRMSRegs.VoltageRMSReg_A); //Print AVRMS register
        printf("AWATT:");        
        printf("%d\n", powerRegs.ActivePowerReg_A); //Print AWATT register

    }
}