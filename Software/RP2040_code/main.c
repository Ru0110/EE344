#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/multicore.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/vreg.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "ADE9000API_RP2040.h"
#include "lwipopts.h"
#include "bme280.h"
#include "blocking.h"
#include "flash_utils.h"
#include "tcp_server.h"
#include "ws2812.pio.h"

#define DELAY_CONN 400
#define DELAY_NOCONN 2000

/* NEOPIXEL DEFINITIONS */
#define IS_RGBW true
#define NUM_PIXELS 1
#define WS2812_PIN 9

/* ADE9000 POWER QUALITY INTERRUPT DEFINITIONS */
#define DIP_LEVEL 5.0 // Dip threshold. Must be float
#define SWELL_LEVEL 9.0 // Swell threshold. Must be float
#define DIP_SWELL_CYCLE 100 // Number of halfcycles of sustained dip or swell before interrupt
#define OVERCURRENT_LEVEL 1.0 //Overcurrent threshold. Must be float 
#define MASK1_USER (1 << 23  | 1 << 20 | 1 << 17) // Enable dip_A, swell_A, and overcurrent detection
#define EVENTMASK_USER (1 << 3 | 1 << 0) // Enable events for dip_A, swell_A
#define CONFIG3_USER (1 << 12 | 1 << 2) // Enable overcurrent detection and peak measurement on channel A 
uint32_t event_start = 0;
uint32_t event_end = 0;
uint32_t event_status;

/* SPI DEFINITIONS */
#define SPI_SPEED 5000000  //1 MHz 
#define SCK_PIN 18
#define MOSI_PIN 19
#define MISO_PIN 16
#define CS_PIN 17

/* I2C DEFINITIONS */
#define SDA_PIN 4
#define SCL_PIN 5
#define I2C_SPEED 100000 // 100 kHz

/* GPIO DEFINITIONS */
#define RESETADE_PIN 20
#define IRQ0_PIN 10
#define IRQ1_PIN 22
#define ZX_PIN 12
#define DREADY_PIN 13
#define PM1_PIN 14
#define BUZZER 26

/* FLASH CONFIGURATION ADDRESSES */
#define ADDR_PERSISTENT getAddressPersistent()

/* WIFI DETAILS */
#define WIFI_SSID "Shobhit"
#define WIFI_PASSWORD "nananana"

/* MULTICORE DEFINITIONS */
/*#define START_LOGGING 1     // signal for first core to start data logging. Make sure its a low number
#define SENSOR_EMERGENCY 2 */ 
typedef struct {            // to faciliate sensor data transfer 
    uint32_t time;
    float VArms;
    float VBrms;
    float IArms;
    float IBrms;
    float PowerA;
    float PowerB;
    float PfA;
    float PfB;
    float freqA;
    float freqB;
    float Temperature;
    float Pressure;
    float Humidity;
    uint32_t emergency;     // From LSB: (DipA)(DipB)(SwellA)(SwellB)(OIA)(OIB)(Temp)(Pressure)(Humidity)(NoLoadA)(NoLoadB)
    int32_t waveform_buffer[WFB_ELEMENT_ARRAY_SIZE];
} sensorData;

#define DATA_SIZE sizeof(sensorData)

// Data structures and sensor objects
//ResampledWfbData resampledData; // Resampled Data
WaveformData FixedWaveformData; // Fized rate data from waveform
ActivePowerRegs powerRegs;     // Declare powerRegs of type ActivePowerRegs to store Active Power Register data
CurrentRMSRegs curntRMSRegs;   //Current RMS
VoltageRMSRegs vltgRMSRegs;    //Voltage RMS
PeriodRegs freqData;            // Frequency of channels
PowerFactorRegs pfData;         // Power factors
VoltageTHDRegs voltageTHDRegsnValues; //Voltage THD
bme280_t bme280; //BME sensor struct
bme280_reading_t r; //BME data struct
sensorData data; // struct holding all sensor data;

// Ports
spi_inst_t *spi = spi0;
i2c_inst_t *i2c = i2c0;
PIO pio = pio0;
int sm = 0;

/*void core0_sio_irq() {
    // Just record the latest entry
    while (multicore_fifo_rvalid())
        uint32_t core0_rx_val = multicore_fifo_pop_blocking();

    multicore_fifo_clear_irq();
}

void core1_sio_irq() {
    // Just record the latest entry
    while (multicore_fifo_rvalid())
        core1_rx_val = multicore_fifo_pop_blocking();

    multicore_fifo_clear_irq();
}*/

