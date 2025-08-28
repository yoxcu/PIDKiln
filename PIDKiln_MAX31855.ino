#if (TC_BOARD_TYPE_KILN == TC_BOARD_MAX31855) || (TC_BOARD_TYPE_HOUSING == TC_BOARD_MAX31855)
#include <MAX31855.h>
extern SPIClass ESP32_SPI;

// MAX31855 temperature readout
template <class TC>
void Read_Temperature_MAX31855(TC &Thermocouple, double &tc_reading,
                               double &cj_reading, uint8_t &fault) {
  uint32_t raw = Thermocouple.readRawData();

  if (!raw) { // probably MAX31855 not connected
    DBG dbgLog(LOG_ERR, "[MAX31855] Board did not respond\n");
    fault = MAX31855_ERROR;
    return;
  }

  if (Thermocouple.detectThermocouple(raw) != MAX31855_THERMOCOUPLE_OK) {
    fault = Thermocouple.detectThermocouple();
    switch (fault) {
    case MAX31855_THERMOCOUPLE_SHORT_TO_VCC:
      DBG dbgLog(LOG_ERR, "[MAX31855] short to VCC\n");
      break;

    case MAX31855_THERMOCOUPLE_SHORT_TO_GND:
      DBG dbgLog(LOG_ERR, "[MAX31855] short to GND\n");
      break;

    case MAX31855_THERMOCOUPLE_NOT_CONNECTED:
      DBG dbgLog(LOG_ERR, "[MAX31855] not connected\n");
      break;

    default:
      DBG dbgLog(LOG_ERR, "[MAX31855] unknown error, check spi cable\n");
      break;
    }
    return;
  }

  cj_reading = Thermocouple.getColdJunctionTemperature(raw);
  tc_reading = Thermocouple.getTemperature(raw);
}
#endif

#if TC_BOARD_TYPE_KILN == TC_BOARD_MAX31855
// Initialize MAX31855 of KILN
MAX31855 Thermocouple_Kiln(TC_CS_KILN);
// Thermocouple_Kiln temperature readout
void Read_Temperature_Kiln(double &tc_reading, double &cj_reading,
                           uint8_t &fault) {
  Read_Temperature_MAX31855(Thermocouple_Kiln, tc_reading, cj_reading, fault);
}

void Setup_Thermocouple_Kiln() { Thermocouple_Kiln.begin(&ESP32_SPI); }
#endif

#if TC_BOARD_TYPE_HOUSING == TC_BOARD_MAX31855
// Initialize MAX31855 of HOSING
MAX31855 Thermocouple_Housing(TC_CS_HOUSING);
// Thermocouple_Housing temperature readout
void Read_Temperature_Housing(double &tc_reading, double &cj_reading,
                              uint8_t &fault) {
  Read_Temperature_MAX31855(Thermocouple_Housing, tc_reading, cj_reading,
                            fault);
}

void Setup_Thermocouple_Housing() { Thermocouple_Housing.begin(&ESP32_SPI); }
#endif
