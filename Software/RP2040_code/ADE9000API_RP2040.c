#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "ADE9000API_RP2040.h"

// Reset cycle for the ADE9000
void resetADE(const uint reset_pin) {
    gpio_put(reset_pin, 0);
    sleep_ms(50);
    gpio_put(reset_pin, 1);
    sleep_ms(1000);
}

// Write configuration data into the config registers to start up the ADE9000
void SetupADE9000(spi_inst_t *spi) {

    SPI_Write_16(spi, ADDR_PGA_GAIN,ADE9000_PGA_GAIN);     
    SPI_Write_32(spi, ADDR_CONFIG0,ADE9000_CONFIG0); 
    SPI_Write_16(spi, ADDR_CONFIG1,ADE9000_CONFIG1);
    SPI_Write_16(spi, ADDR_CONFIG2,ADE9000_CONFIG2);
    SPI_Write_16(spi, ADDR_CONFIG3,ADE9000_CONFIG3);
    SPI_Write_16(spi, ADDR_ACCMODE,ADE9000_ACCMODE);
    SPI_Write_16(spi, ADDR_TEMP_CFG,ADE9000_TEMP_CFG);
    SPI_Write_16(spi, ADDR_ZX_LP_SEL,ADE9000_ZX_LP_SEL);
    SPI_Write_32(spi, ADDR_MASK0,ADE9000_MASK0);
    SPI_Write_32(spi, ADDR_MASK1,ADE9000_MASK1);
    SPI_Write_32(spi, ADDR_EVENT_MASK,ADE9000_EVENT_MASK);
    SPI_Write_16(spi, ADDR_WFB_CFG,ADE9000_WFB_CFG);
    SPI_Write_32(spi, ADDR_VLEVEL,ADE9000_VLEVEL);
    SPI_Write_32(spi, ADDR_DICOEFF,ADE9000_DICOEFF);
    SPI_Write_16(spi, ADDR_EGY_TIME,ADE9000_EGY_TIME);
    SPI_Write_16(spi, ADDR_EP_CFG,ADE9000_EP_CFG);		//Energy accumulation ON
    SPI_Write_16(spi, ADDR_RUN,ADE9000_RUN_ON);		//DSP ON
}

// Write 16 bits to specified register
void SPI_Write_16(spi_inst_t *spi,
                const uint16_t reg, 
                const uint16_t data) {
    
    uint16_t msg[2];
    msg[0] = ((reg << 4) & 0xFFF0); // form message (set ~W and MB to 0)
    msg[1] = data;

    // Write to register
    spi_write16_blocking(spi, msg, 2);
}

// Write 32 bits to specified register
void SPI_Write_32(spi_inst_t *spi,
                const uint16_t reg, 
                const uint32_t data) {
    
    uint16_t msg[3];
    msg[0] = ((reg << 4) & 0xFFF0); // form message (set ~W and MB to 0)
    msg[1] = (data & 0xFFFF0000)>>16;
    msg[2] = (data & 0x0000FFFF);

    // Write to register
    spi_write16_blocking(spi, msg, 3);
}

// Read 16 bits from a register
uint16_t SPI_Read_16(spi_inst_t *spi,
                const uint16_t reg) {
    
    uint16_t temp_address[1];
    temp_address[0] = (((reg << 4) & 0XFFF0) + 8);
    uint16_t temp_packet;

    // Read from register
    spi_write16_blocking(spi, temp_address, 1);
    spi_read16_blocking(spi, 0, &temp_packet, 1);

    return temp_packet;
}

// Read 32 bits from a register
uint32_t SPI_Read_32(spi_inst_t *spi,
                const uint16_t reg) {
    
    uint16_t temp_address[1];
    temp_address[0] = (((reg << 4) & 0XFFF0) + 8);
    uint16_t temp_highpacket;
    uint16_t temp_lowpacket;
    uint32_t temp_packet;

    // Read from register
    spi_write16_blocking(spi, temp_address, 1);
    spi_read16_blocking(spi, 0, &temp_highpacket, 1);
    spi_read16_blocking(spi, 0, &temp_lowpacket, 1);

    temp_packet = temp_highpacket << 16 + temp_lowpacket;
    return temp_packet;
}


