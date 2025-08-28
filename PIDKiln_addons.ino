/*
** Function for relays (SSR, EMR) and temperature sensors
**
*/

// SPIClass for thermocouples
// SPIClass *ESP32_SPI = new SPIClass(HSPI);
SPIClass ESP32_SPI(HSPI);    // object, not pointer

// If we have defines power meter pins
#ifdef ENERGY_MON_PIN
#include <EmonLib.h>
#define ENERGY_MON_AMPS 30        // how many amps produces 1V on your meter (usualy with voltage output meters it's their max value).
#define EMERGY_MON_VOLTAGE 230    // what is your mains voltage
#define ENERGY_IGNORE_VALUE 0.4   // if measured current is below this - ignore it (it's just noise)
EnergyMonitor emon1;
#endif
uint16_t Energy_Wattage=0;        // keeping present power consumtion in Watts
double Energy_Usage=0;            // total energy used (Watt/time)


boolean SSR_On; // just to narrow down state changes.. I don't know if this is needed/faster

// Simple functions to enable/disable SSR - for clarity, everything is separate
//
void Enable_SSR(){
  if(!SSR_On){
    digitalWrite(SSR1_RELAY_PIN, HIGH);
#ifdef SSR2_RELAY_PIN
    digitalWrite(SSR2_RELAY_PIN, HIGH);
#endif
    SSR_On=true;
  }
}

void Disable_SSR(){
  if(SSR_On){
    digitalWrite(SSR1_RELAY_PIN, LOW);
#ifdef SSR2_RELAY_PIN
    digitalWrite(SSR2_RELAY_PIN, LOW);
#endif
    SSR_On=false;
  }
}

void Enable_EMR(){
  digitalWrite(EMR_RELAY_PIN, HIGH);
}

void Disable_EMR(){
  digitalWrite(EMR_RELAY_PIN, LOW);
}

void print_bits(uint32_t raw){
    for (int i = 31; i >= 0; i--)
    {
        bool b = bitRead(raw, i);
        Serial.print(b);
    }

Serial.println();
}

// Kiln Thermocouple temperature update
void Update_Temperature_Kiln(){
  uint8_t fault=0;
  double tc_temp, cj_temp;
  Read_Temperature_Kiln(tc_temp,cj_temp,fault);
  if (fault!=0){
    if(Temp_Kiln_errors<Prefs[PRF_ERROR_GRACE_COUNT].value.uint8){
      Temp_Kiln_errors++;
      DBG dbgLog(LOG_ERR,"[ADDONS] Kiln Thermocouple had an error but we are still below grace threshold - continue. Error %d of %d\n",Temp_Kiln_errors,Prefs[PRF_ERROR_GRACE_COUNT].value.uint8);
    }else{
      ABORT_Program(PR_ERR_MAX31A_INT_ERR);
    }
    return;
  }

  int_temp = (int_temp+cj_temp)/2;
  kiln_temp=(kiln_temp*0.9+tc_temp*0.1);    // We try to make bigger hysteresis

  if(Temp_Kiln_errors>0) Temp_Kiln_errors--;  // Lower errors count after proper readout
  
  DBG dbgLog(LOG_DEBUG, "[ADDONS] Kiln Temperature sensor readout: Internal temp = %.1f \t Last temp = %.1f \t Average kiln temp = %.1f\n", int_temp, tc_temp, kiln_temp); 
}

