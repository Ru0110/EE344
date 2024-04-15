/*
  ADE9000API.h - Library for ADE9000/ADE9078 - Energy and PQ monitoring AFE
  Author:nchandra and shobhit <3 (pls dont sue me AD)
  Date: 3-16-2017
*/
#ifndef ADE9000API_RP2040_h
#define ADE9000API_RP2040_h

/****************************************************************************************************************
 Includes
***************************************************************************************************************/

#include "pico/stdlib.h"
#include  "ADE9000RegMap.h"

/****************************************************************************************************************
 Definitions
****************************************************************************************************************/
/*Configuration registers*/
#define ADE9000_PGA_GAIN 0x0000    	    /*PGA@0x0000. Gain of all channels=1*/
#define ADE9000_CONFIG0 0x00000000		/*Integrator disabled*/
#define ADE9000_CONFIG1	0x0002			/*CF3/ZX pin outputs Zero crossing */
#define ADE9000_CONFIG2	0x0C00			/*Default High pass corner frequency of 1.25Hz*/
#define ADE9000_CONFIG3	0x0000			/*Peak and overcurrent detection disabled*/
#define ADE9000_ACCMODE 0x0000			/*60Hz operation, 3P4W Wye configuration, signed accumulation*/
										/*Clear bit 8 i.e. ACCMODE=0x00xx for 50Hz operation*/
										/*ACCMODE=0x0x9x for 3Wire delta when phase B is used as reference*/	
#define ADE9000_TEMP_CFG 0x000C			/*Temperature sensor enabled*/
#define ADE9000_ZX_LP_SEL 0x001E		/*Line period and zero crossing obtained from combined signals VA,VB and VC*/	
#define ADE9000_MASK0 0x00000001		/*Enable EGYRDY interrupt*/				
#define ADE9000_MASK1 0x00000000		/*MASK1 interrupts disabled*/
#define ADE9000_EVENT_MASK 0x00000000	/*Events disabled */
#define ADE9000_VLEVEL	0x0022EA28		/*Assuming Vnom=1/2 of full scale. 
										/*Refer Technical reference manual for detailed calculations.*/
#define ADE9000_DICOEFF 0x00000000 		/* Set DICOEFF= 0xFFFFE000 when integrator is enabled*/

/*Constant Definitions***/
#define ADE90xx_FDSP 8000   			/*ADE9000 FDSP: 8000sps, ADE9078 FDSP: 4000sps*/
#define ADE9000_RUN_ON 0x0001			/*DSP ON*/
/*Energy Accumulation Settings*/
#define ADE9000_EP_CFG 0x0011			/*Enable energy accumulation, accumulate samples at 8ksps*/
										/*latch energy accumulation after EGYRDY*/
										/*If accumulation is changed to half line cycle mode, change EGY_TIME*/
#define ADE9000_EGY_TIME 0x1F3F 				/*Accumulate 8000 samples*/

/*Waveform buffer Settings*/
#define ADE9000_WFB_CFG 0x1000			/*Neutral current samples enabled, Resampled data enabled*/
										/*Burst all channels*/
#define WFB_ELEMENT_ARRAY_SIZE 256  	/*size of buffer to read. 512 Max.Each element IA,VA...IN has max 512 points 
										/*[Size of waveform buffer/number of sample sets = 2048/4 = 512]*/
										/*(Refer ADE9000 technical reference manual for more details)*/

/*Full scale Codes referred from Datasheet.Respective digital codes are produced when ADC inputs are at full scale. Donot Change. */
#define ADE9000_RMS_FULL_SCALE_CODES  52702092
#define ADE9000_WATT_FULL_SCALE_CODES 20694066
#define ADE9000_RESAMPLED_FULL_SCALE_CODES  18196
#define ADE9000_PCF_FULL_SCALE_CODES  74532013

/*Size of array reading calibration constants from EEPROM*/
#define CALIBRATION_CONSTANTS_ARRAY_SIZE 13
#define ADE9000_EEPROM_ADDRESS 0x54			//1010xxxy xxx---> 100(A2,A1,A0 defined by hardware). y (1: Read, 0:Write)
#define EEPROM_WRITTEN 0x01