/* 
Description: Burst reads the content of waveform buffer. This function only works with resampled data. Configure waveform buffer to have Resampled data, and burst enabled (BURST_CHAN=0000 in WFB_CFG Register).
Input: The starting address. Use the starting address of a data set. e.g 0x800, 0x804 etc to avoid data going into incorrect arrays. 
       Read_Element_Length is the number of data sets to read. If the starting address is 0x800, the maximum sets to read are 512.
Output: Resampled data returned in structure
*/
void SPI_Burst_Read_Resampled_Wfb(spi_inst_t *spi,
                                uint16_t Address, 
                                uint16_t Read_Element_Length, 
                                ResampledWfbData *ResampledData) {
    
    uint16_t i;
    uint16_t reg[1];
    reg[0] = ((Address << 4) & 0xFFF0)+8;

    spi_write16_blocking(spi, reg, 1); // Send address to read from
    //burst read the data upto Read_Length 
	for(i=0;i<Read_Element_Length;i++) {
        spi_read16_blocking(spi, 0, &ResampledData->IA_Resampled[i], 1);
        spi_read16_blocking(spi, 0, &ResampledData->VA_Resampled[i], 1);
        spi_read16_blocking(spi, 0, &ResampledData->IB_Resampled[i], 1);
        spi_read16_blocking(spi, 0, &ResampledData->VB_Resampled[i], 1);
        spi_read16_blocking(spi, 0, &ResampledData->IC_Resampled[i], 1);
        spi_read16_blocking(spi, 0, &ResampledData->VC_Resampled[i], 1);
        spi_read16_blocking(spi, 0, &ResampledData->IN_Resampled[i], 1);
	}
}

void ReadActivePowerRegs(spi_inst_t *spi,
                        ActivePowerRegs *Data) {

    Data->ActivePowerReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AWATT));
	Data->ActivePowerReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BWATT));
	Data->ActivePowerReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CWATT));
}

void ReadReactivePowerRegs(spi_inst_t *spi,
                        ReactivePowerRegs *Data) {

    Data->ReactivePowerReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AVAR));
	Data->ReactivePowerReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BVAR));
	Data->ReactivePowerReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CVAR));
}

void ReadApparentPowerRegs(spi_inst_t *spi,
                        ApparentPowerRegs *Data) {

    Data->ApparentPowerReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AVA));
	Data->ApparentPowerReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BVA));
	Data->ApparentPowerReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CVA));	
}

void ReadVoltageRMSRegs(spi_inst_t *spi,
                        VoltageRMSRegs *Data) {

	Data->VoltageRMSReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AVRMS));
	Data->VoltageRMSReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BVRMS));
	Data->VoltageRMSReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CVRMS));	
}

void ReadCurrentRMSRegs(spi_inst_t *spi,
                        CurrentRMSRegs *Data) {

	Data->CurrentRMSReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AIRMS));
	Data->CurrentRMSReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BIRMS));
	Data->CurrentRMSReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CIRMS));
	Data->CurrentRMSReg_N = (int32_t) (SPI_Read_32(spi, ADDR_NIRMS));
}

void ReadFundActivePowerRegs(spi_inst_t *spi,
                        FundActivePowerRegs *Data) {

	Data->FundActivePowerReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AFWATT));
	Data->FundActivePowerReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BFWATT));
	Data->FundActivePowerReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CFWATT));
}

void ReadFundReactivePowerRegs(spi_inst_t *spi,
                        FundReactivePowerRegs *Data) {

	Data->FundReactivePowerReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AFVAR));
	Data->FundReactivePowerReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BFVAR));
	Data->FundReactivePowerReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CFVAR));
}

void ReadFundApparentPowerRegs(spi_inst_t *spi,
                        FundApparentPowerRegs *Data) {

	Data->FundApparentPowerReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AFVA));
	Data->FundApparentPowerReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BFVA));
	Data->FundApparentPowerReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CFVA));
}

void ReadFundVoltageRMSRegs(spi_inst_t *spi,
                        FundVoltageRMSRegs *Data) {

	Data->FundVoltageRMSReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AVFRMS));
	Data->FundVoltageRMSReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BVFRMS));
	Data->FundVoltageRMSReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CVFRMS));	
}

