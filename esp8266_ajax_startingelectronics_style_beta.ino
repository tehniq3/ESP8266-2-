/*
// http://startingelectronics.org/tutorials/arduino/ethernet-shield-web-server-tutorial/AJAX-read-switches-analog/
// https://forum.arduino.cc/index.php?topic=353502.0
Start up a very mini web server 
 https://petestechprojects.wordpress.com/2014/12/19/leds-control-through-esp8266-arduino-web-page/
 change for Serial2 on Arduino Mega by niq_ro from http://www.tehnic.go.ro
 & http://nicuflorica.blospot.ro
 & http://arduinotehniq.blogspot.com/
 */
 
String SSID = "SSID";
String PASSWORD = "pasword";

#define TIMEOUT      5000 // mS
#define GREENLED     9
#define REDLED       11
#define BLUELED      10
#define ESP_RESET    22
#define BAUDRATE     115200

//*** Literals and vars used in the message queue ***
#define QUE_SIZE     8
#define HTML_REQUEST  1
#define FAVICON_REQUEST  2
#define QUE_INTERVAL    300
int QueIn=0;
int QueOut=0;
int CommandQue[QUE_SIZE];
int CommandQueIPD_CH[QUE_SIZE];
//String CommandQue[QUE_SIZE];
//String WaitQue[QUE_SIZE];
float LastCommandSent=0;
float LastQueEntered=0;
String CIPSendString="";
String HTTPHeader;
String HTTPHeader2;
String HTMLCode1;  
String HTMLCode2;
String HTMLCode4;


//*** States of the LEDs ***
boolean RED_State = false;
boolean GREEN_State = false;
boolean BLUE_State = false;

int nou = 0;

//*** Used for server stat section ***
int NumberOfResets=0;
int NumberServed=0;
int NumberIconReqs=0;
int NumberLEDRequest=0;
int NumberBusyS=0;

void setup()  
{
  pinMode(REDLED,OUTPUT); 
  pinMode(GREENLED,OUTPUT);
  pinMode(BLUELED,OUTPUT);
  pinMode(ESP_RESET,OUTPUT);
  
  digitalWrite(ESP_RESET,HIGH);
  Serial.begin(57600);         // Manual interface port
  Serial2.begin(BAUDRATE);        // Port to ESP8266 Wifi Module

  InitWifiModule();
}

void loop(){
  String IncomingString="";
  char SingleChar;
  boolean StringReady = false;


  //*** Handle each character that is coming in from the ESP8266 ****
  while(Serial2.available()) 
  {
    IncomingChar (Serial2.read ());
  }  
  
  //*** Allow manual commands to be given during run time. ***
  while(Serial.available())
  {
    Serial2.write(Serial.read());
  }
  
  if (QueIn != QueOut){
    if ((millis()-LastCommandSent > QUE_INTERVAL) && (millis()-LastQueEntered > QUE_INTERVAL)){
      ProcessQueCommand(QueOut);
      if(QueOut!=QueIn){
        QueOut++;
      }
      LastCommandSent = millis();
      if (QueOut==QUE_SIZE){
        QueOut=0;
        Serial.println("Resetting QueOut");
      }
    }else{
      //Serial.println("waiting to send");
    }
  }
  
  ///*** Setting the LED states to what are in the state vars for the case of a ESP8266 reset event ***
  SetLEDStates(); 
  
}



boolean WaitForKey(String keyword){
  byte current_char   = 0;
  byte keyword_length = keyword.length();
  long deadline = millis() + TIMEOUT;

  while(millis() < deadline)
  {
    if (Serial2.available())
    {
      char ch = Serial2.read();
      IncomingChar (ch);                                    // Keep processing incoming commands
      if (ch == keyword[current_char])
        if (++current_char == keyword_length)
        {
          Serial.println("Found keyword: " + keyword);
         
          //***  ESP8266 seems to choke if things are sent too soon ***
          while(millis()-LastCommandSent < QUE_INTERVAL){
              if(Serial2.available()) 
              {
                IncomingChar (Serial2.read ());
              }  
          }
          return true;
        }
    }
  }
  Serial.println("Timed out on keyword: " + keyword);
  return false;
}
  


boolean SendCIPChunk(String StringToSend,int IPD_CH){
  
  Serial2.println("AT+CIPSEND=" + String(IPD_CH) + "," + String(StringToSend.length()+2));
  LastCommandSent = millis();
  WaitForKey(">");                    // Don't care if timed out for now.  Will just continue for now.
  
  if (QueOut==QueIn) return false;    // This only can happen if ESP8266 reset while waiting to send string 
 
  Serial2.println(StringToSend);
  LastCommandSent = millis();
  WaitForKey("SEND OK");
  if (QueOut==QueIn) return false;    // This only can happen if ESP8266 reset while waiting to send string 

  return true;
}


  

