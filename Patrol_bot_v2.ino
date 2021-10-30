#include "WiFiEsp.h"
#include "ThingSpeak.h" 

//Defining IR sensor Values
#define black 1
#define white 0
// Front Ultrasonic Sensor
#define frontTrig 30
#define frontEcho 31
// Right Ultrasonic Sensor
#define rTrig 32
#define rEcho 33
// Left Ultrasonic Sensor
#define lTrig 34
#define lEcho 35
//IR Sensor
#define senR 36
#define senL 37
// Right Motor connections
#define rmspd 2
#define rmp 3
#define rmm 4
// Left Motor connections
#define lmspd 5
#define lmp 6
#define lmm 7


//Wifi Details
char ssid[] = "Aaryansh_2.4GHz_EXT";    //  your network SSID (name) 
char pass[] = "Ansh17@aarya12";   // your network password
int status = WL_IDLE_STATUS; // Initial wifi status

// Creating Wifi module to connect to
WiFiEspClient  client;

//Thingspeak Channel Details
unsigned long myChannelNumber = 1525318;
const char * myWriteAPIKey = "OTJI3J9D2FQPBYS9";
const int field = 1;
int flag=0;
int x;

// Right IR Sensor value
int Rread= 0;

// Left IR Sensor value
int Lread= 0;

//USS distsnces
double ld=0;
double rd=0;
double fd=0;
double fwdduration=0;
double fwddistance=0;
double lduration=0;
double ldistance=0;
double rduration=0;
double rdistance=0;

//Max Distance
double md=0;
double mfd=0;

void setup() 
{
    //Initialize serial for debugging
    Serial.begin(115200);

    // initialize serial for ESP module
    Serial3.begin(115200); 
    
    // initialize Wifi module
    WiFi.init(&Serial3);

    // Initialize ThingSpeak
    ThingSpeak.begin(client);
    
    //connect to wifi
    startwifi();
    
    // Set all the motor control pins to outputs
    pinMode(rmp, OUTPUT);
    pinMode(rmm, OUTPUT);
    pinMode(rmspd, OUTPUT);
    pinMode(lmp, OUTPUT);
    pinMode(lmm, OUTPUT);
    pinMode(lmspd, OUTPUT);
    
    //Set IR Sensor pins to input.(IN ACTUAL IR SENSOR)
    pinMode(senR, INPUT);
    pinMode(senL, INPUT);
  
    //Set Ultrasonic Echo Pins to input
    pinMode(frontEcho, INPUT);
    pinMode(lEcho, INPUT);
    pinMode(rEcho, INPUT);

    //Set Ultrasonic Trigger Pins to output
    pinMode(frontTrig, OUTPUT);
    pinMode(lTrig, OUTPUT);
    pinMode(rTrig, OUTPUT);

    // Turn off motors - Initial state
    digitalWrite(rmp, LOW);
    digitalWrite(rmm, LOW);
    digitalWrite(lmp, LOW);
    digitalWrite(lmm, LOW);
    
    //Speed Control
    analogWrite(rmspd , 255);
    analogWrite(lmspd , 255);

   //Callibration Phase
    Serial.println("Callibration Phase Initiate....");
    ld=getLeftDistance();
    rd=getRightDistance();
    if(ld>rd)
      md=ld;
    else
      md=rd;
    fwd();
    while(digitalRead(senR)!=black && digitalRead(senL)!=black)
    {
      ld=getLeftDistance();
      rd=getRightDistance();
      fd=getFwdDistance();
      if(digitalRead(senR)==black && digitalRead(senL)==black)
      {
        mfd=fd;
      }
      if(ld>rd && ld<md)
      {
        md=ld;
      }
      else if (rd>ld && rd<md)
      {
        md=rd;
      }
      else continue;
    }
    md-=5;
    Serial.println("Callibration Complete\nMax Detection Distance in cm:");
    Serial.print(md);
    stopcar();
    delay(1000);
}

