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
//#define WLAN_SSID "Dobidoo"
//#define WLAN_PASS "dobidoo123"
//#define WLAN_SSID "MX"
//#define WLAN_PASS "asdf7817"
//#define WLAN_SSID         "HUAWEI P10 lite"
//#define WLAN_PASS         "c5943f26-c6f"

/////////////////////////////////
////// PIN SETUP ///////////////
/////////////////////////////////
#define DetEpayment 10       //SD3
#define Machinelocked 12     //D6
#define Epayment 14          //D5
#define DryerEpayment 13          //D7
#define CoinIn 5      //D4
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
//Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);
Adafruit_MQTT_Publish testStatus = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "TestChecked");

///////////////////////////////////
//////// SOME VARIABLE INIT ////////
////////////////////////////////////
int coinCounter1 = 0;
int coinCounter2 = 0;
int coinCounter3 = 0;
int high1 = 0;
int high2 = 0;
int high3 = 0;
int start = 0;
int start2 = 0;
int start3 = 0;
int epay = 0;
int detEpay = 0;
int dryerEpay = 0;
int counter = 0;
int cCount = 0;
int dCount = 0;
int dryCount = 0;
unsigned long previousMillis = 0;
const long interval = 5000;

////////////////////////////////////
//////// TIME AND INTERRUPT ////////
////////////////////////////////////

os_timer_t IOReadout;

void IOtimerISR(void *pArg) {
  //////Serial.print("timer is functioning\b");
  epay = digitalRead(Epayment);
  //Serial.println(epay);
  dryerEpay = digitalRead(DryerEpayment);
  detEpay = digitalRead(DetEpayment);
  dcoinCount(detEpay, &coinCounter1, &high1, &start, &dCount, 0);
  dcoinCount(dryerEpay, &coinCounter3, &high3, &start3, &dryCount, 1);
  coinCount(epay, &coinCounter2, &high2, &start2, &cCount, 1);
}

void timerInit(void) {
  os_timer_setfn(&IOReadout, IOtimerISR, NULL);
  os_timer_arm(&IOReadout, 5, true);
}

///////////////////////////////////
///////// INITIALISATION //////////
///////////////////////////////////


void setup() {
  pinMode(DetEpayment, INPUT);
  pinMode(Epayment, INPUT);
  pinMode(DryerEpayment, INPUT);
  pinMode(Machinelocked, OUTPUT);
  pinMode(CoinIn, OUTPUT);
  digitalWrite(CoinIn, LOW);
  digitalWrite(Machinelocked, LOW);
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
  timerInit();
}





///////////////////////////////////
/////// MAIN LOOP /////////////////
///////////////////////////////////
void loop() {
  MQTT_connect();
  if (counter < 1) {
    for (int i=1; i<=8; i++){
      digitalWrite(CoinIn, HIGH);
      delay(50);
      digitalWrite(CoinIn, LOW);
      delay(1000);
    }
    digitalWrite(Machinelocked, LOW);
    delay(15000);
    digitalWrite(Machinelocked, HIGH);
    delay(15000);
    digitalWrite(Machinelocked, LOW);
    delay(15000);
    digitalWrite(Machinelocked, HIGH);
    delay(15000);
    counter++;
  }
}

//////////////////////////////////
////// FUNCTIONS /////////////////
//////////////////////////////////

void MQTT_connect() {
  int8_t ret;
  int mycount;
  mycount = 0;
  // Stop if already connected.
  if (mqtt.connected()) {
    //Serial.println("MQTT Connected!");
    return;
  }
  unsigned long currentMillis = millis();
  // Serial.print("Connecting to MQTT... ");
  if (currentMillis - previousMillis >= interval) {
    if ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
      //    Serial.println(mqtt.connectErrorString(ret));
      //    Serial.println("Retrying MQTT connection in 5 seconds...");
      mqtt.disconnect();
      previousMillis = currentMillis;
    }
  }

}

void coinCount(int coin_input, int *coinHigh, int *high, int *coinLow, int *coinCount, int type) {
  if (*high == 0) {
    if (coin_input == LOW) {
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
    if (coin_input == HIGH) {
      //Serial.print("coin low");
      (*coinLow)++;
      //Serial.println(*coinLow);
      if (*coinLow > 100) {
        *coinHigh = 0;
        *coinLow = 0;
        *high = 0;
        (*coinCount)++;
        if (type == 1) {
          if (*coinCount == 8) {
            testStatus.publish("Coin8");
            *coinCount = 0;
          }
        }
        Serial.println(*coinCount);
      }
    } else {
      *high = 1;
    }
  }
}

void dcoinCount(int coin_input, int *coinHigh, int *high, int *coinLow, int *coinCount, int type) {
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
      //Serial.println(*coinLow);
      if (*coinLow > 100) {
        *coinHigh = 0;
        *coinLow = 0;
        *high = 0;
        (*coinCount)++;
        if (type == 0) {
          if (*coinCount == 2) {
            testStatus.publish("Coin8");
            *coinCount = 0;
          }
        } else if (type == 1) {
          if (*coinCount == 8) {
            testStatus.publish("Coin8");
            *coinCount = 0;
          }
        }
        Serial.println(*coinCount);
      }
    } else {
      *high = 1;
    }
  }
}



