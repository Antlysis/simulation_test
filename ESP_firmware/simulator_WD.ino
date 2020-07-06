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
//#define WLAN_SSID       "hellotime@2.4Ghz"
//#define WLAN_PASS       "qweasd13579"
#define WLAN_SSID       "ideal 2nd floor"
#define WLAN_PASS       "0379805027ideal"

#define NCULTI    1
#define NO        2

#define TYPE NO

/////////////////////////////////
////// PIN SETUP ///////////////
/////////////////////////////////
#if TYPE == NO
  #define coinIn 4       //SD3
  #define simInput 13
  #define simOutput 14
#elif TYPE == NCULTI
  #define coinIn 13
#endif
#define coinCtrl 5     //D6
//#define DryerCoinIn 5        //D1

/////////////////////////////////
////// MQTT SETUP ///////////////
/////////////////////////////////

#define MQTT_SERVER      "192.168.0.9" // give static address
#define MQTT_PORT         1883
#define MQTT_USERNAME    ""
#define MQTT_PASSWORD         ""

/////////////////////////////////
/////// OTA setting /////////////
/////////////////////////////////

const int FW_VERSION = 9;
const char* fwUrlBase = "http://192.168.0.9/fw/";

WiFiClient client;


///////////////////////////////////
//////// SOME VARIABLE INIT ////////
////////////////////////////////////
unsigned int coinCounter1 = 0;
unsigned int high1 = 0;
unsigned int start = 0;
unsigned int coinRCV = 0;
unsigned int coin_input1 = 0;
unsigned int coinCounter2 = 0;
unsigned int high2 = 0;
unsigned int start2 = 0;
unsigned int coinRCV2 = 0;
unsigned int coin_input2 = 0;

////////////////////////////////////
//////// TIME AND INTERRUPT ////////
////////////////////////////////////

os_timer_t IOReadout;

void IOtimerISR(void *pArg) {
  
  coin_input1 = digitalRead(coinIn);
  #if TYPE == NO
    coin_input2 = digitalRead(simOutput);
    coinCount2(coin_input2, &coinCounter2, &high2, &start2, &coinRCV2);
  #endif
  coinCount(coin_input1, &coinCounter1, &high1, &start, &coinRCV);
}


void timerInit(void) {
  os_timer_setfn(&IOReadout, IOtimerISR, NULL);
  os_timer_arm(&IOReadout, 5, true);
}

///////////////////////////////////
///////// INITIALISATION //////////
///////////////////////////////////


void setup() {
  pinMode(coinCtrl, OUTPUT);
  pinMode(coinIn, INPUT);
  #if TYPE == NO
    pinMode(simInput, OUTPUT);
    pinMode(simOutput, INPUT);
    digitalWrite(simInput, LOW);
  #endif
  digitalWrite(coinCtrl, LOW);
  
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
  String mac;
  mac = getMAC();
  Serial.println (mac);
}




unsigned int currMilli = 0;
unsigned int lastMilli = 0;
unsigned int myepay = 0;
unsigned int myepay2 = 0;
unsigned int counter = 0;
///////////////////////////////////
/////// MAIN LOOP /////////////////
///////////////////////////////////
void loop() {
  currMilli = millis();
  //digitalWrite(coinCtrl, LOW);
  #if TYPE == NO
    pay_NO(&myepay, &lastMilli, &currMilli);
    pay_NO2(&myepay2, &lastMilli, &currMilli);
  #elif TYPE == NCULTI
//    Serial.print("test");
    payIpsoW_NO(&myepay, &lastMilli, &currMilli);
  #endif
  if (counter < 1) {
    lastMilli = millis();
    myepay = 8;
    counter++;
  } else if(counter < 2) {
    lastMilli = millis();
    myepay2 = 8;
    counter++;
  }
}


void payIpsoW_NO (unsigned int *amt, unsigned int *lastms, unsigned int *currms ) {
//  Serial.println(*amt);
  unsigned int jdo = 0;
  unsigned int jdo2 = 0;
  if (*amt != 0) {
    if (*currms - *lastms <= 50) {
      if (jdo == 0) {
        digitalWrite(coinCtrl, HIGH);
        jdo++;
      }       
    } else if (*currms - *lastms >= 50 && *currms - *lastms <= 650) {
      if (jdo2 == 0) {
        digitalWrite(coinCtrl, LOW);
        jdo2++;
      }
    } else if (*currms - *lastms >=1000) {
        jdo = 0;
        jdo2 = 0;
        (*amt)--;
        *lastms = *currms;
    }
  }
}

void pay_NO (unsigned int *amt, unsigned int *lastms, unsigned int *currms ) {
  unsigned int jdo = 0;
  unsigned int jdo2 = 0;
  if (*amt != 0) {
    if (*currms - *lastms <= 50) {
      if (jdo == 0) {
        digitalWrite(coinCtrl, HIGH);
        jdo++;
      }       
    } else if (*currms - *lastms >= 50 && *currms - *lastms <= 650) {
      if (jdo2 == 0) {
        digitalWrite(coinCtrl, LOW);
        jdo2++;
      }
    } else if (*currms - *lastms >=1000) {
        jdo = 0;
        jdo2 = 0;
        (*amt)--;
        *lastms = *currms;
    }
  }
}
#if TYPE == NO
void pay_NO2 (unsigned int *amt, unsigned int *lastms, unsigned int *currms ) {
  unsigned int jdo = 0;
  unsigned int jdo2 = 0;
  if (*amt != 0) {
    if (*currms - *lastms <= 50) {
      if (jdo == 0) {
        digitalWrite(simInput, HIGH);
        jdo++;
      }       
    } else if (*currms - *lastms >= 50 && *currms - *lastms <= 650) {
      if (jdo2 == 0) {
        digitalWrite(simInput, LOW);
        jdo2++;
      }
    } else if (*currms - *lastms >=1000) {
        jdo = 0;
        jdo2 = 0;
        (*amt)--;
        *lastms = *currms;
    }
  }
}
#endif
//////////////////////////////////
////// FUNCTIONS /////////////////
//////////////////////////////////
String getMAC()
{
  uint8_t mac[6];
  char result[14];
  WiFi.macAddress(mac);
  sprintf(result, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String( result );
}

void coinCount(unsigned int coin_input, unsigned int *coinHigh, unsigned int *high, unsigned int *coinLow, unsigned int *coin_rcv) {
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
      if (*coinLow > 40) {
        *coinHigh = 0;
      }
      *high = 0;
    }
  } else if (*high == 1) {
    if (coin_input == LOW) {
      //Serial.print("coin low");
      (*coinLow)++;
      //Serial.println(*coinLow);
      if (*coinLow > 10) {
        *coinHigh = 0;
        *coinLow = 0;
        *high = 0;
        (*coin_rcv)++;
        Serial.println(*coin_rcv);
      }
    } else {
      *high = 1;
    }
  }
}

void coinCount2(unsigned int coin_input, unsigned int *coinHigh, unsigned int *high, unsigned int *coinLow, unsigned int *coin_rcv) {
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
      if (*coinLow > 40) {
        *coinHigh = 0;
      }
      *high = 0;
    }
  } else if (*high == 1) {
    if (coin_input == HIGH) {
      //Serial.print("coin low");
      (*coinLow)++;
      //Serial.println(*coinLow);
      if (*coinLow > 10) {
        *coinHigh = 0;
        *coinLow = 0;
        *high = 0;
        (*coin_rcv)++;
        Serial.println("Output");
        Serial.println(*coin_rcv);
      }
    } else {
      *high = 1;
    }
  }
}
