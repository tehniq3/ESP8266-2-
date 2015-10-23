/*
Start up a very mini web server
 ******* This requires the Mega 2560 board *****
 https://petestechprojects.wordpress.com/2014/12/19/leds-control-through-esp8266-arduino-web-page/
 change for Serial2 on Arduino Mega by niq_ro from http://www.tehnic.go.ro
 & http://nicuflorica.blospot.ro
 & http://arduinotehniq.blogspot.com/
 */

#define TIMEOUT      5000 // mS
#define GREENLED     4
#define REDLED       5
#define BLUELED      3
#define ESP_RESET    22
#define BAUDRATE     9600

//*** Literals and vars used in the message queue ***
#define QUE_SIZE     8
#define HTML_REQUEST  1
#define FAVICON_REQUEST  2
#define QUE_INTERVAL    500
int QueIn=0;
int QueOut=0;
int CommandQue[QUE_SIZE];
int CommandQueIPD_CH[QUE_SIZE];
//String CommandQue[QUE_SIZE];
//String WaitQue[QUE_SIZE];
float LastCommandSent=0;
float LastQueEntered=0;
String CIPSendString="";


//*** States of the LEDs ***
boolean RED_State = false;
boolean GREEN_State = false;
boolean BLUE_State = false;


//*** Used for server stat section ***
int NumberOfResets=0;
int NumberServed=0;
int NumberIconReqs=0;
int NumberLEDRequest=0;