/*Address of registers stored in EEPROM.Calibration data is 4 bytes*/
#define ADDR_CHECKSUM_EEPROM 0x0800		 // Simple checksum to verify data transmission errors. Add all the registers upto CPGAIN. The lower 32 bits should match data starting @ADDR_CHECKSUM_EEPROM
#define ADDR_EEPROM_WRITTEN_BYTE 0x0804  //1--> EEPROM Written, 0--> Not written. One byte only

#define ADDR_AIGAIN_EEPROM 0x0004
#define ADDR_BIGAIN_EEPROM 0x000C
#define ADDR_CIGAIN_EEPROM 0x0010
#define ADDR_NIGAIN_EEPROM 0x0014
#define ADDR_AVGAIN_EEPROM 0x0018
#define ADDR_BVGAIN_EEPROM 0x001C
#define ADDR_CVGAIN_EEPROM 0x0020
#define ADDR_APHCAL0_EEPROM 0x0024
#define ADDR_BPHCAL0_EEPROM 0x0028
#define ADDR_CPHCAL0_EEPROM 0x002C
#define ADDR_APGAIN_EEPROM 0x0030
#define ADDR_BPGAIN_EEPROM 0x0034
#define ADDR_CPGAIN_EEPROM 0x0038

/* GAIN CONSTANTS CALCULATED FOR EVAL BOARD */
#define voltage_gain 566.307/(double)ADE9000_RMS_FULL_SCALE_CODES // 0.707*801/full_scale_code
#define current_gain 44.1875/(double)ADE9000_RMS_FULL_SCALE_CODES  // 0.707*2500/(2*20*full_scale_code)
#define power_gain 20018.95/(double)ADE9000_WATT_FULL_SCALE_CODES  // 78327/full_scale_code
#define resampling_gain 801.0/(double)ADE9000_RESAMPLED_FULL_SCALE_CODES // 801/full_scale_code

/****************************************************************************************************************
 EEPROM Global Variables
****************************************************************************************************************/

extern uint32_t calibrationDatafromEEPROM[CALIBRATION_CONSTANTS_ARRAY_SIZE];
extern uint32_t ADE9000_CalibrationRegAddress[CALIBRATION_CONSTANTS_ARRAY_SIZE];
extern uint32_t ADE9000_Eeprom_CalibrationRegAddress[CALIBRATION_CONSTANTS_ARRAY_SIZE];

/****************************************************************************************************************
 Structures and Global Variables
****************************************************************************************************************/

typedef struct ResampledWfbData {
    int16_t VA_Resampled[512];
    int16_t IA_Resampled[512];
    int16_t VB_Resampled[512];
    int16_t IB_Resampled[512];
    int16_t VC_Resampled[512];
    int16_t IC_Resampled[512];
    int16_t IN_Resampled[512];
} ResampledWfbData;

// Does not read IN btw
typedef struct WaveformData { 
    int32_t IA_waveform[128];
    int32_t VA_waveform[128];
    int32_t IB_waveform[128];
    int32_t VB_waveform[128];
    int32_t IC_waveform[128];
    int32_t VC_waveform[128];
} WaveformData;

typedef struct ActivePowerRegs {
    int32_t ActivePowerReg_A;
    int32_t ActivePowerReg_B;
    int32_t ActivePowerReg_C;
} ActivePowerRegs;

typedef struct ReactivePowerRegs {
    int32_t ReactivePowerReg_A;
    int32_t ReactivePowerReg_B;
    int32_t ReactivePowerReg_C;
} ReactivePowerRegs;

typedef struct ApparentPowerRegs {
    int32_t ApparentPowerReg_A;
    int32_t ApparentPowerReg_B;
    int32_t ApparentPowerReg_C;
} ApparentPowerRegs;

typedef struct VoltageRMSRegs {
    int32_t VoltageRMSReg_A;
    int32_t VoltageRMSReg_B;
    int32_t VoltageRMSReg_C;
} VoltageRMSRegs;

