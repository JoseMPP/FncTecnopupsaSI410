//Azure IoT Hub + DHT11 + NodeMCU ESP8266 Experiment Done By Prasenjit Saha
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include "DHT.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>
// WiFi settings
const char* ssid = "FLIA PIZARRO";
const char* password = "3276856xyz";
const char * headerKeys[] = {"ETag"};
const size_t numberOfHeaders = 1;
//Banderas
String bandera_servo="true";
String bandera_alarma="false";
String verdadero="true";
String falso="false";
//Mensaje
String mensaje="on";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "south-america.pool.ntp.org", -14400, 60000);

#define DHTPIN 5
// Digital pin connected to the DHT sensor

//Azure IoT Hub
const String AzureIoTHubURI="https://IoTHub-TecnoUpsa.azure-devices.net/devices/esp-01/messages/events?api-version=2020-03-13";
const String AzureIoTHubURIGet="https://IoTHub-TecnoUpsa.azure-devices.net/devices/esp-01/messages/deviceBound?api-version=2020-03-13";
const String AzureIoTHubURIDel="https://IoTHub-TecnoUpsa.azure-devices.net/devices/esp-01/messages/deviceBound/{etag}?api-version=2020-03-13";
//openssl s_client -servername myioteventhub.azure-devices.net -connect myioteventhub.azure-devices.net:443 | openssl x509 -fingerprint -noout //
const String AzureIoTHubFingerPrint="9C:46:08:5D:55:5C:F7:83:79:0A:03:BD:DE:F4:54:F2:C9:E6:FF:9C"; 
//az iot hub generate-sas-token --device-id {YourIoTDeviceId} --hub-name {YourIoTHubName} 
const String AzureIoTHubAuth="SharedAccessSignature sr=IoTHub-TecnoUpsa.azure-devices.net%2Fdevices%2Fesp-01&sig=Y%2FT2UbTQJ4oewgEQyWrr9bVJYRgtfxgHPx6V6%2BvQQoA%3D&se=1623479200";

#define DHTTYPE DHT11  

DHT dht(DHTPIN, DHTTYPE);
////Servo Motor
Servo servo1;
int Pmin=600;
int Pmax=2750;
//Buzzer
const int Buzzer=0;
//leds
const int GreenPin=2;
const int RedPin=14;

void setup() {
  Serial.begin(9600);
  Serial.println(F("DHTxx test!"));

  dht.begin();
  Serial.println("ESP8266 starting in normal mode");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Print the IP address
  Serial.println(WiFi.localIP());
  timeClient.begin();
  //Servo motor
  servo1.attach(4,Pmin,Pmax);
  //Buzzer
  pinMode(Buzzer, OUTPUT);
  //Leds
  pinMode(GreenPin, OUTPUT);
  pinMode(RedPin, OUTPUT);
}

