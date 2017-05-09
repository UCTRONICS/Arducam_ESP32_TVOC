/**************************************
 Hardware connection
 
KQM2801A          ESP32

5V                3V3
GND               GND
A                 RX0
B                 TX0
**************************************

DHT11            ESP32

VCC               3V3
DATA              D2
GND               GND

***************************************

OLED             ESP32

GND               GND
VCC               3V3
SCL               D22
SDA               D21
**************************************/

#include <WiFi.h>
#include <ArducamSSD1306.h>    // Modification of Adafruit_SSD1306 for ESP32 compatibility
#include <Adafruit_GFX.h>   // Needs a little change in original Adafruit library (See README.txt file)
#include <Wire.h>           // For I2C comm, but needed for not getting compile error
#include <dht.h>


// Pin definitions
#define OLED_RESET 19  // Pin 19 -RESET digital signal

ArducamSSD1306 display(OLED_RESET); // FOR I2C


//Wifi Thingspeak data logging

WiFiClient client;

String apiKey = "GT8A0Q5UFOHFUHUI";
char ssid[20] = "KK";
char password[20] = "12345687";




const char* server = "api.thingspeak.com";


//Temperature humidity Sensor
//#define SHT_ADDRESS 0x44
//Adafruit_SHT31 sht31 = Adafruit_SHT31();
DHT sensor = DHT();

//Global Variables
float temperature,humidity;
float tvoc;
uint16_t tvocTemp;

void setup() 
{
        sensor.attach(2);
  
      Serial.begin(9600);
      display.begin();  // Switch OLED  
      display.clearDisplay(); 
      display.setCursor(0,20);
      display.print("Connecting to:");
    
      display.setCursor(0,40);
      display.print(ssid);
      
        // Setup the Internet
      WiFi.begin(ssid, password);
      int pos  = 0;
      int dot = 20;
      while (WiFi.status() != WL_CONNECTED) 
      {
    
        delay(500);
        pos++;
        dot++;
        display.setCursor(dot,40);
        display.print(".");
       
      }
    
      display.clearDisplay();
      delay(100); 
  
}

void loop() 
{
 
      iaqUpdate();  
      readSensors();
      updateDisplay();
      printData();
      logData();
}
//Update and print data from TVOC
void iaqUpdate() 
{ 
  char revData[4]={0};
   readline(revData,4);
    tvoc=(float)tvocTemp*0.1;
   readline(revData,4);
   
}


void readSensors()
{
   sensor.update();

    switch (sensor.getLastError())
    {
        case DHT_ERROR_OK:
        temperature= sensor.getTemperatureInt();
         humidity = sensor.getHumidityInt();
         break;
        case DHT_ERROR_START_FAILED_1:
            Serial.println("Error: start failed (stage 1)");
            break;
        case DHT_ERROR_START_FAILED_2:
            Serial.println("Error: start failed (stage 2)");
            break;
        case DHT_ERROR_READ_TIMEOUT:
            Serial.println("Error: read timeout");
            break;
        case DHT_ERROR_CHECKSUM_FAILURE:
            Serial.println("Error: checksum error");
            break;
    }
 }

void updateDisplay()
{
      display.setTextSize(1);
      display.setTextColor(WHITE);
    
      display.clearDisplay();
      display.setCursor(0,0);
      display.print(" Indoor Air Quality     ");
     
     
      display.setCursor(0,20);
      display.print("Temp     :");
      display.setCursor(70,20);
      display.print(temperature);
      display.setCursor(100,20);
      display.print(" C");
    
      display.setCursor(0,30);
      display.print("Humidity :");
      display.setCursor(70,30);
      display.println(humidity);
      display.setCursor(100,30);
      display.print(" %");
 
     display.setCursor(0,50);
     display.print("TVOC     :");
      display.setCursor(70,50);
     if(tvocTemp==0xffff)
      display.print("preheat");
      else{
      display.print(tvoc);
      display.setCursor(100,50);
      display.print(" ppb");
      }
      display.display();
}

void logData()
{ 
      if (client.connect(server,80)) 
        {  
          String postStr = apiKey;
                 postStr +="&field1=";
                 postStr += String(temperature);
                 postStr +="&field2=";
                 postStr += String(humidity);
                 postStr +="&field3=";
                 postStr += String(tvoc);
                 postStr += "\r\n\r\n";
       
           client.print("POST /update HTTP/1.1\n"); 
           client.print("Host: api.thingspeak.com\n"); 
           client.print("Connection: close\n"); 
           client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n"); 
           client.print("Content-Type: application/x-www-form-urlencoded\n"); 
           client.print("Content-Length: "); 
           client.print(postStr.length()); 
           client.print("\n\n"); 
           client.print(postStr);
                         
                 
          }
         
       client.stop();
       Serial.println("****Next data log after 20 seconds*****");    
         
       // thingspeak needs minimum 15 sec delay between updates
       delay(20000);    

}


//prints data to the terminal, can be removed later

void printData()
{
    
      Serial.println("****Indoor Air Quality Values******");
      
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.println(" *C "); 
    
      Serial.print("Humidity");  
      Serial.print(humidity);
      Serial.println("%");
    
       Serial.print("TVOC:");
       Serial.print(tvoc);
       Serial.println("ppm");
}

uint8_t readline(char *buff, uint8_t maxbuff)
  {
    uint16_t buffidx = 0;
    char c;

  while (true) {
    if (Serial.available()) {
      c =  Serial.read();
      }
      if (c != 0x5F) continue;
      buff[buffidx] = c;
      buffidx++;
      while(buffidx <maxbuff)
      {
         if (Serial.available()) {
           char c =  Serial.read();
           buff[buffidx] = c;
           buffidx++;
         }  
      }
     buffidx==0;
     if(buff[1]==0xFF&& buff[2]==0xFF){
      tvocTemp = 0xFFFF; break;
      }
     
     if(buff[0]+buff[1]+buff[2]==buff[3]){
       tvocTemp = buff[1];tvocTemp= tvocTemp<<8;tvocTemp=tvocTemp|buff[2];  
     break;
     }else
     continue;  
  }
    }