typedef struct CurrentRMSRegs {
    int32_t CurrentRMSReg_A;
    int32_t CurrentRMSReg_B;
    int32_t CurrentRMSReg_C;
    int32_t CurrentRMSReg_N;
} CurrentRMSRegs;

typedef struct FundActivePowerRegs {
    int32_t FundActivePowerReg_A;
    int32_t FundActivePowerReg_B;
    int32_t FundActivePowerReg_C;
} FundActivePowerRegs; 

typedef struct FundReactivePowerRegs {
    int32_t FundReactivePowerReg_A;
    int32_t FundReactivePowerReg_B;
    int32_t FundReactivePowerReg_C;
} FundReactivePowerRegs; 

typedef struct FundApparentPowerRegs {
    int32_t FundApparentPowerReg_A;
    int32_t FundApparentPowerReg_B;
    int32_t FundApparentPowerReg_C;
} FundApparentPowerRegs; 

typedef struct FundVoltageRMSRegs {
    int32_t FundVoltageRMSReg_A;
    int32_t FundVoltageRMSReg_B;
    int32_t FundVoltageRMSReg_C;
} FundVoltageRMSRegs; 

typedef struct FundCurrentRMSRegs {
    int32_t FundCurrentRMSReg_A;
    int32_t FundCurrentRMSReg_B;
    int32_t FundCurrentRMSReg_C;
//Fundamental neutral RMS is not calculated 
} FundCurrentRMSRegs; 

typedef struct HalfVoltageRMSRegs {
    int32_t HalfVoltageRMSReg_A;
    int32_t HalfVoltageRMSReg_B;
    int32_t HalfVoltageRMSReg_C;
} HalfVoltageRMSRegs; 

typedef struct HalfCurrentRMSRegs {
    int32_t HalfCurrentRMSReg_A;
    int32_t HalfCurrentRMSReg_B;
    int32_t HalfCurrentRMSReg_C;
    int32_t HalfCurrentRMSReg_N;	
} HalfCurrentRMSRegs; 

typedef	struct Ten12VoltageRMSRegs {
    int32_t Ten12VoltageRMSReg_A;
    int32_t Ten12VoltageRMSReg_B;
    int32_t Ten12VoltageRMSReg_C;	
} Ten12VoltageRMSRegs; 

typedef	struct Ten12CurrentRMSRegs {
    int32_t Ten12CurrentRMSReg_A;
    int32_t Ten12CurrentRMSReg_B;
    int32_t Ten12CurrentRMSReg_C;
    int32_t Ten12CurrentRMSReg_N;	
} Ten12CurrentRMSRegs; 

typedef	struct VoltageTHDRegs {
    int32_t VoltageTHDReg_A;
    int32_t VoltageTHDReg_B;
    int32_t VoltageTHDReg_C;
    float VoltageTHDValue_A;
    float VoltageTHDValue_B;
    float VoltageTHDValue_C;
} VoltageTHDRegs; 

typedef struct CurrentTHDRegs {
    int32_t CurrentTHDReg_A;
    int32_t CurrentTHDReg_B;
    int32_t CurrentTHDReg_C;
    float CurrentTHDValue_A;
    float CurrentTHDValue_B;
    float CurrentTHDValue_C;	
} CurrentTHDRegs; 

typedef	struct PowerFactorRegs {
    int32_t PowerFactorReg_A;
    int32_t PowerFactorReg_B;
    int32_t PowerFactorReg_C;	
    float PowerFactorValue_A;
    float PowerFactorValue_B;
    float PowerFactorValue_C;
} PowerFactorRegs;  

typedef	struct PeriodRegs {
    int32_t PeriodReg_A;
    int32_t PeriodReg_B;
    int32_t PeriodReg_C;
    float FrequencyValue_A;
    float FrequencyValue_B;
    float FrequencyValue_C;	
} PeriodRegs;   