void loop() {
  //Iniciar Cinta
  if(bandera_alarma==falso){
    digitalWrite(RedPin , LOW);
    bandera_servo=verdadero;
    Serial.println("Funcionamiento Correcto");
    noTone(Buzzer);}
  else{
    digitalWrite(GreenPin , LOW);
    bandera_servo=falso;
    Serial.println("ALERTA!!ALERTA!!ALERTA!!!");
    tone(Buzzer, 1523);
    digitalWrite(RedPin , HIGH);
    delay(700);
    noTone(Buzzer);
    digitalWrite(RedPin , LOW);
    delay(700);
    tone(Buzzer, 1523);
    digitalWrite(RedPin , HIGH);
    delay(700);
    noTone(Buzzer);
    digitalWrite(RedPin , LOW);
    delay(700);
    tone(Buzzer, 1523);
    digitalWrite(RedPin , HIGH);
  
   
    }
  if((bandera_servo==verdadero)&&(mensaje=="on")){
    servo1.write(180);
    digitalWrite(GreenPin , HIGH);
    }
  else{
    servo1.write(0);}
    
  
  //Sacar la hora y darle formato
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();
  String formattedTime = timeClient.getFormattedTime();
  struct tm *ptm = gmtime ((time_t *)&epochTime);

  int monthDay = ptm->tm_mday;
  String m;
  int currentMonth = ptm->tm_mon+1;
  if(currentMonth<10)
  {m="0"+String(currentMonth);}
  else
  {m=String(currentMonth);}

  int currentYear = ptm->tm_year+1900;

  String currentDate = String(currentYear) + "-" + m + "-" + String(monthDay)+"T"+formattedTime;
  
  // Wait a few seconds between measurements.
  delay(5000);
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  String b=bandera_alarma;
  String ms=bandera_servo;
  String PostData="{\"NameDevice\":\"ESP-01\",\"EventDateTime\":'"+String(currentDate)+"',\"Temperatura\":'"+String(t)+"',\"Humedad\":'"+String(h)+"',\"MicroServo\":'"+ms+"',\"Buzzer\":'"+b+"'}";
      Serial.println(PostData);
      // Send data to cloud
      int returnCode=RestPostData(AzureIoTHubURI,AzureIoTHubFingerPrint,AzureIoTHubAuth,PostData);
      Serial.println(returnCode);
      String mensaje=RestGetData(AzureIoTHubURIGet,AzureIoTHubFingerPrint,AzureIoTHubAuth);
      Serial.println(mensaje);

float limite=23.5;
if(t>limite){
  bandera_alarma=verdadero;
  Serial.println("Temperatura sobrepasada");}
else{
  bandera_alarma=falso;
  }
}

// Functions
int RestPostData(String URI, String fingerPrint, String Authorization, String PostData)
{
    HTTPClient http;
    http.begin(URI,fingerPrint);
    http.addHeader("Authorization",Authorization);
    http.addHeader("Content-Type", "application/atom+xml;type=entry;charset=utf-8");
    int returnCode=http.POST(PostData);
    if(returnCode<0) 
  {
    Serial.println("RestPostData: Error sending data: "+String(http.errorToString(returnCode).c_str()));
  }
    http.end();
  return returnCode;
}

String RestGetData(String URI, String fingerPrint, String Authorization)
{
    HTTPClient http;
    http.begin(URI,fingerPrint);
    http.addHeader("Authorization",Authorization);
    http.addHeader("Content-Type", "application/atom+xml;type=entry;charset=utf-8");
    http.collectHeaders(headerKeys, numberOfHeaders);
    int httpCode = http.GET();
    String payload;
    if(httpCode<0) 
  {
    Serial.println("RestGetData: Error getting data: "+String(http.errorToString(httpCode).c_str()));
  }
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String eTag = http.header("ETag");
        String newEtag=trimETag(eTag);
        payload = http.getString();
        mensaje=payload;
        String urlNew=AzureIoTHubURIDel;
        urlNew.replace("etag",newEtag);
        int httpdel=RestDelData(urlNew,AzureIoTHubFingerPrint,AzureIoTHubAuth);
        Serial.println(httpdel);
    }
    else{
    payload=String(httpCode);}
    http.end();
  return payload;
}

int RestDelData(String URI, String fingerPrint, String Authorization)
{
    HTTPClient http;
    http.begin(URI,fingerPrint);
    http.addHeader("Authorization",Authorization);
    http.addHeader("Content-Type", "application/atom+xml;type=entry;charset=utf-8");
    int httpCode = http.sendRequest("DELETE");;
    if(httpCode<0) 
  {
    Serial.println("RestDelData: Error getting data: "+String(http.errorToString(httpCode).c_str()));
  }
    http.end();
  return httpCode;
}

String trimETag(String value)
{
    String retVal=value;

    if(value.startsWith("\""))
      retVal=value.substring(1);

    if(value.endsWith("\""))
      retVal=retVal.substring(0,retVal.length()-1);

    return retVal;     
}
