/*
original material from
https://petestechprojects.wordpress.com/2014/12/10/mini-server-on-arduino-esp8266/
Start up a very mini web server 
******* This requires the Mega 2560 board *****

adapted sketch by niq_ro from http://www.tehnic.go.ro
http://nicuflorica.blogspot.ro/
http://arduinotehniq.blogspot.com/
 
 */

#include <DHT.h>

#define DHTPIN 8     // what pin we're connected the DHT output
#define DHTTYPE DHT11   // DHT 11 
//#define DHTTYPE DHT22   // DHT 22 
DHT dht(DHTPIN, DHTTYPE);


#define TIMEOUT 5000 // mS
#define GREENLED 4
#define REDLED 5
#define BLUELED 3

boolean RED_State = false;
boolean GREEN_State = false;
boolean BLUE_State = false;

void setup() 
{
 dht.begin();   
  
 pinMode(REDLED,OUTPUT); 
 pinMode(GREENLED,OUTPUT);
 pinMode(BLUELED,OUTPUT);

 Serial.begin(115200);
 Serial2.begin(115200);

 int CommandStep = 1;

 //This initializes the Wifi Module as a server 
 BlinkLED(REDLED,CommandStep,50); 
 SendCommand("AT+RST", "Ready", true);
 BlinkLED(GREENLED,CommandStep,50);
 CommandStep++;

 BlinkLED(REDLED,CommandStep,50); 
 SendCommand("AT+GMR", "OK", true);
 BlinkLED(GREENLED,CommandStep,50);
 CommandStep++;

 delay(3000);

 BlinkLED(REDLED,CommandStep,50); 
 SendCommand("AT+CIFSR", "OK", true);
 BlinkLED(GREENLED,CommandStep,50);
 CommandStep++;


 BlinkLED(REDLED,CommandStep,50); 
 SendCommand("AT+CIPMUX=1","OK",true);
 BlinkLED(GREENLED,CommandStep,50);
 CommandStep++;

 BlinkLED(REDLED,CommandStep,50); 
 SendCommand("AT+CIPSERVER=1,80","OK",true);
 BlinkLED(GREENLED,CommandStep,50);

 digitalWrite(GREENLED,HIGH);
 //----------------------------------------------------
}

void BlinkLED(int LEDPin, int NumberOfBlinks, int OnDuration)
{
 for (int x=1; x <= NumberOfBlinks ; x ++){
 digitalWrite(LEDPin,HIGH);
 delay(OnDuration);
 digitalWrite(LEDPin,LOW);
 delay(OnDuration); 
 }
}


void loop(){
 String IncomingString="";
 char SingleChar;
 boolean StringReady = false;

//int h = dht.readHumidity();
//float t = dht.readTemperature();

 //*** Handle each character that is coming in from the ESP8266 ****
 while(Serial2.available()) 
 {
 IncomingChar (Serial2.read ());
 } 
 
 while(Serial.available())
 {
 Serial2.write(Serial.read());
 }

}



void ProcessCommand (const char * data)
{
 Serial.println (data);

 String IncomingLine = String(data);

 if (IncomingLine.indexOf("GET / ") != -1) {
 char ClientIdChar = IncomingLine.charAt(5);
 echoFind("OK"); 
 delay(100);
 int h = dht.readHumidity();
 float t = dht.readTemperature(); 
 SendHTML(String(ClientIdChar), h, t);
 }

} 


void SendHTML(String ClientId, int has, float te){
 Serial.println("Someone requested a HTML Page on Client Id:" + ClientId );
// client.println("<meta http-equiv=\"refresh\" content=\"5\" URL='http://tehniq.go.ro:8081/'\" >");
 SendClient("<HTML><HEAD><meta http-equiv=\"refresh\" content=\"5\"><TITLE>Pete's Mini8266 Server</TITLE></HEAD>"

 "<BODY><center><H1>Welcome to niq_ro's ESP8266 page</H1>",ClientId);
 SendClient("<BR><BR><h2>humidity = ",ClientId);
 SendClient(String(has),ClientId);
 SendClient(" % RH <br> temperature = ",ClientId);

int te2 = te*10;
if (te2 > 0) SendClient("+",ClientId);
if (te2 < 0)
    {
    te2 = -te2;
    SendClient("+",ClientId);
    }
 int te21 = te2/10;
 int te22 = te2 - 10*te21;
 SendClient(String(te21),ClientId);
 SendClient(",",ClientId);
 SendClient(String(te22),ClientId);
 SendClient("<sup>o</sup>C <br><p><p> Up time: ",ClientId);
 SendClient(String(millis()/1000),ClientId);
 SendClient(" second(s)...",ClientId);
 SendClient("</BODY></HTML>",ClientId);
 SendCommand("AT+CIPCLOSE="+ ClientId,"OK",true);
}

void SendClient(String ToClientString,String ClientID){
 SendCommand("AT+CIPSEND=" + ClientID + ","+ (ToClientString.length() +2) ,">",false);
 SendCommand(ToClientString,"OK",false);
}



void IncomingChar (const byte InChar)
{
 static char InLine [500]; //Hope we don't get more than that in one line
 static unsigned int Position = 0;

 switch (InChar)
 {
 case '\r': // Don't care about carriage return so throw away.
 break;
 
 case '\n': 
 InLine [Position] = 0; 
 ProcessCommand (InLine);
 Position = 0; 
 break;

 default:
 InLine [Position++] = InChar;
 } 

} 


//************** This section specific to simple AT command with no need to parse the response ************
// Warning: Will drop any characters coming in while waiting for the indicated response.
boolean SendCommand(String cmd, String ack, boolean halt_on_fail)
{
 Serial2.println(cmd); // Send command to module

 // Otherwise wait for ack.
 if (!echoFind(ack)) // timed out waiting for ack string
 if (halt_on_fail)
 errorHalt(cmd+" failed");// Critical failure halt.
 else
 return false; // Let the caller handle it.
 return true; // ack blank or ack found
}

// Read characters from WiFi module and echo to serial until keyword occurs or timeout.
boolean echoFind(String keyword)
{
 byte current_char = 0;
 byte keyword_length = keyword.length();

 // Fail if the target string has not been sent by deadline.
 long deadline = millis() + TIMEOUT;
 while(millis() < deadline)
 {
 if (Serial2.available())
 {
 char ch = Serial2.read();
 Serial.write(ch);
 if (ch == keyword[current_char])
 if (++current_char == keyword_length)
 {
 Serial.println();
 return true;
 }
 }
 }
 return false; // Timed out
}

// Print error message and loop stop.
void errorHalt(String msg)
{
 Serial.println(msg);
 Serial.println("HALT");
 digitalWrite(REDLED,HIGH);
 digitalWrite(GREENLED,HIGH);
 while(true){
 };
}
