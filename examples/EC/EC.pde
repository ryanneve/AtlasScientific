#include <Atlas_EZO.h> // DO, EC, ORP and pH

#define EC_BAUD_RATE 9600 // This must be correct

EZO_EC		EC_sensor;  // creates instance of EZO_EC called EC_sensor

void setup(){
  // THIS IS JUST FOR MY HARDWARE, YOU CAN COMMENT IT OUT----------------------------
  #define PIN_ATLAS_VCC		2	// Output to power Atlas Circuits.
  #define PIN_ATLAS_SELECT_S0	7	// Output to select which Atlas circuit is on Serial2
  #define PIN_ATLAS_SELECT_S1	8	// Output to select which Atlas circuit is on Serial2
  pinMode(PIN_ATLAS_SELECT_S0,OUTPUT);  digitalWrite(PIN_ATLAS_SELECT_S0,HIGH);
  pinMode(PIN_ATLAS_SELECT_S1,OUTPUT);  digitalWrite(PIN_ATLAS_SELECT_S1,LOW);
  pinMode(PIN_ATLAS_VCC,OUTPUT);        digitalWrite(PIN_ATLAS_VCC,HIGH);
  delay(2000);
  // BACK TO THE EXAMPLE-------------------------------------------------------------------
  Serial.begin(57600);
  Serial2.begin(EC_BAUD_RATE);
  EC_sensor.debugOn(); // optional
  EC_sensor.begin(&Serial2,EC_BAUD_RATE);
  EC_sensor.initialize(); // Gets a bunch of settings from the circuit
  EC_sensor.setOnline();
  Serial.print("The voltage is:");
  Serial.println(EC_sensor.getVoltage());
  Serial.print("The sensor K values is set to:");
  EC_sensor.queryK(); // Asks the senor for it's K value
  Serial.println(EC_sensor.getK());
  // and any other info you might want
}

void loop(){
  EC_sensor.querySingleReading();
  Serial.println("The EC values are:");
  Serial.print("    EC  = "); Serial.println(EC_sensor.getEC());
  Serial.print("    TDS = "); Serial.println(EC_sensor.getTDS());
  Serial.print("    SAL = "); Serial.println(EC_sensor.getSAL());
  Serial.print("    SG  = "); Serial.println(EC_sensor.getSG());
  EC_sensor.sleep(); // to save power
  delay(5000);
  EC_sensor.wake();
}