// Housing Thermocouple temperature update
#ifdef TC_BOARD_TYPE_HOUSING
void Update_Temperature_Housing(){
  uint8_t fault=0;
  double tc_temp, cj_temp;
  Read_Temperature_Housing(tc_temp,cj_temp,fault);
  if (fault!=0){
    if(Temp_Housing_errors<Prefs[PRF_ERROR_GRACE_COUNT].value.uint8){
      Temp_Housing_errors++;
      DBG dbgLog(LOG_ERR,"[ADDONS] Housing Thermocouple had an error but we are still below grace threshold - continue. Error %d of %d\n",Temp_Housing_errors,Prefs[PRF_ERROR_GRACE_COUNT].value.uint8);
    }else{
      ABORT_Program(PR_ERR_MAX31B_INT_ERR);
    }
    return;
  }

  int_temp = (int_temp+cj_temp)/2;
  case_temp=(case_temp*0.8+tc_temp*0.2);    // We try to make bigger hysteresis

  if(Temp_Housing_errors>0) Temp_Housing_errors--;  // Lower errors count after proper readout
  
  DBG dbgLog(LOG_DEBUG, "[ADDONS] Housing Temperature sensor readout: Internal temp = %.1f \t Last temp = %.1f \t Average kiln temp = %.1f\n", int_temp, tc_temp, case_temp); 
}
#endif

// Measure current power usage - to be expanded
//
void Read_Energy_INPUT(){
double Irms;
static uint8_t cnt=0;
static uint32_t last=0;

#ifdef ENERGY_MON_PIN
  Irms = emon1.calcIrms(512);  // Calculate Irms only; 512 = number of samples (internaly ESP does 8 samples per measurement)
  if(Irms<ENERGY_IGNORE_VALUE){
    Energy_Wattage=0;
    return;   // In my case everything below 0,3A is just noise. Comparing to 10-30A we are going to use we can ignore it. Final readout is correct.  
  }
  Energy_Wattage=(uint16_t)(Energy_Wattage+Irms*EMERGY_MON_VOLTAGE)/2;  // just some small hysteresis
  if(last){
    uint16_t ttime;
    ttime=millis()-last;
    Energy_Usage+=(double)(Energy_Wattage*ttime)/3600000;  // W/h - 60*60*1000 (miliseconds)
  }
  last=millis();

  if(cnt++>20){
    DBG dbgLog(LOG_DEBUG,"[ADDONS] VCC is set:%d ; RAW Power: %.1fW, Raw current: %.2fA, Power global:%d W/h:%.6f\n",emon1.readVcc(),Irms*EMERGY_MON_VOLTAGE,Irms,Energy_Wattage,Energy_Usage);
    cnt=0;
  }

#else
  return;
#endif

}


// Power metter loop - read energy consumption
//
void Power_Loop(void * parameter){
  for(;;){
    Read_Energy_INPUT();  // current redout takes around 3-5ms - so we will do it 10 times a second.
    vTaskDelay( 100 / portTICK_PERIOD_MS );
  }
}


// Stops Alarm
//
void STOP_Alarm(){
  ALARM_countdown=0;
  digitalWrite(ALARM_PIN, LOW);
}
// Start Alarm
//
void START_Alarm(){
  if(!Prefs[PRF_ALARM_TIMEOUT].value.uint16) return;
  ALARM_countdown=Prefs[PRF_ALARM_TIMEOUT].value.uint16;
  digitalWrite(ALARM_PIN, HIGH);
}


void Setup_Addons(){
  pinMode(EMR_RELAY_PIN, OUTPUT);
  pinMode(SSR1_RELAY_PIN, OUTPUT);
#ifdef SSR2_RELAY_PIN
    pinMode(SSR2_RELAY_PIN, OUTPUT);
#endif

  pinMode(ALARM_PIN, OUTPUT);

  SSR_On=false;
  Setup_Thermocouple_Kiln();
#ifdef TC_BOARD_TYPE_HOUSING
  Setup_Thermocouple_Housing();
#endif

#ifdef ENERGY_MON_PIN
  emon1.current(ENERGY_MON_PIN, ENERGY_MON_AMPS);
  xTaskCreatePinnedToCore(
              Power_Loop,      /* Task function. */
              "Power_metter",  /* String with name of task. */
              8192,            /* Stack size in bytes. */
              NULL,            /* Parameter passed as input of the task */
              2,               /* Priority of the task. */
              NULL,1);         /* Task handle. */
              
#endif
}
