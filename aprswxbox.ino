

/*
APRS Weather Station
coded by Yang Lei
20120210
Beta 1.8
*/

#include <Adafruit_BMP085.h>
#include <DHT.h>

#include <Wire.h>
#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>
#define DHTTYPE DHT21
#define DHTPIN 3

//****************************************************************************
//Some network settings below.
byte mac[] = {0xDE, 0xAD, 0xBE, 0x00, 0xFE, 0x00};//Set your MAC Address here. 
char SVR_NAME[] = "hangzhou.aprs2.net";
#define SVR_PORT 14580
//  define your callsign, passcode and location below
#define callsign "yourcall-13"
#define passcode "your passcode"
#define location "0000.00N/00000.00E"

#define VER "2.0"
#define SVR_PROMPT "javAPRSSrvr"
#define SVR_VERIFIED "verified"
int REPORT_INTERVAL = 10;

#define TO_LINE  10000

//****************************************************************************

EthernetClient client;//Create a client

#define DHTTYPE DHT22
#define DHTPIN 3
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;

void setup()
{
  Serial.begin(9600);
  delay(2000);
  Serial.println();
  Serial.println("APRS WX Station");
  bmp.begin();
  initNet();  
}

void loop()
{
  float h =0;
  float t =0;
  float b =0;
  boolean sent = false;

  if ( dht.read() )
  {
    h = dht.readHumidity();
    t = dht.readTemperature(true);
    b = bmp.readPressure();
    
    Serial.print(h);
    Serial.print(" ");
    Serial.print(t);    
    Serial.print(" ");
    Serial.println(b);    
    
    if ( client.connect(SVR_NAME, SVR_PORT) ) 
    { 
      Serial.println("Server connected");
      if ( wait4content(&client, SVR_PROMPT, 11) )
      {
        Serial.println("Prompt ok");
        client.print("user ");
        client.print(callsign);
        client.print(" pass ");
        client.print(passcode);
        client.print(" vers APRSduino ");
        client.println(VER);
        if ( wait4content(&client, SVR_VERIFIED, 8) ) 
        {
          Serial.println("Login ok");
          client.print(callsign);
          client.print(">APRS,TCPIP*:");
          client.print("!");
          client.print(location);
          client.print("_");
          client.print("000/000g000t");
          sendTemp(t);
          client.print("r000p000P000h");
          sendHumi(h);
          client.print("b");
          sendBaro(b);
          client.print("APRSWXBox ");//Sending comments
          client.println(VER);
          Serial.println("Data sent ok");
          delay(5000);
          client.stop();
          Serial.println("Server disconnected");
          delay((long)REPORT_INTERVAL*60L*1000L);
          //delay(30*1000L);
          sent = true;
        }  //  if login
        else 
        {
          Serial.println("Login failed.");
        }
      }  //  if prompt
      else 
      {
        Serial.println("No prompt from the server.");
      }
    }  //  if connect
    else 
    {
      Serial.println("Can not connect to the server.");
    }
    if ( !sent ) 
    {
      initNet();
    }
  }  //  if dht.read()
  else 
  {
    Serial.println("DHT fail");
  }
  delay(5000);
}

void initNet()
{
  Serial.println("Initiating net");
  
  do {
  } while ( Ethernet.begin(mac) == 0 );
  delay(1000);//wait for the Ethernet Shield for 1 second
  Serial.print("Net inited, IP:");
  Ethernet.localIP().printTo(Serial);
  Serial.println();
}

void sendTemp(float t)
{
  int i=t+0.5;
  if ( i<0 ) {
    client.print("-");
    i = -i;
    if ( i>99 ) 
      i=99;
    else if ( i<10 )
      client.print("0");
  } else {
    if ( i<10 )
      client.print("00");
    else if ( i<100 )
      client.print("0");
  }
  client.print(i);
}

void sendHumi(float h)
{
  int i = h+0.5;
  if ( i>99 )  i=99;
  if ( i<10 )
    client.print("0");
  client.print(i);
}

void sendBaro(float b)
{
  int i = b/10;
  if (i > 99999){
    i = 99999;
    client.print(i);}
  if (i > 10000 && i <= 99999){
    client.print(i);}
  if (i < 10000 && i >= 1000){
    client.print(0);
    client.print(i);}
  if (i < 1000 && i > 0){
    client.print(00);
    client.print(i);}
  if(i<0){
    client.print(00000);}
}

boolean wait4content(Stream* stream, char *target, int targetLen)
{
  size_t index = 0;  // maximum target string length is 64k bytes!
  int c;
  boolean ret = false;
  unsigned long timeBegin;
  delay(50);
  timeBegin = millis();
  
  while ( true ) 
  {
    //  wait and read one byte
    while ( !stream->available() ) 
    {
      if ( millis() - timeBegin > TO_LINE )
      {
        break;
      }
      delay(2);
    }
    if ( stream->available() ) {
      c = stream->read();
      //  judge the byte
      if ( c == target[index] )
      {
        index ++;
        if ( !target[index] )  
        // return true if all chars in the target match
        { 
          ret = true;
          break;
        }
      }
      else if ( c>=0 )
      {
        index = 0;  // reset index if any char does not match
      } else //  timed-out for one byte
      {
        break;
      }
    } 
    else  //  timed-out
    { 
      break;
    }
  }
  return ret;
}