void ReadFundCurrentRMSRegs(spi_inst_t *spi,
                        FundCurrentRMSRegs *Data) {

    Data->FundCurrentRMSReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AIFRMS));
	Data->FundCurrentRMSReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BIFRMS));
	Data->FundCurrentRMSReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CIFRMS));	
}

void ReadHalfVoltageRMSRegs(spi_inst_t *spi,
                        HalfVoltageRMSRegs *Data) {

	Data->HalfVoltageRMSReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AVRMSONE));
	Data->HalfVoltageRMSReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BVRMSONE));
	Data->HalfVoltageRMSReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CVRMSONE));
}       

void ReadHalfCurrentRMSRegs(spi_inst_t *spi,
                        HalfCurrentRMSRegs *Data) {

	Data->HalfCurrentRMSReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AIRMSONE));
	Data->HalfCurrentRMSReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BIRMSONE));
	Data->HalfCurrentRMSReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CIRMSONE));
	Data->HalfCurrentRMSReg_N = (int32_t) (SPI_Read_32(spi, ADDR_NIRMSONE));
}   

void ReadTen12VoltageRMSRegs(spi_inst_t *spi,
                        Ten12VoltageRMSRegs *Data) {

	Data->Ten12VoltageRMSReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AVRMS1012));
	Data->Ten12VoltageRMSReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BVRMS1012));
	Data->Ten12VoltageRMSReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CVRMS1012));	                            
}       

void ReadTen12CurrentRMSRegs(spi_inst_t *spi,
                        Ten12CurrentRMSRegs *Data) {

	Data->Ten12CurrentRMSReg_A = (int32_t) (SPI_Read_32(spi, ADDR_AIRMS1012));
	Data->Ten12CurrentRMSReg_B = (int32_t) (SPI_Read_32(spi, ADDR_BIRMS1012));
	Data->Ten12CurrentRMSReg_C = (int32_t) (SPI_Read_32(spi, ADDR_CIRMS1012));
	Data->Ten12CurrentRMSReg_N = (int32_t) (SPI_Read_32(spi, ADDR_NIRMS1012));	                            
}   

void ReadVoltageTHDRegsnValues(spi_inst_t *spi,
                        VoltageTHDRegs *Data) {

	uint32_t tempReg;
	float tempValue;
	
	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_AVTHD)); //Read THD register
	Data->VoltageTHDReg_A = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %
	Data->VoltageTHDValue_A=tempValue;	

	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_BVTHD)); //Read THD register
	Data->VoltageTHDReg_B = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %
	Data->VoltageTHDValue_B=tempValue;	

	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_CVTHD)); //Read THD register
	Data->VoltageTHDReg_C = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %
	Data->VoltageTHDValue_C=tempValue;	                            
}

void ReadCurrentTHDRegsnValues(spi_inst_t *spi,
                        CurrentTHDRegs *Data) {

    uint32_t tempReg;
	float tempValue;	
	
	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_AITHD)); //Read THD register
	Data->CurrentTHDReg_A = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %	
	Data->CurrentTHDValue_A=tempValue;		

	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_BITHD)); //Read THD register
	Data->CurrentTHDReg_B = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %	
	Data->CurrentTHDValue_B=tempValue;

	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_CITHD)); //Read THD register
	Data->CurrentTHDReg_C = tempReg;
	tempValue=(float)tempReg*100/(float)134217728; //Calculate THD in %		
	Data->CurrentTHDValue_C=tempValue;                            
}       

void ReadPowerFactorRegsnValues(spi_inst_t *spi,
                        PowerFactorRegs *Data) {

    uint32_t tempReg;
	float tempValue;	
	
	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_APF)); //Read PF register
	Data->PowerFactorReg_A = tempReg;
	tempValue=(float)tempReg/(float)134217728; //Calculate PF	
	Data->PowerFactorValue_A=tempValue;

	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_BPF)); //Read PF register
	Data->PowerFactorReg_B = tempReg;
	tempValue=(float)tempReg/(float)134217728; //Calculate PF	
	Data->PowerFactorValue_B=tempValue;	

	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_CPF)); //Read PF register
	Data->PowerFactorReg_C = tempReg;
	tempValue=(float)tempReg/(float)134217728; //Calculate PF	
	Data->PowerFactorValue_C=tempValue;                            
}       

