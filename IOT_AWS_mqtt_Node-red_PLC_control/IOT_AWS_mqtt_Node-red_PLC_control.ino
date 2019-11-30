#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

//json을 위한 설정
StaticJsonDocument<200> doc;
DeserializationError error;
JsonObject root;

char ssid[32] = "";
char password[32] = "";

// AP 설정을 위한 변수
const char *softAP_ssid = "ap_CSB";
const char *softAP_password = "00000000";
/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);
boolean connect;
unsigned long lastConnectTry = 0;
String sAP_ssid; // mac address를 문자로 기기를 구분하는 기호로 사용
char cAP_ssid[40];

//mqtt 통신 변수
const char *thingId = "";          // 사물 이름 (thing ID) 
const char *host = "adh167sfy9bbp-ats.iot.us-east-2.amazonaws.com"; // AWS IoT Core 주소
const char* outTopic = "/sungbeen/out"; // 이름이 중복되지 않게 설정 기록
const char* inTopic = "/sungbeen/in"; // 이름이 중복되지 않게 설정 기록
const char* clientName = "";  // setup 함수에서 자동생성
String sChipID; // mac address를 문자로 기기를 구분하는 기호로 사용
char cChipID[40];
int value=0;
//WiFiClient espClient;
//PubSubClient client(espClient);

//plc 제어 변수
String s="";
int P4[4]={0};
int P0[6]={0};
int lastRead=0,lastMqtt=0,autoRead=0;
boolean stringComplete = false; 
String inputString;
char msg[100];

ESP8266WebServer server(80);