uint32_t count = 0;
uint32_t curr_time = 0;

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return
            ((uint32_t) (r) << 8) |
            ((uint32_t) (g) << 16) |
            (uint32_t) (b);
}

bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

static int scan_result(void *env, const cyw43_ev_scan_result_t *result) {
    if (result) {
        printf("ssid: %-32s rssi: %4d chan: %3d mac: %02x:%02x:%02x:%02x:%02x:%02x sec: %u\n",
            result->ssid, result->rssi, result->channel,
            result->bssid[0], result->bssid[1], result->bssid[2], result->bssid[3], result->bssid[4], result->bssid[5],
            result->auth_mode);
    }
    return 0;
}

void getData () {
    count += 1;
    curr_time = to_ms_since_boot(get_absolute_time());

    // Read from BME280
    bme280_read(&bme280, &r);

    //Template to read Power registers from ADE9000 and store data in Arduino MCU
    ReadActivePowerRegs(spi, CS_PIN, &powerRegs);
    ReadCurrentRMSRegs(spi, CS_PIN, &curntRMSRegs);
    ReadVoltageRMSRegs(spi, CS_PIN, &vltgRMSRegs);
    ReadPowerFactorRegsnValues(spi, CS_PIN, &pfData);
    ReadPeriodRegsnValues(spi, CS_PIN, &freqData);

    /*Read Waveform data*/
    SPI_Write_16(spi, CS_PIN, ADDR_WFB_CFG, 0x1000); // Stop waveform sampling
    SPI_Write_16(spi, CS_PIN, ADDR_WFB_CFG, 0x1220); // Store the fixed data rate values from sinc4 + LPF
    SPI_Write_16(spi, CS_PIN, ADDR_WFB_CFG, 0x1230); // Start sampling
    sleep_ms(200); //approximate time to fill the waveform buffer with 4 line cycles  
    SPI_Burst_Read_Fixed_Rate(spi, CS_PIN, 0x800, WFB_ELEMENT_ARRAY_SIZE, &FixedWaveformData);   // Burst read function

    // Update data struct
    data.time = curr_time;
    data.VArms = voltage_gain*(float)vltgRMSRegs.VoltageRMSReg_A;
    data.VBrms = voltage_gain*(float)vltgRMSRegs.VoltageRMSReg_B;
    data.IArms = current_gain*(float)curntRMSRegs.CurrentRMSReg_A;
    data.IBrms = current_gain*(float)curntRMSRegs.CurrentRMSReg_B;
    data.PowerA = power_gain*(float)powerRegs.ActivePowerReg_A;
    data.PowerB = power_gain*(float)powerRegs.ActivePowerReg_B;
    data.PfA = pfData.PowerFactorValue_A;
    data.PfB = pfData.PowerFactorValue_B;
    data.freqA = freqData.FrequencyValue_A;
    data.freqB = freqData.FrequencyValue_B;
    data.Temperature = r.temperature;
    data.Pressure = r.pressure;
    data.Humidity = r.humidity;
    data.emergency = 0;
    memcpy(data.waveform_buffer, FixedWaveformData.VA_waveform, WFB_ELEMENT_ARRAY_SIZE*4); // each entry is four bytes long
}

void printWaveformBuffer() {
    printf("\n==========Waveform buffer============\n");
    uint32_t temp;
    for(temp=0;temp<WFB_ELEMENT_ARRAY_SIZE;temp++)
        {
        printf("VA: %d\n", data.waveform_buffer[temp]);
        //printf("IA: %f\n", resampledData.IA_Resampled[temp]);
        //printf("VB: %d\n", resampledData.VB_Resampled[temp]);
        //printf("IB: %d\n", resampledData.IB_Resampled[temp]);
        //printf("VC: %d\n", resampledData.VC_Resampled[temp]);
        //printf("IC: %d\n", resampledData.IC_Resampled[temp]);
        //printf("IN: %d\n", resampledData.IN_Resampled[temp]);
    } 
    printf("====================================\n");
}

