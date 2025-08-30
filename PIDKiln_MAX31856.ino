#if (TC_BOARD_TYPE_KILN == TC_BOARD_MAX31856) || (TC_BOARD_TYPE_HOUSING == TC_BOARD_MAX31856)
#include <Adafruit_MAX31856.h>
extern SPIClass ESP32_SPI;

// MAX31855 temperature readout
template <class TC>
void Read_Temperature_MAX31856(TC& Thermocouple, double& tc_reading, double& cj_reading, uint8_t& fault){
  fault = Thermocouple.readFault();
  if (fault) {
    if (fault & MAX31856_FAULT_CJRANGE){ DBG dbgLog(LOG_ERR,"[MAX31856] Cold Junction Range Fault");}
    if (fault & MAX31856_FAULT_TCRANGE){ DBG dbgLog(LOG_ERR,"[MAX31856] Thermocouple Range Fault");}
    if (fault & MAX31856_FAULT_CJHIGH){  DBG dbgLog(LOG_ERR,"[MAX31856] Cold Junction High Fault");}
    if (fault & MAX31856_FAULT_CJLOW){   DBG dbgLog(LOG_ERR,"[MAX31856] Cold Junction Low Fault");}
    if (fault & MAX31856_FAULT_TCHIGH){  DBG dbgLog(LOG_ERR,"[MAX31856] Thermocouple High Fault");}
    if (fault & MAX31856_FAULT_TCLOW){   DBG dbgLog(LOG_ERR,"[MAX31856] Thermocouple Low Fault");}
    if (fault & MAX31856_FAULT_OVUV){    DBG dbgLog(LOG_ERR,"[MAX31856] Over/Under Voltage Fault");}
    if (fault & MAX31856_FAULT_OPEN){    DBG dbgLog(LOG_ERR,"[MAX31856] Thermocouple Open Fault");}
    return;
  }

  cj_reading = Thermocouple.readCJTemperature(); 
  tc_reading = Thermocouple.readThermocoupleTemperature();
}
#endif



#if TC_BOARD_TYPE_KILN == TC_BOARD_MAX31856
// Initialize MAX31856 of KILN
Adafruit_MAX31856 Thermocouple_Kiln(TC_CS_KILN,&ESP32_SPI);
// Thermocouple_Kiln temperature readout
void Read_Temperature_Kiln(double& tc_reading, double& cj_reading, uint8_t& fault){
  Read_Temperature_MAX31856(Thermocouple_Kiln,tc_reading,cj_reading,fault);
}

void Setup_Thermocouple_Kiln(){
  Thermocouple_Kiln.begin();
}
#endif

#if TC_BOARD_TYPE_HOUSING == TC_BOARD_MAX31856
// Initialize MAX31856 of HOSING
Adafruit_MAX31856 Thermocouple_Housing(TC_CS_HOUSING,&ESP32_SPI);
// Thermocouple_Housing temperature readout
void Read_Temperature_Housing(double& tc_reading, double& cj_reading, uint8_t& fault){
  Read_Temperature_MAX31856(Thermocouple_Housing,tc_reading,cj_reading,fault);
}

void Setup_Thermocouple_Housing(){
  Thermocouple_Housing.begin();
}
#endif