void setup()  
{
  pinMode(REDLED,OUTPUT); 
  pinMode(GREENLED,OUTPUT);
  pinMode(BLUELED,OUTPUT);
  pinMode(ESP_RESET,OUTPUT);
  
  digitalWrite(ESP_RESET,HIGH);
  Serial.begin(115200);         // Manual interface port
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


//***** Formulate command string and send *****
void ProcessQueCommand(int QueOut){

String HTTPHeader;
String HTMLCode;  

  switch (CommandQue[QueOut]){
    case HTML_REQUEST:
      float CIPSendTime;
      //*** Build the HTML code ***
      //Note: Need this first since HTTP Header needs length of content
      HTMLCode   =    "<HTML>"
                      "<HEAD><TITLE>Pete's Mini8266 Server</TITLE>"
                      "<BODY><H1>Welcome to Pete's ESP8266 \"hacking\" project</H1>"
                         "<form action=\"\" method=\"post\">"
                         "<fieldset>"
                            "<legend>Red LED State</legend>"
                            "<input type=\"radio\" name=\"RedLEDState\" value=\"RED_ON\"> ON"
                            "<input type=\"radio\" name=\"RedLEDState\" value=\"RED_OFF\" checked=\"checked\"> OFF<br>"
                         "</fieldset>"
                         "<fieldset>"
                            "<legend>Green LED State</legend>"
                            "<input type=\"radio\" name=\"GreenLEDState\" value=\"GREEN_ON\"> ON"
                            "<input type=\"radio\" name=\"GreenLEDState\" value=\"GREEN_OFF\" checked=\"checked\"> OFF<br>"
                         "</fieldset>"
                         "<fieldset>"
                            "<legend>Blue LED State</legend>"
                            "<input type=\"radio\" name=\"BlueLEDState\" value=\"BLUE_ON\"> ON"
                            "<input type=\"radio\" name=\"BlueLEDState\" value=\"BLUE_OFF\" checked=\"checked\"> OFF<br>"
                         "</fieldset>"                         
                         "<input type=\"submit\" value=\"Submit\">"
                         "</form>"
                      "<BR><BR>"
                      "<HR>"
                      "<H2>Server Stats</H2>"  
                      "<B>Number Servered: </B>" + String(NumberServed) +
                      "<BR><B>Number LED Changes: </B>" + String(NumberLEDRequest) +
                      "<BR><B>Number of Resets: </B>" + String(NumberOfResets) +
                      "<BR><B> Up time: </B>" + String(millis()) + 
                      "</BODY></HTML>  ";
      //*** Build HTTP Header ***
      HTTPHeader = "HTTP/1.0 200 OK \r\n"
                      "Date: Fri, 31 Dec 1999 23:59:59 GMT \r\n"
                      "Content-Type: text/html\r\n"
                      "Content-Length: " + String(HTMLCode.length()) + "\r\n";

      //*** Send the CIPSEND command for the HTTPHeader string ***
      Serial2.println("AT+CIPSEND=" + String(CommandQueIPD_CH[QueOut]) + ","+  String(HTTPHeader.length()+2));
      CIPSendTime = millis();
      while ((millis() - CIPSendTime)<QUE_INTERVAL){
        //*** Handle each character that is coming in from the ESP8266 ***
        //*** Need to do this here also to not miss any incoming messages ***
        while(Serial2.available()) 
        {
          IncomingChar (Serial2.read ());
        }  
      }
      if (QueOut==QueIn) break;        // There must have been a reset

      //*** Send the HTTPHeader String ***
      Serial2.println(HTTPHeader);
      CIPSendTime = millis();
      while ((millis() - CIPSendTime)<QUE_INTERVAL){
        //*** Handle each character that is coming in from the ESP8266 ***
        //*** Need to do this here also to not miss any incoming messages ***
        while(Serial2.available()) 
        {
          IncomingChar (Serial2.read ());
        }
      }  
      if (QueOut==QueIn) break;        // There must have been a reset


      //*** Send the CIPSEND command for the HTML page ***       
      Serial2.println("AT+CIPSEND=" + String(CommandQueIPD_CH[QueOut]) + ","+  String(HTMLCode.length()+2));
      CIPSendTime = millis();
      while ((millis() - CIPSendTime)<QUE_INTERVAL){
        //*** Handle each character that is coming in from the ESP8266 ***
        //*** Need to do this here also to not miss any incoming messages ***
        while(Serial2.available()) 
        {
          IncomingChar (Serial2.read ());
        }
      }  
      if (QueOut==QueIn) break;        // There must have been a reset

      //*** Send out the HTML code ***
      Serial2.println(HTMLCode);
      CIPSendTime = millis();
      while ((millis() - CIPSendTime)<QUE_INTERVAL){
        //*** Handle each character that is coming in from the ESP8266 ***
        //*** Need to do this here also to not miss any incoming messages ***
        while(Serial2.available()) 
        {
          IncomingChar (Serial2.read ());
        }
      }  
      break;
      
    case FAVICON_REQUEST:
      //*** Send the CIPSEND command for Close ***       
      Serial2.println("AT+CIPCLOSE=" + String(CommandQueIPD_CH[QueOut]));
      break;
      
    default:
      //Nothing here yet  
      Serial.println("Should never see this");
  }
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
  if (InLine.startsWith("RedLEDState=")){
    
    ParseLEDControl(InLine);
    SetLEDStates();
  }
  if (InLine.indexOf("GET / ") != -1) {
    CommandQue[QueIn++]=HTML_REQUEST;
    NumberServed++;
  }
  if (InLine.indexOf("POST / ") != -1) {
    CommandQue[QueIn++]=HTML_REQUEST;
    NumberLEDRequest++;
  }
  if (InLine.indexOf("favicon.ico") != -1) {  
    CommandQue[QueIn++]=FAVICON_REQUEST;
    NumberIconReqs++;
  }
  if (InLine.indexOf("System Ready") != -1) {
    Serial.println("The ESP8266 Reset for some reason");
    InitWifiModule();
    NumberOfResets++;
  }
  if (InLine.indexOf("busy s...") != -1) {
    Serial.println("dead with busy s...   HW Reset");
    QueOut=QueIn;                      //Clear out the command que
    digitalWrite(ESP_RESET,LOW);
    delay(500);
    digitalWrite(ESP_RESET,HIGH);    
    // Note: Parser should see the reset and start the InitWifiModule routine
  }
  if (InLine.indexOf("link is not") != -1) {
    Serial.println("AHAHAHAHAHAAHAHAHAHAHAH***********************************");
  }  
  
  if (QueIn==QUE_SIZE){
    Serial.println("Resetting QueIn");
    QueIn=0;
  }

  LastQueEntered = millis();

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
