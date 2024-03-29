#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "ADE9000API_RP2040.h"
#include "lwipopts.h"

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
    if (cyw43_arch_init()) {
        printf("Wifi init failed");
        return -1;
    }

    // Ports
    spi_inst_t *spi = spi0;

    // Initialize SPI port at specified speed
    spi_init(spi, SPI_SPEED);

    // Set SPI format. We set it to transfer 16 bits every transfer. SPI MODE 0
    spi_set_format( spi,   // SPI instance
                    16,     // Number of bits per transfer
                    SPI_CPOL_0,      // Polarity (CPOL)
                    SPI_CPHA_0,      // Phase (CPHA)
                    SPI_MSB_FIRST);

    // Initialize SPI pins
    gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);

    // Initialize other pins 
    gpio_init(PM1_PIN);
    gpio_init(IRQ0_PIN);
    gpio_init(IRQ1_PIN);
    gpio_init(ZX_PIN);
    gpio_init(DREADY_PIN);
    gpio_init(RESETADE_PIN);
    gpio_init(CS_PIN);
    gpio_set_dir(PM1_PIN, GPIO_OUT);
    gpio_set_dir(RESETADE_PIN, GPIO_OUT);
    gpio_set_dir(IRQ0_PIN, GPIO_IN);
    gpio_set_dir(IRQ1_PIN, GPIO_IN);
    gpio_set_dir(ZX_PIN, GPIO_IN);
    gpio_set_dir(DREADY_PIN, GPIO_IN);
    gpio_set_dir(CS_PIN, GPIO_OUT);

    gpio_put(CS_PIN, 1);
    gpio_put(PM1_PIN, 0); // start in normal power mode
    gpio_put(RESETADE_PIN, 1); // reset is active low

    /* PERFORM ADE9000 CONFIGURATIONS HERE. LIKE THE INIT ADE9000 FUNCTION IDK */
    resetADE(RESETADE_PIN);
    sleep_ms(1000);
    SetupADE9000(spi, CS_PIN);
    
    // Wait before taking measurements
    sleep_ms(3000);

    // Workaround: perform throw-away read to make SCK idle high. idk, just copied
    SPI_Read_16(spi, CS_PIN, ADDR_VERSION);

    // print DEBUG info for the spi bus
    //printf("Baudrate: %d\n", spi_get_baudrate(spi));
    //printf("Instance number: %d\n", spi_get_index(spi));
    //printf("SPI device writeable? %s\n", spi_is_writable(spi) ? "True" : "False");
    //printf("SPI device readable? %s\n", spi_is_readable(spi)? "True": "False");
    //printf("ADDRESS: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_VERSION));
    //printf("\n");

    uint32_t count = 0;
    // Loop forever
    while (true) {
        // toggle led
        gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
        gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(500);

        //uint32_t temp;
        /*Read and Print Resampled data*/
        /*Start the Resampling engine to acquire 4 cycles of resampled data*/
        /*SPI_Write_16(spi, ADDR_WFB_CFG,0x1000);
        SPI_Write_16(spi, ADDR_WFB_CFG,0x1010);
        sleep_ms(100); //approximate time to fill the waveform buffer with 4 line cycles  
        SPI_Burst_Read_Resampled_Wfb(spi, 0x800,WFB_ELEMENT_ARRAY_SIZE,&resampledData);   // Burst read function
        printf("==========Waveform buffer============\n");
        for(temp=0;temp<WFB_ELEMENT_ARRAY_SIZE;temp++)
            {
            printf("VA: ");
            printf("%d\n", resampledData.VA_Resampled[temp]);
            printf("IA: ");
            printf("%d\n", (float)resampledData.IA_Resampled[temp]/(float)ADE9000_RESAMPLED_FULL_SCALE_CODES);
            printf("VB: ");
            printf("%d\n", (float)resampledData.VB_Resampled[temp]/(float)ADE9000_RESAMPLED_FULL_SCALE_CODES);
            printf("IB: ");
            printf("%d\n", (float)resampledData.IB_Resampled[temp]/(float)ADE9000_RESAMPLED_FULL_SCALE_CODES);
            printf("\n");
        } 
        printf("====================================\n");
        */

        //Template to read Power registers from ADE9000 and store data in Arduino MCU
        ReadActivePowerRegs(spi, CS_PIN, &powerRegs);
        ReadCurrentRMSRegs(spi, CS_PIN, &curntRMSRegs);
        ReadVoltageRMSRegs(spi, CS_PIN, &vltgRMSRegs);
        printf("VA_rms: %f\n", (double)(vltgRMSRegs.VoltageRMSReg_A*801)*0.707/(double)ADE9000_RMS_FULL_SCALE_CODES);
        printf("IA_rms: %f\n", 0.707*(double)(curntRMSRegs.CurrentRMSReg_A*2000)/(10.2*(double)(ADE9000_RMS_FULL_SCALE_CODES)));
        printf("Power in A channel (WATT): %f\n", (double)(powerRegs.ActivePowerReg_A*78327)/(double)ADE9000_WATT_FULL_SCALE_CODES);
        printf("VERSION (should be 254): %d\n", SPI_Read_16(spi, CS_PIN, ADDR_VERSION));
        printf("RUN REGISTER: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_RUN));
        printf("ZERO CROSSING THRESHOLD: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_ZXTHRSH));
        printf("MEASURE COUNT: %d\n", count++);
        printf("\n");
    }
    return 0;
}