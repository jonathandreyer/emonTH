// emonTH Low Power Pulse Counting
// Interrupt based pulse counting with MCU sleeping in between interrupts
// Wakeup periodically (default 60s) to send current pulse count via RF

// Default settings RFM69CW and emonTH V1.5


bool debug = 1;

#define RF69_COMPAT 1                       // Set to 1 if using RFM69CW or 0 is using RFM12B
#include <JeeLib.h>                         // https://github.com/jcw/jeelib (RFu_JeeLib required for emonTH V1.4 https://github.com/openenergymonitor/rfu_jeelib)

#include <avr/power.h>
#include <avr/sleep.h>
ISR(WDT_vect) { Sleepy::watchdogEvent(); }


#define RF_freq RF12_433MHZ                 // Frequency of RF12B module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
int nodeID = 2;                             // EmonTH temperature RFM12B node ID - should be unique on network
const int networkGroup = 1;                 // EmonTH RFM12B wireless network group - needs to be same as emonBase and emonGLCD

cosnt int TIME_IN_BETWEEN_READINGS = 60000; //time between RF transmissions (ms)
const byte min_pulsewidth= 110;             // minimum width of interrupt pulse (default pulse output meters = 100ms)
const int LED=            9;
const int BATT_ADC=       1;
cost int IRQ_PIN=         1;                // IRQ 1 for emonTH V1.5 / IRQ 0 for emonTH V1.4


unsigned long pulsetime=0;                                      // Record time of interrupt pulse

typedef struct {                                                      // RFM69CW / RFM12B RF payload datastructure
  	  unsigned long pulsecount;
          int battery;
} Payload;
Payload emonth;

void setup() {
  pinMode(LED,OUTPUT); digitalWrite(LED,HIGH);
  
  if (debug) {
    Serial.begin(9600);
    Serial.println("emonTH Pulse Counting example");
    delay(100);
  } else {
    power_usart0_disable();
  }
  power_twi_disable();
  
  rf12_initialize(nodeID, RF_freq, networkGroup);
  rf12_sleep(RF12_SLEEP);
  
  emonth.pulsecount = 0;
  
  attachInterrupt(IRQ_PIN, onPulse, FALLING);
  digitalWrite(LED,LOW);
}

void loop()
{
  // STATUS COUNT
  emonth.battery = analogRead(BATT_ADC) * 3.32;                    //read battery voltage, convert ADC to volts x10

  if (debug) {
    Serial.print("Count: ");
    Serial.println(emonth.pulsecount);
    delay(25);
    Serial.print("Battery: ");
    Serial.println(emonth.battery);
    delay(25);
  }

  if (pulseCount)                                                       // if the ISR has counted some pulses, update the total count
  {
    cli();                                                              // Disable interrupt just in case pulse comes in while we are updating the count
    emonth.pulseCount += pulseCount;
    pulseCount = 0;
    sei();                                                              // Re-enable interrupts
    }
  
  power_spi_enable();
  rf12_sleep(RF12_WAKEUP);
  Sleepy::loseSomeTime(20);
  rf12_sendNow(0, &emonth, sizeof emonth);
  rf12_sendWait(2);
  rf12_sleep(RF12_SLEEP);
  emonth.pulsecount++;   // TRY TO GET NUMBER SENT
  power_spi_disable();
  
  Sleepy::loseSomeTime(TIME_IN_BETWEEN_READINGS);  //TAKEN ONE ZERO OFF FOR TESTING!!!
  
}

// The interrupt routine - runs each time a falling edge of a pulse is detected
void onPulse()
{
  if ( (millis() - pulsetime) > min_pulsewidth) {
    pulseCount++;					//calculate wh elapsed from time between pulses
    pulsetime=millis();
    
    digitalWrite(LED, HIGH);
    delay(2);
    digitalWrite(LED, LOW);
  }
}
