
#include <SPI.h>
#include  "ADE9000RegMap.h"
#include "ADE9000API.h"

/*Basic initializations*/
ADE9000Class ade9000;
#define SPI_SPEED 5000000     //SPI Speed
#define CS_PIN 17 //8-->Arduino Zero. 16-->ESP8266 
#define ADE9000_RESET_PIN 20 //Reset Pin on HW
#define PM_1 14              //PM1 Pin: 4-->Arduino Zero. 15-->ESP8266 

/*Structure decleration */
struct ActivePowerRegs powerRegs;     // Declare powerRegs of type ActivePowerRegs to store Active Power Register data
struct CurrentRMSRegs curntRMSRegs;   //Current RMS
struct VoltageRMSRegs vltgRMSRegs;    //Voltage RMS
struct VoltageTHDRegs voltageTHDRegsnValues; //Voltage THD
struct ResampledWfbData resampledData; // Resampled Data
struct PeriodRegs freqData;
struct PowerFactorRegs pfData;

/*Function Decleration*/
void readRegisterData(void);
void readResampledData(void);
void resetADE9000(void);

void setup() 
{
  Serial.begin(115200);
  pinMode(PM_1, OUTPUT);    //Set PM1 pin as output 
  digitalWrite(PM_1, LOW);   //Set PM1 select pin low for PSM0 mode
  pinMode(ADE9000_RESET_PIN, OUTPUT);
  digitalWrite(ADE9000_RESET_PIN, HIGH); 
  void resetADE9000(); 
  delay(1000);
  ade9000.SPI_Init(SPI_SPEED,CS_PIN); //Initialize SPI
  ade9000.SetupADE9000();             //Initialize ADE9000 registers according to values in ADE9000API.h
  //ade9000.SPI_Write_16(ADDR_RUN,0x1); //Set RUN=1 to turn on DSP. Uncomment if SetupADE9000 function is not used
  Serial.print("RUN Register: ");
  Serial.println(ade9000.SPI_Read_16(ADDR_RUN),HEX);
}

void loop() {
  delay(2000);
  readRegisterData();
  readResampledData();
  
}

void readRegisterData()
{
 /*Read and Print Specific Register using ADE9000 SPI Library */
  //Serial.print("AIRMS: "); 
  //Serial.println(ade9000.SPI_Read_32(ADDR_AIRMS),HEX); // AIRMS

 /*Read and Print RMS & WATT Register using ADE9000 Read Library*/
  ade9000.ReadVoltageRMSRegs(&vltgRMSRegs);    //Template to read Power registers from ADE9000 and store data in Arduino MCU
  ade9000.ReadActivePowerRegs(&powerRegs);
  ade9000.ReadPeriodRegsnValues(&freqData);
  ade9000.ReadCurrentRMSRegs(&curntRMSRegs);
  ade9000.ReadPowerFactorRegsnValues(&pfData);
  Serial.print("AVRMS: ");        
  //Serial.println(float(vltgRMSRegs.VoltageRMSReg_A*801)*0.707/float(ADE9000_RMS_FULL_SCALE_CODES)); //Print AVRMS register
  Serial.println(double(vltgRMSRegs.VoltageRMSReg_A)*801.0*0.707/double(ADE9000_RMS_FULL_SCALE_CODES));
  Serial.print("AIRMS: ");
  Serial.println(double(curntRMSRegs.CurrentRMSReg_A*2000)*0.707/(2.0*5.1*double(ADE9000_RMS_FULL_SCALE_CODES)));
  Serial.print("freq_A: ");        
  Serial.println(freqData.FrequencyValue_A); //Print AVRMS register
  Serial.print("Power factor A: ");
  Serial.println(pfData.PowerFactorValue_A);
  Serial.print("AWATT:");        
  Serial.println((double(powerRegs.ActivePowerReg_A)*78327.0)/double(ADE9000_WATT_FULL_SCALE_CODES)); //Print AWATT register. How did 78327.0 come ?
  Serial.println("");
}

void readResampledData()
{
  uint32_t temp;
/*Read and Print Resampled data*/
  /*Start the Resampling engine to acquire 4 cycles of resampled data*/
  ade9000.SPI_Write_16(ADDR_WFB_CFG,0x1000);
  ade9000.SPI_Write_16(ADDR_WFB_CFG,0x1010);
  delay(100); //approximate time to fill the waveform buffer with 4 line cycles  
  /*Read Resampled data into Arduino Memory*/
  ade9000.SPI_Burst_Read_Resampled_Wfb(0x800,WFB_ELEMENT_ARRAY_SIZE,&resampledData);   // Burst read function
  
  for(temp=0;temp<WFB_ELEMENT_ARRAY_SIZE;temp++)
    {
      //Serial.print("VA: ");
      //Serial.println((float)resampledData.VA_Resampled[temp]*801.0/(float)ADE9000_RESAMPLED_FULL_SCALE_CODES);
      //Serial.print("IA: ");
      //Serial.println((float)resampledData.IA_Resampled[temp]*2000/((float)ADE9000_RESAMPLED_FULL_SCALE_CODES)*5.1);
      /*Serial.print("VB: ");
      Serial.println((float)resampledData.VB_Resampled[temp]*801.0/(float)ADE9000_RESAMPLED_FULL_SCALE_CODES);
      Serial.print("IB: ");
      Serial.println((float)resampledData.IB_Resampled[temp]*801.0/(float)ADE9000_RESAMPLED_FULL_SCALE_CODES);*/
   } 
    
   Serial.print("Device ID: ");
   Serial.println(ade9000.SPI_Read_16(ADDR_VERSION));
   Serial.print("RUN Register: ");
  Serial.println(ade9000.SPI_Read_16(ADDR_RUN));
  Serial.print("Zero crossing threshold: ");
  Serial.println(ade9000.SPI_Read_16(ADDR_ZXTHRSH));
  Serial.println();

}

void resetADE9000(void)
{
 digitalWrite(ADE9000_RESET_PIN, LOW);
 delay(50);
 digitalWrite(ADE9000_RESET_PIN, HIGH);
 delay(1000);
 Serial.println("Reset Done");
}
