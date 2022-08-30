#include <dht.h>
#include <SoftwareSerial.h>
#include <String.h>
#include "LowPower.h"

SoftwareSerial gprsSerial(8,9); // Tx,Rx
dht DHT; 
#define dht_apin A0 // Analog Pin sensor is connected to
#define sensor_relay 2
#define gsm_relay 3
#define bat1_relay 11
#define bat2_relay 12
#define bat3_relay 4

float hum;
float temp; 
float aqi;
int NREADING = 5;
int SENSEDELAY = 1000; //2 s delay between sensor reads
int startTime,endTime;

void restart();

void setup(){
  gprsSerial.begin(9600);  
  Serial.begin(9600);
  delay(1000);//Delay to let system boot
  pinMode(sensor_relay,OUTPUT);
  pinMode(gsm_relay,OUTPUT);
  pinMode(bat1_relay,OUTPUT);
  pinMode(bat2_relay,OUTPUT);
  pinMode(bat3_relay,OUTPUT);
  digitalWrite(sensor_relay,LOW);
  digitalWrite(gsm_relay,HIGH);
  digitalWrite(bat1_relay,HIGH);
  digitalWrite(bat2_relay,HIGH);
  digitalWrite(bat2_relay,HIGH);}//end "setup()"
 
void loop(){
        //Start of Program 
        int count = 5;
        int check;
        digitalWrite(bat1_relay,LOW);     //series connected batteries
        digitalWrite(bat2_relay,LOW);
        digitalWrite(bat3_relay,LOW);
        delay(1000);
        digitalWrite(sensor_relay,HIGH);      // Turn on sensors
        delay(1000);
        readSensors(NREADING);                // Sensor Reads
        Serial.print("AirQua="); 
        Serial.print(aqi, DEC);               // prints the value read
        Serial.println(" PPM");  
        Serial.print("Current AVG humidity = ");
        Serial.print(hum);
        Serial.print("%  ");
        Serial.print("AVG temperature = ");
        Serial.print(temp); 
        Serial.println("C  ");
        delay(1000);
        digitalWrite(sensor_relay,LOW);       // Turn off sensors
        delay(1000);
        
        digitalWrite(gsm_relay,LOW);
        delay(20000);  //to be 20s
        while(count)
        {check = sendData();     
        if(check) break;
        count--;
        }

        if(!check)SendMessage();
        
        digitalWrite(gsm_relay,HIGH);
        delay(1000);
        
        //Fastest should be once every two seconds.
        digitalWrite(bat1_relay,HIGH);     //parallel connected batteries
        digitalWrite(bat2_relay,HIGH);
        digitalWrite(bat3_relay,HIGH);
        
        unsigned int sleepCounter;    // sleepcounter = no of seconds to sleep/8
        for (sleepCounter = 6*3600/8; sleepCounter > 0; sleepCounter--)
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);

}// end loop(

void readSensors(int N){
    int i;
    float temp_hum=0;
    float temp_temp=0;
    float temp_aqi=0;
    for(i=0;i<N;i++)
    {
        DHT.read11(dht_apin);
        //Serial.print("Current humidity = ");
        
        temp_hum += DHT.humidity;       
        temp_temp += DHT.temperature;
        temp_aqi += analogRead(2);  //AQI read
        //Serial.print(analogRead(2),DEC);
        //Serial.print("  ");
        //Serial.print(DHT.humidity);
        //Serial.print("  ");
        //Serial.print(DHT.temperature); 
        //Serial.print("     ");
        delay(SENSEDELAY);
    }
    hum = temp_hum/N;
    temp = temp_temp/N;
    aqi = temp_aqi/N; 
}