void ReadPeriodRegsnValues(spi_inst_t *spi,
                        PeriodRegs *Data) {

	uint32_t tempReg;
	float tempValue;

	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_APERIOD)); //Read PERIOD register
	Data->PeriodReg_A = tempReg;
	tempValue=(float)(8000*65536)/(float)(tempReg+1); //Calculate Frequency	
	Data->FrequencyValue_A = tempValue;

	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_BPERIOD)); //Read PERIOD register
	Data->PeriodReg_B = tempReg;
	tempValue=(float)(8000*65536)/(float)(tempReg+1); //Calculate Frequency	
	Data->FrequencyValue_B = tempValue;

	tempReg=(int32_t) (SPI_Read_32(spi, ADDR_CPERIOD)); //Read PERIOD register
	Data->PeriodReg_C = tempReg;
	tempValue=(float)(8000*65536)/(float)(tempReg+1); //Calculate Frequency	
	Data->FrequencyValue_C = tempValue;                            
}

void ReadAngleRegsnValues(spi_inst_t *spi,
                        AngleRegs *Data) {

    uint32_t tempReg;	
	uint16_t temp;
	float mulConstant;
	float tempValue;
	
	temp=SPI_Read_16(spi, ADDR_ACCMODE); //Read frequency setting register
	if((temp&0x0100)>=0)
		{
			mulConstant=0.02109375;  //multiplier constant for 60Hz system
		}
	else
		{
			mulConstant=0.017578125; //multiplier constant for 50Hz system		
		}
	
	tempReg=(int16_t) (SPI_Read_32(spi, ADDR_ANGL_VA_VB)); //Read ANGLE register
	Data->AngleReg_VA_VB=tempReg;
	tempValue=tempReg*mulConstant;	//Calculate Angle in degrees					
	Data->AngleValue_VA_VB=tempValue;

	tempReg=(int16_t) (SPI_Read_32(spi, ADDR_ANGL_VB_VC));
	Data->AngleReg_VB_VC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VB_VC=tempValue;	

	tempReg=(int16_t) (SPI_Read_32(spi, ADDR_ANGL_VA_VC));
	Data->AngleReg_VA_VC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VA_VC=tempValue;	

	tempReg=(int16_t) (SPI_Read_32(spi, ADDR_ANGL_VA_IA));
	Data->AngleReg_VA_IA=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VA_IA=tempValue;

	tempReg=(int16_t) (SPI_Read_32(spi, ADDR_ANGL_VB_IB));
	Data->AngleReg_VB_IB=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VB_IB=tempValue;	

	tempReg=(int16_t) (SPI_Read_32(spi, ADDR_ANGL_VC_IC));
	Data->AngleReg_VC_IC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_VC_IC=tempValue;		

	tempReg=(int16_t) (SPI_Read_32(spi, ADDR_ANGL_IA_IB));
	Data->AngleReg_IA_IB=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_IA_IB=tempValue;

	tempReg=(int16_t) (SPI_Read_32(spi, ADDR_ANGL_IB_IC));
	Data->AngleReg_IB_IC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_IB_IC=tempValue;	

	tempReg=(int16_t) (SPI_Read_32(spi, ADDR_ANGL_IA_IC));
	Data->AngleReg_IA_IC=tempReg;
	tempValue=tempReg*mulConstant;
	Data->AngleValue_IA_IC=tempValue;				                            
}	

void ReadTempRegnValue(spi_inst_t *spi,
                        TemperatureRegnValue *Data) {

    uint32_t trim;
	uint16_t gain;
	uint16_t offset;
	uint16_t tempReg; 
	float tempValue;
	
	SPI_Write_16(spi, ADDR_TEMP_CFG, ADE9000_TEMP_CFG);//Start temperature acquisition cycle with settings in defined in ADE9000_TEMP_CFG
	sleep_ms(2); //delay of 2ms. Increase delay if TEMP_TIME is changed

	trim = SPI_Read_32(spi, ADDR_TEMP_TRIM);
	gain= (trim & 0xFFFF);  //Extract 16 LSB
	offset= ((trim>>16)&0xFFFF); //Extract 16 MSB
	tempReg= SPI_Read_16(spi, ADDR_TEMP_RSLT);	//Read Temperature result register
	tempValue= (float)(offset>>5)-((float)tempReg*(float)gain/(float)65536); 
	
	Data->Temperature_Reg=tempReg;
	Data->Temperature=tempValue;                            
}       