// 사물 인증서 (파일 이름: xxxxxxxxxx-certificate.pem.crt)
const char cert_str[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDWTCCAkGgAwIBAgIUBSjdqv1D7v6eVCNPsN+ANoFHzT0wDQYJKoZIhvcNAQEL
BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g
SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTE5MTAwOTE0MTAw
NFoXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0
ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMRNusNzN3nGY6NZxzLS
rEGLYPZdaleCA2TQX1j/jH6yjWLQOzdRFT8ld6NaN+zed+G5ol8ChRWsy/DPgJlV
5BwFPO/jCo0T3jZ3xF8mF/NT98hOQoahaIuwn95W85BNJr5shdXm2eN+y1jp9KcA
32UE2ovmCo0dlcA+dXRDe2lnVW54v3BPlJVqwezO1M37DZJCRh6TNytvO9ygbMUb
Puv2OZmwOsdHorH22rn8DsPNMk3KJ6WjKBCA28AcN0pTaCB9QQBezL3u1C+PP+9E
FZGrqZxGBv9GCKmtpSDFzyQzy9QgTYeHhGL/ncGUKVU+Voalnvk3Q2fYNZw6A00X
V9UCAwEAAaNgMF4wHwYDVR0jBBgwFoAUdy8qerntemlMNBnroZHUrVi7BFcwHQYD
VR0OBBYEFNzwZ6XOJWsXDdSvjZWN2ptqotKPMAwGA1UdEwEB/wQCMAAwDgYDVR0P
AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQAXEg0glSIxICaYCl6GxumatV33
kNA9MV/xDeeBn4oiAzWx7qaTr4aF6k1SuBd2/58dGEIdwKmCq4XCtOZj66WYIHGV
FECbt5QKeEMSATW5EElDVgmpJVzsLzlcVZKjJeRB8t+KdjxIx9P2A+rMWeRoUOVV
Yi/vJkHLLKKnlEl1ZU7KvhTsuMPPIdBPf9UzhjndGuKFLITCxnco2Y926P024geS
/jjlPUnLFaQWk1BQsdv2XOTIvBxUx1b636lLsCQLBDLQmIkrTY4K/Y3mppkQQFLM
OK+tHxUumrWdzK8hU7kSNbiq9CecmToO1tpaR0RFIbDR8XQXKkr4exOj0C+c
-----END CERTIFICATE-----
)EOF";
// 사물 인증서 프라이빗 키 (파일 이름: xxxxxxxxxx-private.pem.key)
const char key_str[] PROGMEM = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
MIIEpQIBAAKCAQEAxE26w3M3ecZjo1nHMtKsQYtg9l1qV4IDZNBfWP+MfrKNYtA7
N1EVPyV3o1o37N534bmiXwKFFazL8M+AmVXkHAU87+MKjRPeNnfEXyYX81P3yE5C
hqFoi7Cf3lbzkE0mvmyF1ebZ437LWOn0pwDfZQTai+YKjR2VwD51dEN7aWdVbni/
cE+UlWrB7M7UzfsNkkJGHpM3K2873KBsxRs+6/Y5mbA6x0eisfbaufwOw80yTcon
paMoEIDbwBw3SlNoIH1BAF7Mve7UL48/70QVkaupnEYG/0YIqa2lIMXPJDPL1CBN
h4eEYv+dwZQpVT5WhqWe+TdDZ9g1nDoDTRdX1QIDAQABAoIBAQCwIJGuZYTjxMWH
PbkFd5JIKLbxtPUB2MwUuwShhH/Y+XjzrKNEyYJHquVB8mN1KlHjfDwcBNXFkclV
ocffS/ojzetTzKCd3bKlqiXNvQ8GKlOKK/VyeAWmN+3ofpl/4kkd157pQXxDqnLm
rtrssFK/f3BazyauYFoJJ8EYdKeY9CQ7qeD6NBW2izsQVd6P2b9NnWXB0l+4xLGo
HbjDZ6Cex15Sz/TwxXOcpgev28s8WeIyov7QX/DcFZ/dEL8HFodQc7yFnZgq7sRP
X8Cv3CXZak9/fVQYzTY5iQZ6FRpWx676Y1iAP67lxkAYBzyjYjvj1sZ0afOKxVzJ
MKrl+ustAoGBAOqCsX1tB7cFN0B3H9ugpMbTBoZ3tvm1Xvcyn4RenLnYObUOkjWm
a9zucPbOH8uDCpsnEI0MSTsta4K7vbc19BftTZ2Hf6gu56Fvpgnvq/lsLDzhaV4G
+0Aa6NYhpwKhLI+f+3ohY2ltUec3CkuiS6FV1+Wg8jBSIxnaMV3WdDkbAoGBANZK
wMxv880Ge1w/Frr7WMvkxaLpi3xjcya2MBSQPEh0cchBVrtjGiitvAW5Q6us0DKW
hr8rELMyTPUUeZ1S6iigDKBhKB6zYskIhWuVxTKik/LDR0HM62c19iqCm3SiMRRO
bgzzrqxW9wijFmHQDYrnNNFMQH6jlc63pxSMfTHPAoGAblrCGzvLfZy+u19TZo7Z
+5YiY/TB7hAjeEAiQLAlIG9X+JO6yfRencC29+1XmPU5cHmZmEZHz8k35nVieWyy
SPAqrg6tS/+GkLlxcQuP6fe1eZC5fL3cTfZbyyha5ynhRlUKihZcKah/+kY0WhMt
c+Zpim2KiqKcUfYTdQ0N9DECgYEAvuPgUzz3JRA9G7NngDEerKKiG7mHc8rzbOym
b+r8iF6SbR0nMc3aI5p//2lFisxqumW0qt6mBbstKrSOkUKnLIFGPifOIWsS+JoK
oAfOnoiM2phyBX+Rw2MiYkPuTzucFMa46fi5BPSDIsKE0aUQK0hCmJ8gZK88Z+TZ
hUiQzakCgYEA4yryYpzyIrokrQazd9+Tb9aMxGBAIpmBBTtzfFK8bfoMbCq2o6UD
DtxZCRZSbDopewnA4SqtZkZrkrsmuZJSXdIZPKl+J5iMV7vXGSzEZBIe4bT5iGXW
GjgcnHv5kvwqytJzxJLv3kWy4dn6N9YO2QU7ylelDIlljcqX+qJBdbI=
-----END RSA PRIVATE KEY-----
)EOF";
// Amazon Trust Services(ATS) 엔드포인트 CA 인증서 (서버인증 > "RSA 2048비트 키: Amazon Root CA 1" 다운로드)
const char ca_str[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy
rqXRfboQnoZsG4q5WTP468SQvvG5
-----END CERTIFICATE-----
)EOF";

// 통신에서 문자가 들어오면 이 함수의 payload 배열에 저장된다.
void callback(char* topic, byte* payload, unsigned int length) {
  /*
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    Serial1.print((char)payload[i]);
  }
  Serial.println();
  */
  deserializeJson(doc,payload);
  root = doc.as<JsonObject>();
  const char* rChip = root["chip"];
  const char* prt = root["prt"];
  if(!rChip)
    return;
  Serial.println(prt);
  if(strcmp(rChip,cChipID)==0)
    Serial1.print(prt);
}

X509List ca(ca_str);
X509List cert(cert_str);
PrivateKey key(key_str);
WiFiClientSecure wifiClient;
PubSubClient client(host, 8883, callback, wifiClient); //set  MQTT port number to 8883 as per //standard