typedef	struct AngleRegs {
int16_t AngleReg_VA_VB;
int16_t AngleReg_VB_VC;
int16_t AngleReg_VA_VC;
int16_t AngleReg_VA_IA;
int16_t AngleReg_VB_IB;
int16_t AngleReg_VC_IC;
int16_t AngleReg_IA_IB;
int16_t AngleReg_IB_IC;
int16_t AngleReg_IA_IC;
float AngleValue_VA_VB;
float AngleValue_VB_VC;
float AngleValue_VA_VC;
float AngleValue_VA_IA;
float AngleValue_VB_IB;
float AngleValue_VC_IC;
float AngleValue_IA_IB;
float AngleValue_IB_IC;
float AngleValue_IA_IC;	
} AngleRegs;   

typedef	struct TemperatureRegnValue {
    int16_t Temperature_Reg;
    float Temperature;	
} TemperatureRegnValue;   

/*--------------------------------------------NEW CODE BEGINS HERE----------------------------*/

void resetADE(const uint reset_pin);
void SetupADE9000(spi_inst_t *spi, const uint cs_pin); 

/* SPI FUNCTION DECLARATIONS */

//void SPI_Init(uint32_t SPI_speed , uint8_t chipSelect_Pin); // idk if this is needed 	

void SPI_Write_16(spi_inst_t *spi,
                const uint cs_pin, 
                const uint16_t reg, 
                const uint16_t data);

void SPI_Write_32(spi_inst_t *spi,
                const uint cs_pin, 
                const uint16_t reg, 
                const uint32_t data);		

uint16_t SPI_Read_16(spi_inst_t *spi,
                const uint cs_pin, 
                const uint16_t reg);

uint32_t SPI_Read_32(spi_inst_t *spi,
                const uint cs_pin, 
                const uint16_t reg);

void SPI_Burst_Read_Resampled_Wfb(spi_inst_t *spi,
                                const uint cs_pin,
                                uint16_t Address, 
                                uint16_t Read_Element_Length, 
                                ResampledWfbData *ResampledData);

void SPI_Burst_Read_Fixed_Rate(spi_inst_t *spi,
                                const uint cs_pin,
                                uint16_t Address, 
                                uint16_t Read_Element_Length, 
                                WaveformData *Data);

/*ADE9000 Calculated Parameter Read Functions*/

void ReadActivePowerRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        ActivePowerRegs *Data);

void ReadReactivePowerRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        ReactivePowerRegs *Data);

void ReadApparentPowerRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        ApparentPowerRegs *Data);

void ReadVoltageRMSRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        VoltageRMSRegs *Data);

void ReadCurrentRMSRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        CurrentRMSRegs *Data);

void ReadFundActivePowerRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        FundActivePowerRegs *Data);

void ReadFundReactivePowerRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        FundReactivePowerRegs *Data);

void ReadFundApparentPowerRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        FundApparentPowerRegs *Data);

void ReadFundVoltageRMSRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        FundVoltageRMSRegs *Data);

void ReadFundCurrentRMSRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        FundCurrentRMSRegs *Data);

void ReadHalfVoltageRMSRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        HalfVoltageRMSRegs *Data);

void ReadHalfCurrentRMSRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        HalfCurrentRMSRegs *Data);

void ReadTen12VoltageRMSRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        Ten12VoltageRMSRegs *Data);

void ReadTen12CurrentRMSRegs(spi_inst_t *spi,
                        const uint cs_pin, 
                        Ten12CurrentRMSRegs *Data);

void ReadVoltageTHDRegsnValues(spi_inst_t *spi,
                        const uint cs_pin, 
                        VoltageTHDRegs *Data);

void ReadCurrentTHDRegsnValues(spi_inst_t *spi,
                        const uint cs_pin, 
                        CurrentTHDRegs *Data);

void ReadPowerFactorRegsnValues(spi_inst_t *spi,
                        const uint cs_pin, 
                        PowerFactorRegs *Data);

void ReadPeriodRegsnValues(spi_inst_t *spi,
                        const uint cs_pin, 
                        PeriodRegs *Data);

void ReadAngleRegsnValues(spi_inst_t *spi,
                        const uint cs_pin, 
                        AngleRegs *Data);		

void ReadTempRegnValue(spi_inst_t *spi,
                        const uint cs_pin,
                        TemperatureRegnValue *Data);

void SoftwareReset(spi_inst_t *spi,
                        const uint cs_pin);



#endif