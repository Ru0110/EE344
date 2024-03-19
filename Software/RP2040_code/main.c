#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
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

static char event_str[128];

void gpio_event_string(char *buf, uint32_t events);

void gpio_callback(uint gpio, uint32_t events) {
    // Put the GPIO event(s) that just happened into event_str
    // so we can print it
    gpio_event_string(event_str, events);
    printf("GPIO %d %s\n", gpio, event_str);
}

static const char *gpio_irq_str[] = {
        "LEVEL_LOW",  // 0x1
        "LEVEL_HIGH", // 0x2
        "EDGE_FALL",  // 0x4
        "EDGE_RISE"   // 0x8
};

void gpio_event_string(char *buf, uint32_t events) {
    for (uint i = 0; i < 4; i++) {
        uint mask = (1 << i);
        if (events & mask) {
            // Copy this event string into the user string
            const char *event_str = gpio_irq_str[i];
            while (*event_str != '\0') {
                *buf++ = *event_str++;
            }
            events &= ~mask;

            // If more events add ", "
            if (events) {
                *buf++ = ',';
                *buf++ = ' ';
            }
        }
    }
    *buf++ = '\0';
}

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
    spi_set_format( spi,   // SPI instance
                    16,     // Number of bits per transfer
                    SPI_CPOL_1,      // Polarity (CPOL)
                    SPI_CPHA_1,      // Phase (CPHA)
                    SPI_MSB_FIRST);

    // Initialize SPI pins
    gpio_set_function(SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(CS_PIN, GPIO_FUNC_SPI);

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
    SPI_Read_16(spi, ADDR_VERSION);

    /* PERFORM ADE9000 CONFIGURATIONS HERE. LIKE THE INIT ADE9000 FUNCTION IDK */
    resetADE(RESETADE_PIN);
    sleep_ms(1000);
    SetupADE9000(spi);
    
    // Wait before taking measurements
    sleep_ms(2000);

    // init gpio to monitor some pins
    // gpio_set_irq_enabled_with_callback(16, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);  

    uint32_t count = 0;
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
        SPI_Write_16(spi, ADDR_WFB_CFG,0x1000);
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

        //Template to read Power registers from ADE9000 and store data in Arduino MCU
        ReadActivePowerRegs(spi, &powerRegs);
        ReadCurrentRMSRegs(spi, &curntRMSRegs);
        ReadVoltageRMSRegs(spi, &vltgRMSRegs);
        printf("VA_rms: %f\n", (float)vltgRMSRegs.VoltageRMSReg_A*(801.0)/(float)ADE9000_RMS_FULL_SCALE_CODES);
        //printf("VB_rms: %f\n", 0.707*(float)vltgRMSRegs.VoltageRMSReg_B/(float)ADE9000_RMS_FULL_SCALE_CODES);
        //printf("IA_rms: %f\n", 0.707*(float)curntRMSRegs.CurrentRMSReg_A/(float)ADE9000_RMS_FULL_SCALE_CODES);
        //printf("IB_rms: %f\n", 0.707*(float)curntRMSRegs.CurrentRMSReg_B/(float)ADE9000_RMS_FULL_SCALE_CODES);
        printf("Power in A channel (WATT): %f\n", (float)powerRegs.ActivePowerReg_A/(float)ADE9000_WATT_FULL_SCALE_CODES);
        printf("Power in B channel (WATT): %f\n", (float)powerRegs.ActivePowerReg_B/(float)ADE9000_WATT_FULL_SCALE_CODES);        
        printf("VERSION: %d\n", (uint16_t)SPI_Read_16(spi, ADDR_VERSION));
        printf("MEASURE COUNT: %d\n", count++);
        printf("\n");
    }
    return 0;
}