// mqtt 통신에 지속적으로 접속한다.
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(thingId)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "hello world");
      // ... and resubscribe
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      char buf[256];
      wifiClient.getLastSSLError(buf,256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Set time via NTP, as required for x.509 validation
void setClock() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}


void setup(void) {
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial.setDebugOutput(true);
  Serial.println();

  //이름 자동으로 생성
  uint8_t chipid[6]="";
  WiFi.macAddress(chipid);
  sprintf(cChipID,"%02x%02x%02x%02x%02x%02x%c",chipid[5], chipid[4], chipid[3], chipid[2], chipid[1], chipid[0],0);
  sChipID=String(cChipID);
  sAP_ssid=String(softAP_ssid)+sChipID;
  sAP_ssid.toCharArray(cAP_ssid,sAP_ssid.length()+1);
  softAP_ssid=&cAP_ssid[0];
  setupAp();
  Serial.println(sChipID);
  Serial.println(softAP_ssid);
  
  server.on("/", handleRoot);
  server.on("/onoffP40", handleOnOffP40);
  server.on("/onoffP41", handleOnOffP41);
  server.on("/onoffP42", handleOnOffP42);
  server.on("/onoffP43", handleOnOffP43);
  server.on("/read", handleRead);
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  loadCredentials(); // Load WLAN credentials from network
  if(strlen(ssid) > 0) { // EEPROM에 와이파이 이름 저장 되어 있으면 WLAN 연결
    connectWifi();
  }
}

void setupAp() {
  Serial.println("");
  Serial.println(softAP_ssid);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
}

void connectWifi() {
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  //WiFi.mode(WIFI_STA); //이 모드를 설정 않해야 AP가 살아있습니다.
  WiFi.begin(ssid, password);

  int connRes = WiFi.waitForConnectResult();
  Serial.print ( "connRes: " );
  Serial.println ( connRes );
  if(connRes == WL_CONNECTED){
    wifiClient.setTrustAnchors(&ca);
    wifiClient.setClientRSACert(&cert, &key);
    setClock();
    client.setCallback(callback);
    //Serial.println("Certifications and key are set");
    Serial.println("AWS connected");
    Serial.println(WiFi.localIP());
  }
  else
    WiFi.disconnect();
}


void loop(void) {
  unsigned int sWifi = WiFi.status();
  if (!client.connected() && sWifi==WL_CONNECTED) {
    reconnect();
  }
  client.loop();
  server.handleClient();

  long now = millis();
  //6초에 한번 와이파이 끊기면 다시 연결
  if (sWifi == WL_IDLE_STATUS && now > (lastConnectTry + 60000) && strlen(ssid) > 0 ) {
    lastConnectTry=now;
    Serial.println ( "Connect requested" );
    connectWifi();
  }
  
  if ((now - lastRead > 1000)){
    lastRead = now;
    serialEvent();
    if(autoRead==1) {
      String s="";
      s=char(5);
      s+="00RSS0104%PW0";
      s+=char(4);
      Serial1.print(s);
    }
  }
}

void serialEvent() {
  // print the string when a newline arrives:
  if (stringComplete) {
    Serial.println(inputString);
    inputString.toCharArray(msg, inputString.length());
    if(msg[3]=='R') {
        byte inC=msg[13];
        byte inC1=msg[12];
        P0[0]=bitRead(inC,0);
        P0[1]=bitRead(inC,1);
        P0[2]=bitRead(inC,2);
        P0[3]=bitRead(inC,3);
        P0[4]=bitRead(inC1,0);
        P0[5]=bitRead(inC1,1);
        GoHome();
    }
    inputString="{\"chip\":\""+sChipID+"\",\"prt\":\""+inputString+"\"} ";
    inputString.toCharArray(msg, inputString.length());
    client.publish(outTopic, msg);
    // clear the string:
    inputString = "";
    stringComplete = false;
    
  }
  
  if(Serial.available() == false) 
    return;
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    //Serial.print(inChar);
    // add it to the inputString:
    inputString += inChar;
  }
  stringComplete = true;
}

void plcOut() {
  //int out=P40+P41*2+P42*4+P43*8;
  String out =  String(P4[0]+P4[1]*2+P4[2]*4+P4[3]*8, HEX);
  String s="";
  s=char(5);
  s+="00WSS0104%PW4000";
  s+=out;
  s+=char(4);
  Serial1.print(s);
}
