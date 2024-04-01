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
#include "ADE9000API_RP2040.h"
#include "lwipopts.h"
#include "bme280.h"
#include "blocking.h"
#include "flash_utils.h"
#include "tcp_server.h"

/* SPI DEFINITIONS */
#define SPI_SPEED 5000000  // 5 MHz
#define SCK_PIN 18
#define MOSI_PIN 19
#define MISO_PIN 16
#define CS_PIN 17

/* I2C DEFINITIONS */
#define SDA_PIN 4
#define SCL_PIN 5
#define I2C_SPEED 100000 // 400 kHz

/* GPIO DEFINITIONS */
#define RESETADE_PIN 9
#define IRQ0_PIN 10
#define IRQ1_PIN 11
#define ZX_PIN 12
#define DREADY_PIN 13
#define PM1_PIN 14
#define BUZZER 15

/* FLASH CONFIGURATION ADDRESSES */
#define ADDR_PERSISTENT getAddressPersistent()

/* WIFI DETAILS */
#define WIFI_SSID "TP-Link_B050"
#define WIFI_PASSWORD "50885275"

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
    float Temperature;
    float Pressure;
    float Humidity;
} sensorData;

#define DATA_SIZE sizeof(sensorData)

// Data structures and sensor objects
ResampledWfbData resampledData; // Resampled Data
ActivePowerRegs powerRegs;     // Declare powerRegs of type ActivePowerRegs to store Active Power Register data
CurrentRMSRegs curntRMSRegs;   //Current RMS
VoltageRMSRegs vltgRMSRegs;    //Voltage RMS
PeriodRegs freqData;            // Frequency of channels
PowerFactorRegs pfData;         // Power factors
VoltageTHDRegs voltageTHDRegsnValues; //Voltage THD
bme280_t bme280; //BME sensor struct
bme280_reading_t r; //BME data struct
sensorData data; // pointer to struct holding all sensor data;

// Ports
spi_inst_t *spi = spi0;
i2c_inst_t *i2c = i2c0;

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

void getData (TCP_SERVER_T *state) {
    /*printf("===================================\n");

    printf("MEASURE COUNT: %d\n", count++);*/
    curr_time = to_ms_since_boot(get_absolute_time());
    /*printf("Current time: %d\n", to_ms_since_boot(get_absolute_time()));*/

    // Read from BME280
    bme280_read(&bme280, &r);
    /*printf(
        "Temperature: %.2f°C\nPressure: %.2f Pa\nHumidity: %.2f%%\n",
        r.temperature, r.pressure, r.humidity
    );

    printf("-----------------------------------\n");*/

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
    /*printf("VA_rms: %f\n", (float)(vltgRMSRegs.VoltageRMSReg_A)*voltage_gain);
    printf("IA_rms: %f\n", (float)(curntRMSRegs.CurrentRMSReg_A)*current_gain);
    printf("freq_A: %f\n", freqData.FrequencyValue_A);        
    printf("Power factor A: %f\n", pfData.PowerFactorValue_A);
    printf("Power in A channel (WATT): %f\n", (float)(powerRegs.ActivePowerReg_A)*power_gain);
    printf("VB_rms: %f\n", (float)(vltgRMSRegs.VoltageRMSReg_B)*voltage_gain);
    printf("IB_rms: %f\n", (float)(curntRMSRegs.CurrentRMSReg_B)*current_gain);
    printf("freq_B: %f\n", freqData.FrequencyValue_B);        
    printf("Power factor B: %f\n", pfData.PowerFactorValue_B);
    printf("Power in B channel (WATT): %f\n", (float)(powerRegs.ActivePowerReg_B)*power_gain);
    printf("VERSION (should be 254): %d\n", SPI_Read_16(spi, CS_PIN, ADDR_VERSION));
    printf("RUN REGISTER: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_RUN));
    printf("ZERO CROSSING THRESHOLD: %d\n", SPI_Read_16(spi, CS_PIN, ADDR_ZXTHRSH));
    printf("===================================\n");*/

    /*if (count == 10) {
        break;
    }*/

    data.time = curr_time;
    data.VArms = voltage_gain*(float)vltgRMSRegs.VoltageRMSReg_A;
    data.VBrms = voltage_gain*(float)vltgRMSRegs.VoltageRMSReg_B;
    data.IArms = current_gain*(float)curntRMSRegs.CurrentRMSReg_A;
    data.IBrms = current_gain*(float)curntRMSRegs.CurrentRMSReg_B;
    data.PowerA = power_gain*(float)powerRegs.ActivePowerReg_A;
    data.PowerB = power_gain*(float)powerRegs.ActivePowerReg_B;
    data.PfA = pfData.PowerFactorValue_A;
    data.PfB = pfData.PowerFactorValue_B;
    data.Temperature = r.temperature;
    data.Pressure = r.pressure;
    data.Humidity = r.humidity;

    printf("Size of structure: %d bytes\n", DATA_SIZE);

    // Copy contents to send buffer
    memcpy(state->buffer_sent, &data, DATA_SIZE);
    //printf("First element of data: %d\n", data.time);
    printf("Elements of send buffer: ");
    for (int i = 0; i<DATA_SIZE; i++) {
        printf("%hhu ", state->buffer_sent[i]);
    }
    printf("\n");
}

int main() {

    // Initialize chosen serial port (USB for now) and WiFi
    stdio_init_all();
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

    sleep_ms(2000);

    // Check if flash reservation works
    printf("\nPersistent Flash (ROM) address at: %d\n\n", ADDR_PERSISTENT);

    // Perform i2c bus scan to see if BME280 is connected
    printf("\nI2C Bus Scan\n");
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
    printf("Done.\n\n");

    absolute_time_t scan_time = nil_time;
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

    // connect to the specified network
    printf("\nAttempting connect to %s\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect using {%s, %s}\n", WIFI_SSID, WIFI_PASSWORD);
        return 1;
    } else {
        printf("Connected to %s\n", WIFI_SSID);
    }

    // Test the TCP connection
    //printf("Starting TCP test\n");
    //run_tcp_server_test();

    /* PERFORM ADE9000 CONFIGURATIONS HERE. LIKE THE INIT ADE9000 FUNCTION IDK */
    resetADE(RESETADE_PIN);
    sleep_ms(1000);
    SetupADE9000(spi, CS_PIN);
    sleep_ms(1000);

    /* BME280 configs */
    bme280_init_struct(&bme280, i2c, 0x76, &pico_callbacks_blocking);
    bme280_init(&bme280);

    /* Set up the sensing unit as a TCP server */
    TCP_SERVER_T *state = tcp_server_init(); // Get new server struct
    if (!state) { return -1;}
    if (!tcp_server_open(state, TCP_PORT)) { // Open the server to connections and wait for someone to connect
        printf("\nFailed to open TCP server. Exiting program\n");
        return -1;
    }
    printf("Waiting for a connection...\n");
    tcp_accept(state->server_pcb, tcp_server_accept); // tcp_server_accept will assign all callbacks
                                                      // once the connection is made
    
    
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


    // Loop forever
    while (true) {
        if (state->client_pcb) {
            getData(state);
            //printf("bruh");
            printf("Sending data...\n");
            tcp_server_send_data(state, state->client_pcb);
        } else {
            getData(state);
            printf("Not yet connected to client oops\n\n");
            //printf("Connect to ip: %u port: %d\n\n", state->server_pcb->local_ip)
        } 
        sleep_ms(400);

    }
    free(state);
    printf("\nExiting main loop\n");
    return 0;
}