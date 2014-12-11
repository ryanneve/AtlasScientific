

//#include <Logger_SD.h>// for my SD logging system

#include <Atlas_EC.h>;

#define SERIAL_EC Serial3

AtlasEC sensor_EC(&SERIAL_EC,46);

void setup(){
  Serial.begin(9600);
  sensor_EC.initialize(true);
  sensor_EC.debug = true;
  sensor_EC.testConnection();
  sensor_EC.readOutput();
  Serial.println("Output Settings");
  Serial.print("  EC   ");  Serial.println(sensor_EC.getOutputEC() );
  Serial.print("  TDS  ");  Serial.println(sensor_EC.getOutputTDS());
  Serial.print("  Sal  ");  Serial.println(sensor_EC.getOutputS()  );
  Serial.print("  SG   ");  Serial.println(sensor_EC.getOutputSG() );
  sensor_EC.readStatus();
  Serial.print("Voltage: ");  Serial.println(sensor_EC.getStatusVoltage() );
  sensor_EC.getInfo();
  Serial.print("Version: ");  Serial.println(sensor_EC.getVersion() );
  Serial.print("LED: ");      Serial.println(sensor_EC.getLED() );
  Serial.print("Response Mode: ");      Serial.println(sensor_EC.getResponseMode() );
  Serial.print("Probe K: ");  Serial.println(sensor_EC.getProbeK() );
  Serial.print("Temperature: ");  Serial.println(sensor_EC.getTemperature() );
  
  
}

void loop(){
  sensor_EC.readData();
  Serial.println("EC EZO data:");
  Serial.print("  EC   "); Serial.println(sensor_EC.getEC());
  Serial.print("  TDS  "); Serial.println(sensor_EC.getTDS());
  Serial.print("  SAL  "); Serial.println(sensor_EC.getSal());
  Serial.print("  SG   "); Serial.println(sensor_EC.getSG());
  delay(5000);
}