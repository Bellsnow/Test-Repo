#include <Wire.h>
#define address_ADC 0x48 //define address of ADC

void data_ADC(/*volatile*/ byte *high, /*volatile*/ byte *low); //function of adc_data recieving
void thresh_Hi(byte high, byte low); //function of setting hi_threshold
void thresh_Lo(byte high, byte low); //function of setting lo_threshold
void ISR_0(); //function of ISR_0

/*volatile*/ byte data_high_p;
/*volatile*/ byte data_low_p;
/*volatile*/ int data_past;
/*volatile*/ byte data_high;
/*volatile*/ byte data_low;
/*volatile*/ int data_current;
volatile byte polling;
float mass;
byte count_feed;
byte count_meal;

/*volatile*/ unsigned long time_measure;
unsigned long time_current;

void setup() {
  Serial.begin(9600); //UART interface / BAUD:9600
  Wire.begin(); //TWI interface initialize
  
  //Interrupt setting
  EIMSK &= ~(0x03);
  polling = 0;
  
  //ADC initializing
  Wire.beginTransmission(address_ADC);
  Wire.write(0x01); //Config register of ADC
  Wire.write(0x0E); //MSB of stat_ADC
  Wire.write(0x10); //LSB of stat_ADC
  Wire.endTransmission();
  
  //Recieve initial DATA
  data_ADC(&data_high_p, &data_low_p);
  data_past = (int)(data_high_p <<8) | (int)data_low_p;
  data_current = data_past;
  mass = ((float)data_past + 39) * 8.347;
  
  //Print initial DATA
  Serial.print("Mass :");
  Serial.print(mass); Serial.print("/"); Serial.print(data_past);
  Serial.print(" Feed :");
  Serial.print(count_feed);
  Serial.print(" Meal :");
  Serial.println(count_meal);

  //Threshold setting
  thresh_Hi(data_high_p, data_low_p);
  thresh_Lo(data_high_p, data_low_p);
  
  //timer setting
  time_current = millis();
  time_measure = time_current;
  
  //enable interrupt
  attachInterrupt(0, ISR_0, LOW);
  EIMSK |= 0x01;
}

void loop() {
  time_current = millis();
  
  if(abs(data_current - data_past) <= 2)
    time_measure = time_current;
  
  if((time_current - time_measure) >= 10000) {
    EIMSK &= ~(0x01); //unable external interrupt
    
    //recieve data
    data_ADC(&data_high, &data_low);
    data_current = (int)(data_high << 8) | (int)data_low;
    mass = ((float)data_current + 39) * 8.347;
    
    //feed & meal counting
    if(data_current > data_past)
      count_feed++;
    else if(data_current < data_past)
      count_meal++;
    
    //threshold setting  
    thresh_Hi(data_high, data_low);
    thresh_Lo(data_high, data_low);
    
    //Print initial DATA
    Serial.print("Mass :");
    Serial.print(mass); Serial.print("/"); Serial.print(data_current);
    Serial.print(" Feed :");
    Serial.print(count_feed);
    Serial.print(" Meal :");
    Serial.println(count_meal);
    
    data_past = data_current; //reset past data
    time_measure = time_current; //reset timer
    
    //enable interrupt
    EIFR |= 0x01;
    EIMSK |= 0x01;
  }
  
  if(polling) {
    EIMSK &= ~(0x01); //unable external interrupt
  
    //recieve data
    data_ADC(&data_high, &data_low);
    data_current = (int)(data_high << 8) | (int)data_low;
  
    //threshold setting
    if(data_current > data_past)
      thresh_Hi(data_high, data_low);
    else
      thresh_Lo(data_high, data_low);
    
    //enable timer
    time_measure = time_current;
    
    //enable interrupt
    polling = 0;
    EIFR |= 0x01;
    EIMSK |= 0x01;
  } 
}

void data_ADC(/*volatile*/ byte *high, /*volatile*/ byte *low) {
  Wire.beginTransmission(address_ADC);
  Wire.write(0x00);
  Wire.endTransmission();
  
  Wire.requestFrom(address_ADC, 2);
  *high = Wire.read();
  *low = Wire.read();
}

void thresh_Hi(byte high, byte low) {
  Wire.beginTransmission(address_ADC);
  Wire.write(0x03);
  Wire.write(high);
  Wire.write(low+2);
  Wire.endTransmission();
}

void thresh_Lo(byte high, byte low) {
  Wire.beginTransmission(address_ADC);
  Wire.write(0x02);
  Wire.write(high);
  Wire.write(low-2);
  Wire.endTransmission();
}

void ISR_0() {
  polling = 1;
}