//***** Formulate command string and send *****
void ProcessQueCommand(int QueOut){

  switch (CommandQue[QueOut]){
    case HTML_REQUEST:
      BuildHTMLPage();
      if(!SendCIPChunk(HTTPHeader,CommandQueIPD_CH[QueOut])){   // Send the CIPSEND command for the HTTPHeader string 
        break;                                                  // returned false because there was a ESP8266 reset
      }    

      if(!SendCIPChunk(HTMLCode1,CommandQueIPD_CH[QueOut])){     // Send the CIPSEND command for HTMLCode1 
        break;                                                  // returned false because there was a ESP8266 reset
      }    

      if(!SendCIPChunk(HTMLCode2,CommandQueIPD_CH[QueOut])){     // Send the CIPSEND command for HTMLCode2 
        break;                                                  // returned false because there was a ESP8266 reset
      }    
      break;
      
    case FAVICON_REQUEST:
      //*** Send the CIPSEND command for Close ***       
      Serial2.println("AT+CIPCLOSE=" + String(CommandQueIPD_CH[QueOut]));
      WaitForKey("OK");
      break;
      
    default:
      //Nothing here yet  
      Serial.println("Should never see this");
  }
}  

//*** Builds the HTTP header + HTML code to send out ***
void BuildHTMLPage(){
    
  //Note: Need this first since HTTP Header needs length of content
  HTMLCode1  =  "<html>\r\n"         
                "<head>\r\n"                  
                "<title>ESP8266 Arduino Web Page using Ajax</title>\r\n" 
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
                "setTimeout('GetSwitchAnalogData()', 10000);\r\n"
                "}\r\n"
                "</script>\r\n"                       
                "</head>\r\n";
  HTMLCode2  =   "<body onload=\"GetSwitchAnalogData()\">\r\n"  
                "<h1>ESP8266 Arduino Web Page using Ajax</h1>\r\n" 
                "<div id=\"sw_an_data\">\r\n"
                "</div>\r\n"     
                "</body>\r\n"          
                "</html>\r\n";
                     
  //*** Build HTTP Header ***
  HTTPHeader = "HTTP/1.0 200 OK \r\n"
                  "Date: Fri, 31 Dec 1999 23:59:59 GMT \r\n"
                  "Content-Type: text/html\r\n"
                  "Content-Length: " + String(HTMLCode1.length() + HTMLCode2.length()) + "\r\n";
}



//***** Group up characters until end of line ****
void IncomingChar (const byte InChar)
{
  static char InLine [500];    //Hope we don't get more than that in one line
  static unsigned int Position = 0;

  switch (InChar)
  {
  case '\r':   // Don't care about carriage return so throw away.
    break;
    
  case '\n':   
    InLine [Position] = 0;  
    ProcessCommand(String(InLine));
    Position = 0;  
    break;

  default:
      InLine [Position++] = InChar;
  }  
} 


