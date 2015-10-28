// sketch: http://forum.arduino.cc/index.php?topic=353502.0
// libraries: https://github.com/anakod/ESP8266pro
#include <SoftwareSerial.h> 
#include <ESP8266pro.h> 
#include <ESP8266proServer.h> 
#include <OneWire.h>
#include <DallasTemperature.h>

#define DEBUG true

const int  RX=10;     
const int  TX=11;     
const int  ts=9;     //temp sensor DS18B20
const int  il=13;    //indicator led
const int  ms=12;    //master switch relay

  #define ONE_WIRE_BUS ts
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature sensors(&oneWire);
         
const int   TN = 1400;                  //tone
float t;            //temperature  
 
unsigned long previousMillis = 0;         //check sensors
const long interval = 60000;               //Sampling interval time mSec

const long COM_BAUD = 38400;                       //ESP UART baudrate
const String mode = "1";                           //1 - Client, 2 - Access Point, 3 - Both                             
const String ssid = "yourwifissid";       //Network SSID
const String password = "yourwifipassword";              //Network Password
const String apipaddress = "192.168.4.1";          //Desired IP address
const String cipaddress = "192.168.0.201";          //Desired IP address
const String subnetworkmask = "255.255.255.0";  
const String gateway = "192.168.0.1";

SoftwareSerial esp8266(RX, TX); // RX, TX   
ESP8266pro wifi(esp8266, Serial); 
ESP8266proServer server(wifi, onClientRequest); 
String requestPaths[ESP_MAX_CONNECTIONS];     

void setup() {
    pinMode(7, INPUT);        // switch is attached to Arduino pin 7
    pinMode(8, INPUT);        // switch is attached to Arduino pin 8 
    pinMode(ts, INPUT);
    pinMode(ms, OUTPUT);
    pinMode(il, OUTPUT);     
    esp8266.begin(COM_BAUD);
    Serial.begin(COM_BAUD);    //for debug

  sendData("AT+RST\r\n",3400,DEBUG); // reset module
  sendData("AT+GMR\r\n",2000,DEBUG); // Sofware version
  sendData("AT+CWMODE="+mode+"\r\n",1000,DEBUG); // configure as access point
  sendData("AT+CWJAP=\""+ssid+"\",\""+password+"\"\r\n",10000,DEBUG); // connect to AP
  sendData("AT+CIPSTA_DEF=\""+cipaddress+"\",\""+gateway+"\",\""+subnetworkmask+"\"\r\n",1000,DEBUG); // set ip address
  sendData("AT+CIPMUX=1\r\n",1000,DEBUG); // configure for multiple connections
  //sendData("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // turn on server on port 80 
  
  // Start server on port 80
  server.start(80);
  Serial.println();
  Serial.println("=================================");
  if(mode=="1"){
  Serial.println("Server started: http://" + cipaddress);
  }
  if(mode=="2"){
  Serial.println("Server started: http://" + apipaddress); 
  }
  if(mode=="3"){
  Serial.println("Server started:");
  Serial.println("Access Point: http://" + apipaddress);
  Serial.println("Client: http://" + cipaddress); 
  }
  Serial.println("================================="); 

  sensors.begin();  //for dallas temp sensor
  checkSensors();   //check all available sensors and save values to RAM
  
  digitalWrite(il,HIGH);   
  delay(1000);   
  digitalWrite(il,LOW);
  
             
 }
    
    void loop() {                                                                                  // Process incoming requests   
                 server.processRequests();
                 delay(100);       
                }
        
        void onClientRequest(ESP8266proConnection* connection,           
        char* buffer, int length, boolean completed) {
          Serial.print(buffer);
          if (strncmp(buffer, "GET ", 4) == 0)   {    
            // We found GET HTTP request
                 
            char* p = strstr(buffer + 4, " ");     
            *p = '\0'; // erase string after path     
            requestPaths[connection->getId()] = (String)((char*)buffer + 4);   }      
            if (completed)   {     String path = requestPaths[connection->getId()];
       //if (path == "/")
       if (path.startsWith("/ajax_switch&nocache="))      
              {
              Serial.println("AJAX request received"); 
    
                if (digitalRead(7)) {
                connection->send(F("<p>Switch 7 state: ON</p>"));
                }
                else {
                connection->send(F("<p>Switch 7 state: OFF</p>"));
                }
               if (digitalRead(8)) {
                connection->send(F("<p>Switch 8 state: ON</p>"));
                }
              else {
                connection->send(F("<p>Switch 8 state: OFF</p>"));
                }
              // read analog pin A2
               int analog_val = analogRead(2);
               connection->send(F("<p>Analog A2: "));
               connection->send(String(analog_val));
               connection->send(F("</p>"));
              }            
       else if (path == "/"){
             connection->send(F(
                "HTTP/1.1 200 OK\r\n\r\n"
                "<!DOCTYPE html>\r\n"
                "<html>\r\n"         
                "<head>\r\n"                  
                "<title>Arduino Web Page</title>\r\n" 
                "<script>\r\n"
                "function GetSwitchAnalogData() {\r\n"
                "nocache = \"&nocache=\" + Math.random() * 1000000;\r\n"
                "var request = new XMLHttpRequest();\r\n"
                "request.onreadystatechange = function() {\r\n"
                "if (this.readyState == 4) {\r\n"
                "if (this.status == 200) {\r\n"
                "if (this.responseText != null) {\r\n"
                "document.getElementById(\"sw_an_data\").innerHTML = this.responseText;\r\n" 
                "}}}}\r\n"
                "request.open(\"GET\", \"ajax_switch\" + nocache, true);\r\n"
                "request.send(null);\r\n"
                "setTimeout('GetSwitchAnalogData()', 1000);\r\n"
                "}\r\n"
                "</script>\r\n"                       
                "</head>\r\n"           
                "<body onload=\"GetSwitchAnalogData()\">\r\n"  
                "<h1>Arduino AJAX Input</h1>\r\n" 
                "<div id=\"sw_an_data\">\r\n"
                "</div>\r\n"     
                "</body>\r\n"          
                "</html>\r\n"));               
               }
      
            
           delay(10);  
           connection->close();   
           } 
       }

                                                      
void checkSensors(){
  
                      sensors.requestTemperatures();     // Send the command to get temperatures
                      t = sensors.getTempCByIndex(0);
                      Serial.print("Temperature = ");   
                      Serial.print(t);   
                      Serial.println(" Celsius");                      
                   } 

void masterswitchOn(){
                      digitalWrite(ms,HIGH); 
                     }
void masterswitchOff(){
                      digitalWrite(ms,LOW); 
                      }       
          
           

String sendData(String command, const int timeout, boolean debug)
     {
    String response = "";
    
    esp8266.print(command); // send the read character to the esp8266
    
    long int time = millis();
    
    while( (time+timeout) > millis())
    {
      while(esp8266.available())
      {
        
        // The esp has data so display its output to the serial window 
        char c = esp8266.read(); // read the next character.
        response+=c;
      }  
    }
    
    if(debug)
    {
      Serial.print(response);
    }
    
    return response;
}                  
    