void printData () {
    printf("===================================\n");
    printf("MEASURE COUNT: %d\n", count);
    printf("Current time: %d\n", curr_time);
    printf("Temperature: %.2f°C\nPressure: %.2f Pa\nHumidity: %.2f%%\n",
        data.Temperature, data.Pressure, data.Humidity);
    printf("-----------------------------------\n");
    printf("VA_rms: %f\n", data.VArms);
    printf("IA_rms: %f\n", data.IArms);
    printf("freq_A: %f\n", data.freqA);        
    printf("Power factor A: %f\n", data.PfA);
    printf("Power in A channel (WATT): %f\n", data.PowerA);
    printf("VB_rms: %f\n", data.VBrms);
    printf("IB_rms: %f\n", data.IBrms);
    printf("freq_B: %f\n", data.freqB);        
    printf("Power factor B: %f\n", data.PfB);
    printf("Power in B channel (WATT): %f\n", data.PowerB);
    printf("VERSION (should be 254): %d\n", SPI_Read_16(spi, CS_PIN, ADDR_VERSION));
    printf("RUN REGISTER: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_RUN));
    printf("===================================\n");
    
    printf("Size of structure: %d bytes\n", DATA_SIZE);
    printf("\n");
}

void irq0_callback(uint gpio, uint32_t events) {
    printf("Interrupt at IRQ0 at GPIO %u!\n", gpio);
    // Read the STATUS0 register
    printf("STATUS0: %u\n", SPI_Read_32(spi, CS_PIN, ADDR_STATUS0));
}

void irq1_callback(uint gpio, uint32_t events) {
    printf("Interrupt at IRQ1 at GPIO %u!\n", gpio);
    // Read the STATUS1 register
    uint32_t status1 = SPI_Read_32(spi, CS_PIN, ADDR_STATUS1);
    printf("STATUS1: %u\n", status1);

    if (status1 & (1 << 23)) { // DIP in channel A
        // Read the peak voltage
        double peak = (double)(SPI_Read_32(spi, CS_PIN, ADDR_VPEAK) & ((1 << 24) - 1))*32.0/voltage_gain;
        status1 ^= (1 << 23);
        if (data.emergency & (1 << 0)) {
            printf("Dip event over in channel A\n");
            printf("Peak voltage during dip: %f\n", peak);
        } else {
            data.emergency |= (1 << 0);
            printf("Dip event started in channel A\n");  
            printf("Peak voltage before dip: %f\n", peak);
        }
    }

    if (status1 & (1 << 20)) { // Swell in channel A
        // Read the peak voltage
        double peak = (double)(SPI_Read_32(spi, CS_PIN, ADDR_VPEAK) & ((1 << 24) - 1))*32.0/voltage_gain;
        status1 ^= (1 << 20);
        if (data.emergency & (1 << 2)) {
            printf("Swell event over in channel A\n");
            printf("Peak voltage during swell: %f\n", peak);
        } else {
            data.emergency |= (1 << 2);
            printf("Swell event started in channel A\n");  
            printf("Peak voltage before swell: %f\n", peak);
        }
    }

    if (status1 & (1 << 17)) { // Overcurrent in channel A
        // Read the peak current
        double peak = (double)(SPI_Read_32(spi, CS_PIN, ADDR_IPEAK) & ((1 << 24) - 1))*32.0/current_gain;
        status1 ^= (1 << 17);
        if (data.emergency & (1 << 4)) {
            printf("Overcurrent event over in channel A\n");
            printf("Peak current during overcurrent: %f\n", peak);
        } else {
            data.emergency |= (1 << 4);
            printf("Overcurrent event started in channel A\n");  
            printf("Peak current before overcurrent: %f\n", peak);
        }
    }

    // Clear the status register to stop the interrupt
    SPI_Write_32(spi, CS_PIN, ADDR_STATUS1, status1);

}

void event_callback(uint gpio, uint32_t events) {
    printf("Event interrupt triggered at GPIO %u!\n", gpio);
    // Read the event status register
    event_status = SPI_Read_32(spi, CS_PIN, ADDR_EVENT_STATUS);
    printf("Event status register: %u\n", event_status);

    if (gpio_get(gpio)) { // event started
        event_start = to_ms_since_boot(get_absolute_time());

        if (event_status & (1 << 0)) {
            printf("We are in a dip event\n");
        }

        if (event_status & (1 << 3)) {
            printf("We are in a swell event\n");
        }
    } else { // event ended
        event_end = to_ms_since_boot(get_absolute_time());
        printf("Event ended after time: %u ms\n", event_end - event_start);
    }
    
}

void dummy_callback(uint gpio, uint32_t events) {
    printf("GPIO TOGGLE ON %u\n", gpio);
    printf("EVENTS: %u\n", events);
}

int main() {

    // Initialize chosen serial port (USB for now) and WiFi
    stdio_init_all();

    // Start the statemachine or the neopixel
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
    put_pixel(urgb_u32(0,0,0x80)); // light blue?

    //sleep_ms(4000);
    //printf("ENTERING PROGRAM\n");

    if (cyw43_arch_init()) {
        printf("Wifi init failed");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    // Initialize Ports
    spi_init(spi, SPI_SPEED);
    i2c_init(i2c, I2C_SPEED);

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

    //Initialize I2C pins
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Initialize interrupt pins. They go low when active so we will only monitor falling edge
    // gpio_set_irq_enabled_with_callback(IRQ0_PIN,  GPIO_IRQ_EDGE_FALL, true, &irq0_callback);
    // gpio_set_irq_enabled_with_callback(IRQ1_PIN,  GPIO_IRQ_EDGE_FALL, true, &irq1_callback);
    // Initialize event pin interrupts. Both edges to measure time of event 
    //gpio_init(DREADY_PIN);
    //gpio_set_dir(DREADY_PIN, GPIO_IN);
    //gpio_set_irq_enabled_with_callback(MISO_PIN, GPIO_IRQ_EDGE_RISE, true, &dummy_callback);

    // Initialize other pins 
    gpio_init(PM1_PIN);
    gpio_init(ZX_PIN);
    gpio_init(RESETADE_PIN);
    gpio_init(CS_PIN);
    gpio_set_dir(PM1_PIN, GPIO_OUT);
    gpio_set_dir(RESETADE_PIN, GPIO_OUT);
    gpio_set_dir(ZX_PIN, GPIO_IN);
    gpio_set_dir(CS_PIN, GPIO_OUT);

    gpio_put(CS_PIN, 1);
    gpio_put(PM1_PIN, 0); // start in normal power mode
    gpio_put(RESETADE_PIN, 1); // reset is active low

    sleep_ms(1000);

    // Check if flash reservation works
    printf("\nPersistent Flash (ROM) address at: %d\n\n", ADDR_PERSISTENT);

    // Perform i2c bus scan to see if BME280 and EEPROM are connected
    /*printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_timeout_us(i2c, addr, &rxdata, 1, false, 10000);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n\n");*/

    /*absolute_time_t scan_time = nil_time;
    bool scan_in_progress = false;

    if (absolute_time_diff_us(get_absolute_time(), scan_time) < 0) {
        if (!scan_in_progress) {
            cyw43_wifi_scan_options_t scan_options = {0};
            int err = cyw43_wifi_scan(&cyw43_state, &scan_options, NULL, scan_result);
            if (err == 0) {
                printf("\nPerforming wifi scan\n");
                scan_in_progress = true;
            } else {
                printf("Failed to start scan: %d\n", err);
                scan_time = make_timeout_time_ms(10000); // wait 10s and scan again
            }
        } else if (!cyw43_wifi_scan_active(&cyw43_state)) {
            scan_time = make_timeout_time_ms(10000); // wait 10s and scan again
            scan_in_progress = false; 
        }
    }

    sleep_ms(2000); // wait for scan results to arrive
    */

   // print DEBUG info for the spi bus
    /*printf("Baudrate: %d\n", spi_get_baudrate(spi));
    printf("Instance number: %d\n", spi_get_index(spi));
    printf("SPI device writeable? %s\n", spi_is_writable(spi) ? "True" : "False");
    printf("SPI device readable? %s\n", spi_is_readable(spi)? "True": "False");
    printf("ADDRESS: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_VERSION));
    printf("\n");*/

    /* PERFORM ADE9000 CONFIGURATIONS HERE. LIKE THE INIT ADE9000 FUNCTION IDK */
    printf("Reset cycle for the ADE9000\n");
    resetADE(RESETADE_PIN);
    sleep_ms(200);
    SetupADE9000(spi, CS_PIN);
    sleep_ms(300);

    // Workaround: perform throw-away read to make SCK idle high. idk, just copied
    printf("Initial VERSION (Should be 254): %d\n", SPI_Read_16(spi, CS_PIN, ADDR_VERSION));
    printf("PART ID (Should be 1048576): %u\n", SPI_Read_32(spi, CS_PIN, ADDR_PARTID) & (1 << 20));
    //printf("Initial VERSION2: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_VERSION2));
    //printf("Initial VERSION: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_VERSION));
    printf("Initial STATUS1: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_STATUS1));

    /* BME280 configs */
    bme280_init_struct(&bme280, i2c, 0x76, &pico_callbacks_blocking);
    bme280_init(&bme280);
    printf("BME280 config done!\n");

    sleep_ms(1000);

    put_pixel(urgb_u32(80, 16, 120)); // purple

    // connect to the specified network
    for (int i = 0; i<3; ++i) {
    printf("\nConnect attempt %d to %s\n", i, WIFI_SSID);
        if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
            printf("failed to connect using {%s, %s}\n", WIFI_SSID, WIFI_PASSWORD);
            if (i == 2) {
                printf("Giving up now\n");
                return -1;
            }
            printf("Retrying...\n");
        } else {
            printf("Connected to %s\n", WIFI_SSID);
            break;
        }
        sleep_ms(1000);
    }


    /* Set up the sensing unit as a TCP server */
    TCP_SERVER_T *state = tcp_server_init(); // Get new server struct
    if (!state) { return -1;}
    if (!tcp_server_open(state, TCP_PORT)) { // Open the server to connections and wait for someone to connect
        printf("\nFailed to open TCP server. Exiting program\n");
        return -1;
    }
    printf("Waiting for a connection...\n\n");
    tcp_accept(state->server_pcb, tcp_server_accept); // tcp_server_accept will assign all callbacks
                                                      // once the connection is made
    
    // Wait before taking measurements
    sleep_ms(1000);

    // Set the voltage swell and dip thresholds
    //SPI_Write_32(spi, CS_PIN, ADDR_DIP_LVL, (uint32_t)DIP_LEVEL/voltage_gain);
    //SPI_Write_32(spi, CS_PIN, ADDR_SWELL_LVL, (uint32_t)SWELL_LEVEL/voltage_gain);
    //SPI_Write_16(spi, CS_PIN, ADDR_DIP_CYC, (uint16_t)DIP_SWELL_CYCLE);
    //SPI_Write_16(spi, CS_PIN, ADDR_SWELL_CYC, (uint16_t)DIP_SWELL_CYCLE);

    //Configure to receive interrupts only when entering and exiting DIP or SWELL modes
    //uint16_t config1 = SPI_Read_16(spi, CS_PIN, ADDR_CONFIG1);
    //config1 = config1 | 0b000001000000000;
    //SPI_Write_16(spi, CS_PIN, ADDR_CONFIG1, config1);

    // Enable overcurrent detection and set threshold
    //SPI_Write_16(spi, CS_PIN, ADDR_CONFIG3, CONFIG3_USER);
    //SPI_Write_32(spi, CS_PIN, ADDR_OILVL, (uint32_t)OVERCURRENT_LEVEL/current_gain);

    // Enable interrupts by changing the MASK1 register and EVENT_MASK register
    // Initially MASK1 and EVENT_MASK are disabled.
    //SPI_Write_32(spi, CS_PIN, ADDR_MASK1, MASK1_USER); 
    //SPI_Write_32(spi, CS_PIN, ADDR_EVENT_MASK, EVENTMASK_USER);

    put_pixel(urgb_u32(0, 0x80, 0)); // green. Can now connect


    // Loop forever
    while (true) {
        if (state->client_pcb) {
            getData();
            printData();
            // Copy contents to send buffer
            memcpy(state->buffer_sent, &data, DATA_SIZE);
            /* printf("Elements of send buffer: ");
            for (int i = 0; i<DATA_SIZE; i++) {
                printf("%hhu ", state->buffer_sent[i]);
            }*/
            printf("Sending data...\n");
            tcp_server_send_data(state, state->client_pcb);
            sleep_ms(DELAY_CONN);
        } else {
        //getData(state);
            getData();
            printData();
            //printWaveformBuffer();
            //printf("STATUS0: %u\n", SPI_Read_32(spi, CS_PIN, ADDR_STATUS0));
            //printf("STATUS1: %u\n", SPI_Read_32(spi, CS_PIN, ADDR_STATUS1));
            //printf("Not yet connected to client oops\n");
            printf("Connect to %s:%u\n\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), TCP_PORT);
            sleep_ms(DELAY_NOCONN);
        }
    } 

    free(state);
    sleep_ms(2000);
    printf("\nExiting main loop\n");
    return 0;
}