void ProcessCommand(String InLine){
  Serial.println("InLine: " + InLine);
  
  if (InLine.startsWith("+IPD,")){
    CommandQueIPD_CH[QueIn]=InLine.charAt(5)-48;    // The value returned is ASCII code.  48 is ASCII code for 0
    Serial.println("******** IPD found: " + String(CommandQueIPD_CH[QueIn]));
  }
  if (InLine.indexOf("LEDFormAction=Set+LED+States") != -1){
    ParseLEDControl(InLine);
    SetLEDStates();
    NumberLEDRequest++;
  }
  if (InLine.indexOf("LEDFormAction=Get+LED+States") != -1){
    //Do nothing sinc the "POST / " command already is sending HTML page with latest LED states
  }

//if (InLine.indexOf("ajax_switch&nocache=") != -1){
if ((InLine.indexOf("ajax_switch&nocache=") != -1) and (nou == 0)){
  Serial.println("AJAX request received"); 
  Serial.println("-----------------------");
//  Serial2.print(String(millis()/1000)); 
//  Serial2.println("seconds------------------</p>");


//  BuildHTMLreq();
  Serial.print("Que = ");
  Serial.println(String(CommandQueIPD_CH[QueIn]));
int analog_val = analogRead(A2);                
  
  //Note: Need this first since HTTP Header needs length of content
  HTMLCode4  =  "<p><p>"         
                " Auuuuuuuuuuuuu !  ";
//                "Analog input is ";          
//  HTMLCode4  = HTMLCode4  + String(analog_val);
//  HTMLCode4  = HTMLCode4  + "<p><p>      ";
//  "\r\n";

//Serial2.print(String(millis()/1000));
//SendCIPChunk(HTMLCode4,CommandQueIPD_CH[QueIn]);
Serial2.println("AT+CIPSEND=" + String(CommandQueIPD_CH[QueIn]) + "," + String(HTMLCode4.length()+2));
 Serial2.println(HTMLCode4);
SendCommand("AT+CIPCLOSE=" + String(CommandQueIPD_CH[QueIn]), "OK", false);  // server mode

//  Serial2.println("AT+CIPCLOSE=" + String(CommandQueIPD_CH[QueIn]));

/*
SendCIPChunk(HTMLCode4,CommandQueIPD_CH[QueIn]);
//  Serial2.println("AT+CIPSEND=" + String(CommandQueIPD_CH[QueIn]) + "," + String(HTMLCode4.length()+2));
//  WaitForKey(">");   
 // Serial2.println(HTMLCode4);
//  WaitForKey("SEND OK");     
 Serial2.println("AT+CIPCLOSE=" + String(CommandQueIPD_CH[QueIn]));
 WaitForKey("OK");     
 */
 nou = 1; 
}

  if (InLine.indexOf("GET / ") != -1) {
    CommandQue[QueIn++]=HTML_REQUEST;
    NumberServed++;
  }
  if (InLine.indexOf("POST / ") != -1) {
    CommandQue[QueIn++]=HTML_REQUEST;
    NumberServed++;
  }
  if (InLine.indexOf("favicon.ico") != -1) {  
    CommandQue[QueIn++]=FAVICON_REQUEST;
    NumberIconReqs++;
  }
  
  
  
  if (InLine.indexOf("System Ready") != -1) {
    Serial.println("The ESP8266 Reset for some reason");
    digitalWrite(ESP_RESET,LOW);
    delay(500);
    digitalWrite(ESP_RESET,HIGH);    
    QueOut=QueIn;                      //Clear out the command que
    InitWifiModule();
    NumberOfResets++;
  }
  if (InLine.indexOf("busy s...") != -1) {
    Serial.println("dead with busy s...   HW Reset");
    QueOut=QueIn;                      //Clear out the command que
    digitalWrite(ESP_RESET,LOW);
    delay(500);
    digitalWrite(ESP_RESET,HIGH);    
    NumberBusyS++;
    // Note: Parser should see the reset and start the InitWifiModule routine
  }
  if (InLine.indexOf("link is not") != -1) {
    Serial.println("AHAHAHAHAHAAHAHAHAHAHAH***********************************");
    QueOut=QueIn;                      //Clear out the command que so no more sending CIPSEND
  }  
  
  if (QueIn==QUE_SIZE){
    Serial.println("Resetting QueIn");
    QueIn=0;
  }

  LastQueEntered = millis();
nou = 0;
}

//*** This parses out the LED control strings and sets the appropriate state var *** 
void ParseLEDControl(String InLine){
  if (InLine.indexOf("RED_OFF")!=-1) RED_State=false;
  if (InLine.indexOf("GREEN_OFF")!=-1) GREEN_State=false;
  if (InLine.indexOf("BLUE_OFF")!=-1) BLUE_State=false;

  if (InLine.indexOf("RED_ON")!=-1) RED_State=true;
  if (InLine.indexOf("GREEN_ON")!=-1) GREEN_State=true;
  if (InLine.indexOf("BLUE_ON")!=-1) BLUE_State=true;
}

//*** This will set the LED control to the current state stored in the state vars ***
void SetLEDStates(){
  if (RED_State) digitalWrite(REDLED,HIGH);
  else digitalWrite(REDLED,LOW);

  if (GREEN_State) digitalWrite(GREENLED,HIGH);
  else digitalWrite(GREENLED,LOW);
  
  if (BLUE_State) digitalWrite(BLUELED,HIGH);
  else digitalWrite(BLUELED,LOW);
}  
  
  
//***** This initializes the Wifi Module as a server  *****
void InitWifiModule(){
  int CommandStep = 1;
  BlinkLED(REDLED,CommandStep,50);  
  SendCommand("AT+RST", "Ready", true);
  BlinkLED(GREENLED,CommandStep,50);
  CommandStep++;

  BlinkLED(REDLED,CommandStep,50); 
//  SendCommand("AT+GMR", "OK", true);
//  SendCommand("AT+CWMODE=2", "OK", false);  // AP mode
  SendCommand("AT+CWMODE=1", "OK", false);  // server mode
  BlinkLED(GREENLED,CommandStep,50);
  CommandStep++;

 delay(3000);

BlinkLED(REDLED,CommandStep,50); 
SendCommand("AT+CWJAP=\"" + SSID + "\",\"" + PASSWORD + "\"", "OK", false);
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

//************** This section specific to simple AT command with no need to parse the response ************
//  Warning: Will drop any characters coming in while waiting for the indicated response.
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
  byte current_char   = 0;
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
  return false;  // Timed out
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


