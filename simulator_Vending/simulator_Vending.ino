#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "ESP8266HTTPClient.h"
#include "ESP8266httpUpdate.h"


extern "C" {
#include "user_interface.h"
}

/////////////////////////////////
////// WIFI SETTING /////////////
/////////////////////////////////

//#define WLAN_SSID       "Tenda_320CE0"
//#define WLAN_PASS       "0105654525afk" //
#define WLAN_SSID       "antlysis_meadow_2.4G@unifi"
#define WLAN_PASS       "!Ath4ht@w4dt!"
//#define WLAN_SSID "MX PJ21"
//#define WLAN_PASS "ssot1178"
//#define WLAN_SSID "MX SP"
//#define WLAN_PASS "ssot1178"
//#define WLANb_SSID "Dobidoo"
//#define WLAN_PASS "dobidoo123"
//#define WLAN_SSID "MX"
//#define WLAN_PASS "asdf7817"
//#define WLAN_SSID         "HUAWEI P10 lite"
//#define WLAN_PASS         "c5943f26-c6f"

/////////////////////////////////
////// PIN SETUP ///////////////
/////////////////////////////////
#define DetCoinIn2 13       //D7
#define VendingMtr1 12     //D6
#define VendingMtr2 14          //D5
#define DetCoinIn1 5      //D4
//#define DryerCoinIn 5        //D1

/////////////////////////////////
////// MQTT SETUP ///////////////
/////////////////////////////////

#define MQTT_SERVER      "192.168.1.3" // give static address
#define MQTT_PORT         1883
#define MQTT_USERNAME    ""
#define MQTT_PASSWORD         ""

/////////////////////////////////
/////// OTA setting /////////////
/////////////////////////////////

const int FW_VERSION = 9;
const char* fwUrlBase = "http://192.168.1.3/fw/";

WiFiClient client;
///////////////////////////////////
//////// SOME VARIABLE INIT ////////
////////////////////////////////////
int coinCounter1 = 0;
int coinCounter2 = 0;
int high1 = 0;
int high2 = 0;
int start = 0;
int start2 = 0;
int epay = 0;
int detEpay = 0;
int counter = 0;
unsigned int cCount = 0;
unsigned int dCount = 0;
unsigned long previousMillis = 0;
const long interval = 5000;

////////////////////////////////////
//////// TIME AND INTERRUPT ////////
////////////////////////////////////

os_timer_t IOReadout;

void IOtimerISR(void *pArg) {
  //////Serial.print("timer is functioning\b");
  //epay = digitalRead(Epayment);
  //detEpay = digitalRead(DetEpayment);
  //coinCount(detEpay, &coinCounter1, &high1, &start, &dCount);
  //coinCount(epay, &coinCounter2, &high2, &start2, &cCount);
}

void timerInit(void) {
  os_timer_setfn(&IOReadout, IOtimerISR, NULL);
  os_timer_arm(&IOReadout, 5, true);
}

///////////////////////////////////
///////// INITIALISATION //////////
///////////////////////////////////


void setup() {
  pinMode(DetCoinIn2, OUTPUT);
  pinMode(DetCoinIn1, OUTPUT);
  pinMode(VendingMtr2, OUTPUT);
  pinMode(VendingMtr1, OUTPUT);
  digitalWrite(DetCoinIn1, HIGH);
  delay(100);
  digitalWrite(DetCoinIn1, LOW);
  digitalWrite(DetCoinIn2, HIGH);
  delay(100);
  digitalWrite(DetCoinIn2, LOW);
  digitalWrite(VendingMtr2, HIGH);
  delay(100);
  digitalWrite(VendingMtr2, LOW);
  digitalWrite(VendingMtr1, HIGH);
  delay(100);
  digitalWrite(VendingMtr1, LOW);
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  //timerInit();
}





///////////////////////////////////
/////// MAIN LOOP /////////////////
///////////////////////////////////
void loop() {
  if (counter < 1) {
    for (int i=1; i <= 2; i++){
      digitalWrite(DetCoinIn1, HIGH);
      delay(100);
      digitalWrite(DetCoinIn1, LOW);
      delay(1000);
      digitalWrite(DetCoinIn2, HIGH);
      delay(100);
      digitalWrite(DetCoinIn2, LOW);
      delay(1000);
      digitalWrite(VendingMtr1, HIGH);
      delay(50);
      digitalWrite(VendingMtr1, LOW);
      delay(1000);
      digitalWrite(VendingMtr2, HIGH);
      delay(50);
      digitalWrite(VendingMtr2, LOW);
      delay(1000);
    }
//    digitalWrite(VendingMtr1, HIGH);
//    digitalWrite(VendingMtr2, HIGH);
//    //digitalWrite(DetCoinIn2, HIGH);
//    delay(5000);
//    digitalWrite(VendingMtr1, LOW);
//    digitalWrite(VendingMtr2, LOW);
    //digitalWrite(DetCoinIn1, LOW);
    //delay(5000);
//    digitalWrite(Machinelocked, HIGH);
//    delay(15000);
//    digitalWrite(Machinelocked, LOW);
//    delay(15000);
    counter++;
  }
}

//////////////////////////////////
////// FUNCTIONS /////////////////
//////////////////////////////////
void coinCount(int coin_input, int *coinHigh, int *high, int *coinLow, unsigned int *coinCount) {
  if (*high == 0) {
    if (coin_input == HIGH) {
      //Serial.print("coin high");
      (*coinHigh)++;
      //Serial.println(*coinHigh);
      if (*coinHigh > 3) {
        *coinHigh = 0;
        *high = 1;
      }
    } else {
      if (*coinLow > 100) {
        *coinHigh = 0;
      }
      *high = 0;
    }
  } else if (*high == 1) {
    if (coin_input == LOW) {
      //Serial.print("coin low");
      (*coinLow)++;
      if (*coinLow > 100) {
        *coinHigh = 0;
        *coinLow = 0;
        *high = 0;
        *coinCount++;
        Serial.println(*coinCount);
      }
    } else {
      *high = 1;
    }
  }
}