void loop() 
{
  if (WiFi.status() == WL_CONNECTED)
  {
    ld=getLeftDistance();
    rd=getRightDistance();
    if (ld>rd)
    {
      left();
      while(digitalRead(senL) != black) 
      {
        left();
      }
      stopcar();
      straighten();
      while (digitalRead(senR)!=black && digitalRead(senL)!=black)
      {
        straighten();
        ld=getLeftDistance();
        fd=getFwdDistance();
        if (ld<md || fd<mfd+1 || fd<mfd-1)
        {
          flag=1;
          alert();
        }
        else continue;    
      }
    }
    if (rd>ld)
    {
      right();
      while(digitalRead(senR) != black) 
      {
        right();
      }
      stopcar();
      straighten();
      while (digitalRead(senR)!=black && digitalRead(senL)!=black)
      {
        straighten();
        rd=getRightDistance();
        fd=getFwdDistance();
        if (rd<md-1 || fd<mfd+1 || fd<mfd-1)
        {
          flag=1;
          alert();
        }
        else continue;    
      }
    }
  }
  else
  {
    startwifi();
  }
}

void alert()
{
  // Write to ThingSpeak.
  int x = ThingSpeak.writeField(myChannelNumber, field, flag, myWriteAPIKey);
  if(x == 200){
    Serial.println("Channel update successful.");
  }
  else{
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
  flag=0;  
}

void startwifi()
{
  Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while(WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network.
      Serial.print(".");
      delay(5000);     
    }
}
void fwd() //All wheels spinning in Anti - Clockwise direction
{
    digitalWrite(rmp, HIGH);
    digitalWrite(rmm, LOW);
    digitalWrite(lmp, HIGH);
    digitalWrite(lmm, LOW);
}
void stopcar() //All wheels stop spinning
{
    digitalWrite(rmp, LOW);
    digitalWrite(rmm, LOW);
    digitalWrite(lmp, LOW);
    digitalWrite(lmm, LOW);
}
void left() //Right wheels spin Anti - Clockwise, left wheels spin Clockwise
{
    digitalWrite(rmp, HIGH);
    digitalWrite(rmm, LOW);
    digitalWrite(lmp, LOW);
    digitalWrite(lmm, HIGH);
}
void right() //left wheels spin Anti - Clockwise, right wheels spin Clockwise
{
    digitalWrite(rmp, LOW);
    digitalWrite(rmm, HIGH);
    digitalWrite(lmp, HIGH);
    digitalWrite(lmm, LOW);
}
void straighten()
{
  if (digitalRead(senR)!= white)
  {
    while (digitalRead(senR)!= white && digitalRead(senL)!= white)
    {
      right();
    }
    fwd();
   }
   else if (digitalRead(senL)!= white)
   {
     while (digitalRead(senR)!= white && digitalRead(senL)!= white)
     {
       left();
     }
       fwd();
     }
}

double getFwdDistance()
{ 
  digitalWrite (frontTrig, LOW); //write trigger pin low to get a clean high pulse
  delayMicroseconds(2); //Low for 2 microseconds
  digitalWrite (frontTrig, HIGH);// Set to high for 10 microseconds
  delayMicroseconds(10);
  digitalWrite (frontTrig, LOW);// set back to low
  fwdduration = pulseIn(frontEcho, HIGH); //get the time for which echopin was high (US ray did not bounce back)
  fwddistance = fwdduration * 0.034 / 2; //s=vt (v= speed of sound, t=duration) we multiply by 2 because time corresponds to US ray travellin the distance twice
  return fwddistance; //return value of distance calculated
}

double getRightDistance()
{ 
  digitalWrite (rTrig, LOW);
  delayMicroseconds(2);
  digitalWrite (rTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite (rTrig, LOW);
  rduration = pulseIn(rEcho, HIGH); 
  rdistance = rduration * 0.034 / 2;
  return rdistance;
}

double getLeftDistance()
{ 
  digitalWrite (lTrig, LOW);
  delayMicroseconds(2);
  digitalWrite (lTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite (lTrig, LOW);
  lduration = pulseIn(lEcho, HIGH); 
  ldistance = lduration * 0.034 / 2; 
  return ldistance; 
}