int sendData(){
  Serial.write("DATA SEND START");
  int errorcount = 0;
  int data_prev, data,flag;
  if (gprsSerial.available())
    Serial.write(gprsSerial.read());
 
  gprsSerial.println("AT");
  delay(1000);
  
  gprsSerial.println("AT+CPIN?");
  delay(1000);
  
  gprsSerial.println("AT+CREG?");
  delay(1000);
 
  gprsSerial.println("AT+CGATT?");
  delay(1000);
 
  gprsSerial.println("AT+CIPSHUT");
  delay(1000);
 
  gprsSerial.println("AT+CIPSTATUS");
  delay(2000);
 
  gprsSerial.println("AT+CIPMUX=0");
  delay(2000);
 
  ShowSerialData();

  cstt:   //Label to restart gprs connection
  if(errorcount > 10) return 0;
  gprsSerial.println("AT+CSTT=\"wap\"");//start task and setting the APN,
  delay(3000);
  //ShowSerialData();
  flag = 0;
  while(gprsSerial.available()!=0)
  {
    data_prev = data;
    data = gprsSerial.read();
    Serial.write(data);
    if(data_prev == 79 && data == 75)
        flag = 1;
   }
  if(flag==1)Serial.write("OK FOUND\n");
  else {Serial.write("ERROR DETECTED\n");errorcount++;goto cstt;}

 
  gprsSerial.println("AT+CIICR");//bring up wireless connection
  delay(8000);
  flag = 0;
  while(gprsSerial.available()!=0)
  {
    data_prev = data;
    data = gprsSerial.read();
    Serial.write(data);
    if(data_prev == 79 && data == 75)
        flag = 1;
   }
  if(flag==1)Serial.write("OK FOUND\n");
  else {Serial.write("ERROR DETECTED\n");errorcount++;goto cstt;}
  //ShowSerialData();
 
  gprsSerial.println("AT+CIFSR");//get local IP adress
  delay(3000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIPSPRT=0");
  delay(3000);
 
  ShowSerialData();
  
  gprsSerial.println("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",\"80\"");//start up the connection
  delay(6000);
 
  ShowSerialData();
 
  gprsSerial.println("AT+CIPSEND");//begin send data to remote server
  delay(6000);
  flag = 1;
  while(gprsSerial.available()!=0)
  {
    data_prev = data;
    data = gprsSerial.read();
    Serial.write(data);
    if(data_prev == 79 && data == 82) //ERROR
        flag = 0;
   }

  if(flag==1)Serial.write("OK FOUND\n");
  else {errorcount++;Serial.write("ERROR DETECTED\n");goto cstt;}
  
  String str="GET https://api.thingspeak.com/update?api_key=0INH4CZQ4820JM0J&field1=" + String(temp) +"&field2="+String(hum)+"&field3="+String(aqi);
  
  Serial.println(str);
  gprsSerial.println(str);//begin send data to remote server
  
  delay(5000);
  ShowSerialData();
 
  gprsSerial.println((char)26);//sending
  delay(6000);//waitting for reply, important! the time is base on the condition of internet 
  gprsSerial.println();
  flag = 0;
  while(gprsSerial.available()!=0)
  {
    data_prev = data;
    data = gprsSerial.read();
    Serial.write(data);
    if(data_prev == 79 && data == 75)
        flag = 1;
   }
  if(flag==1)Serial.write("OK FOUND\n");
  else {Serial.write("ERROR DETECTED\n");
    errorcount++;goto cstt;}
 
  gprsSerial.println("AT+CIPSHUT");//close the connection
  delay(100);
  ShowSerialData();
  Serial.write("\nsend complete");
  return 1;
}
void ShowSerialData()
{
  while(gprsSerial.available()!=0)
  Serial.write(gprsSerial.read());
  delay(3000); 
}

void restart()
{
  digitalWrite(gsm_relay,HIGH);
  delay(1000);
  digitalWrite(gsm_relay,LOW);
}

void SendMessage()
{
  gprsSerial.println("AT+CMGF=1");    //Sets the GSM Module in Text Mode
  delay(1000);  // Delay of 1000 milli seconds or 1 second
  gprsSerial.println("AT+CMGS=\"+88XXXXXXXXXX\"\r"); // Replace x with mobile number
  delay(1000);
  gprsSerial.println("https://api.thingspeak.com/update?api_key=0INH4CZQ4820JM0J&field1=" + String(temp) +"&field2="+String(hum)+"&field3="+String(aqi));// The SMS text you want to send
  delay(100);
  gprsSerial.println((char)26);// ASCII code of CTRL+Z
  delay(1000);
}
