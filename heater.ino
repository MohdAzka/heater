// #include <Wire.h>
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 8
Adafruit_SSD1306 display(OLED_RESET);


int THERMISTOR = 0;
double temp;    

const int HEATER1   = 8;
const int HEATER2   = 9;
const int FAN       = 7;
const int SAMPLE    = 100;

unsigned long clock = 0;
unsigned long min   = 0;
unsigned long sec   = 0;

char currentStage = 0;

int flag     = HIGH;
int rst_flag = HIGH;
int led_flag = HIGH;

uint32_t my_arr[100] = {0};
uint32_t sum = 0;
double   avg = 0;
int Vo;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

enum stages
{
    FIRST  = 1,
    SECOND,
    THIRD,
    FORTH,
    FIFTH
};


float temperature()
{
    float R1 = 10000;
    float logR2, R2, T;
    
    for(char a= 0; a < SAMPLE; ++a)
    {
        my_arr[a] = analogRead(THERMISTOR);
        sum = sum + my_arr[a];
        delay(1);
    }
    avg = sum/100;
    sum =0;
    
    R2 = R1 * (1023.0 / (float)avg - 1.0);
    logR2 = log(R2);
    T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
    T = T - 273.15;

    Serial.print("Temperature: "); 
    Serial.print(T);
    Serial.print(" C  "); 
    return T;
}

void setup() 
{
    Serial.begin(9600); //Start the Serial Port at 9600 baud (default)

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)

    Serial.println("HEATER");

    pinMode(THERMISTOR,INPUT);
    pinMode(HEATER1,OUTPUT);
    pinMode(HEATER2,OUTPUT);
    pinMode(FAN,OUTPUT);

    currentStage = FIRST;       
}

void displayUpdate(float temp, unsigned long min, unsigned long sec)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);

    display.setCursor(0,0);
    display.print("TEMP:");
    display.println(temp);

    display.setCursor(0,10);
    display.print("STAGE:");
    display.print(currentStage,DEC);
    // Serial.print("currrent stage");
    // Serial.print(currentStage,DEC);

    display.setCursor(0,20);
    display.print("TIMER: ");
    display.print(min);
    display.print(':');
    display.print(sec);

    display.display();
}

void heatersON( int value )
{
    analogWrite( HEATER1, value );
    analogWrite( HEATER2, value );
}

void fanON()
{
    digitalWrite(FAN, HIGH);    
}

void fanOFF()
{
    digitalWrite(FAN, LOW); 
}

boolean reaching(char stage, int temp, unsigned long timer)
{

    if(currentStage != stage) return 0;

    float tc = temperature();
    // int percent = (temp - tc)/temp*100; 
    int percent = ((temp - tc)*100)/100;

    Serial.print("stage:");
    Serial.print(stage,DEC);
    Serial.print(", percent:");
    Serial.println(percent);

    // percent>3 ? heatersON(255): heatersON(percent*2.5+130);
    if(percent > 2)
    {
        heatersON(255);
        fanOFF();
    }
    else if(percent < -2){
        heatersON(0);
        fanON();
    }
    else
        heatersON(percent*2.5+130);
    
    //get out of this stage
    tc > temp && tc < temp+1 ? clock = millis(),currentStage++ : millis();

    displayUpdate(tc, 0, 0);
}

boolean retaining(char stage, int temp, unsigned long timer)
{
    if(currentStage != stage) return 0;

    float tc = temperature();
    int percent = ((temp - tc)*100)/100;

    Serial.print("stage:");
    Serial.print(stage,DEC);
    Serial.print(", percent:");
    Serial.println(percent);

    percent>3 ? heatersON(255): heatersON(percent*2.5+130);

    if( millis() > clock + (timer*60*1000) )
        currentStage != 4 ? currentStage++:currentStage=FIRST;
    else
        min = ((clock + (timer*60*1000) - millis())/1000)/60;
        sec = ((clock + (timer*60*1000) - millis())/1000)%60;


    displayUpdate(tc, min, sec);

    return 1;
}


void callStages(char stage, int temp, unsigned long timer, boolean (*operation)(char,int,unsigned long))
{
    operation(stage,temp,timer);
}

void loop() 
{
    callStages(1, 65, 0, reaching);    
    callStages(2, 65, 5, retaining);
    callStages(3, 95, 0, reaching);
    callStages(4, 95, 5, retaining);
}