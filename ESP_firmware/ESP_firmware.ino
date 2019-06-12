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
#define BILL_IN 2
#define BILL_CTRL 10
#define IPSO_W_IN 10    //JP8
#define IPSO_W_STATUS 14   //JP1
#define IPSO_W_CTRL 12     //JP2
#define DTG_CA_CTRL 5     //JP6
#define DTG_CA1_IN 10       //JP8
#define DTG_CA2_IN 12      //JP2
#define DTG_MTR_BEG 14     //JP1 (first two)
#define DTG_MTR_SFT 16      //JP3 (second two)
#define DTG_MTR_DTG 4      //JP3 (third two)
#define DEX_D2_STATUS 16   //JP3
#define DEX_D_IN 5         //JP6
#define IPSO_D_IN 5        //JP6
#define DEX_W_IN 5         //JP6
#define DEX_D1_STATUS 4    //JP5
#define DEX_W_STATUS 4     //JP5
#define IPSO_D_STATUS 4    //JP5
#define IPSO_D_CTRL 2      //JP4
#define DEX_D_CTRL  2      //JP4
#define DEX_W_CTRL  2      //JP4     


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

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
//Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);
Adafruit_MQTT_Publish lastwill = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "connectivity/467b712f6f3dd1baad34f7b11b75f7d1");

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish runStatus = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "Lock/467b712f6f3dd1baad34f7b11b75f7d1");
Adafruit_MQTT_Publish detStatus = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "detDrop/467b712f6f3dd1baad34f7b11b75f7d1");
Adafruit_MQTT_Publish coinIn = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "coinIn/467b712f6f3dd1baad34f7b11b75f7d1");
Adafruit_MQTT_Publish versFeed = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "versionFeed/467b712f6f3dd1baad34f7b11b75f7d1");
// Setup a feed called 'esp8266_led' for subscribing to changes.
Adafruit_MQTT_Subscribe receivedPayment = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "467b712f6f3dd1baad34f7b11b75f7d1");
Adafruit_MQTT_Subscribe firmwareUpdate = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "firmwareUpdate");

////////////////////////////////////
//////// SOME VARIABLE INIT ////////
////////////////////////////////////

int sftnrCounter = 0;
int sftnrDrop = 0;
int turn_sft = 0;
int dtgCounter = 0;
int dtgDrop = 0;
int turn_dtg = 0;
int begCounter = 0;
int begDrop = 0;
int turn_beg = 0;
int coin_input1 = 0;
int coin_input2 = 0;
int bill_input = 0;
int billCounter = 0;
int lockCounter1 = 0;
int lockCounter2 = 0;
int lockState1 = 0;
int lockState2 = 0;
int coinH = 0;
int coinL = 0;
int coinCounter1 = 0;
int unlockCounter1 = 0;
int coinCounter2 = 0;
int unlockCounter2 = 0;
int high1 = 0;
int high2 = 0;
int billhigh1 = 0;
int checkOne1 = 0;
int SendOne1 = 0;
int checkOne2 = 0;
int SendOne2 = 0;
int start = 0;
unsigned long previousMillis = 0;
const long interval = 5000;
// ipsoWasher | ipsoDryer | dexWasher | dexDryerSingle | dexDryerDouble | detergent | billAcceptor
String machineType = String("ipsoWasher");

////////////////////////////////////
//////// TIME AND INTERRUPT ////////
////////////////////////////////////
// start the timer for the heartbeat
os_timer_t myHeartBeat;
os_timer_t IOReadout;

void timerCallback(void *pArg) {
  //////Serial.print("timer is functioning\b");
  lastwill.publish("ON");
}
void IOtimerISR(void *pArg) {
  //////Serial.print("timer is functioning\b");
  if (machineType == "ipsoWasher") {
    lockState1 =  digitalRead(IPSO_W_STATUS);
    coin_input1 = digitalRead(IPSO_W_IN);
  } else if (machineType == "ipsoDryer") {
    lockState1 =  digitalRead(IPSO_D_STATUS);
    coin_input1 = digitalRead(IPSO_D_IN);
  } else if (machineType == "dexWasher") {
    lockState1 =  digitalRead(DEX_W_STATUS);
    coin_input1 = digitalRead(DEX_W_IN);
  } else if (machineType == "dexDryerSingle") {
    lockState1 =  digitalRead(DEX_D1_STATUS);
    coin_input1 = digitalRead(DEX_D_IN);
  } else if (machineType == "dexDryerDouble") {
    lockState1 =  digitalRead(DEX_D1_STATUS);
    lockState2 =  digitalRead(DEX_D2_STATUS);
    coin_input1 = digitalRead(DEX_D_IN);
  }
  coinCount(coin_input1, &coinCounter1, &high1, &start);
}

//void ShiftRegisterISR(void *pArg) {
//   /* Read in first 74HC165 data. */
//   register_value = shift_in();
//   Serial.print(register_value, BIN);
//   Serial.println(" ");
//}

void timerInit(void) {
  os_timer_setfn(&myHeartBeat, timerCallback, NULL);
  os_timer_arm(&myHeartBeat, 30000, true);

  os_timer_setfn(&IOReadout, IOtimerISR, NULL);
  os_timer_arm(&IOReadout, 5, true);

  //os_timer_setfn(&ShiftRegister, ShiftRegisterISR, NULL);
  //os_timer_arm(&ShiftRegister, 15, true);
}



///////////////////////////////////
///////// INITIALISATION //////////
///////////////////////////////////


void setup() {
  if (machineType == "ipsoWasher") {
    pinMode(IPSO_W_IN, INPUT);
    pinMode(IPSO_W_STATUS, INPUT);
    pinMode(IPSO_W_CTRL, OUTPUT);
    digitalWrite(IPSO_W_CTRL, LOW);
    //digitalWrite(IPSO_W_IN, HIGH);
    delay(1000);
    //payIpsoW_NO(1);
  } else if (machineType == "ipsoDryer") {
    pinMode(IPSO_D_IN, INPUT);
    pinMode(IPSO_D_CTRL, OUTPUT);
    pinMode(IPSO_D_STATUS, INPUT);
    digitalWrite(IPSO_D_CTRL, HIGH);
    delay(1000);
    digitalWrite(IPSO_D_CTRL, LOW);
  } else if (machineType == "dexWasher") {
    pinMode(DEX_W_IN, INPUT);
    pinMode(DEX_W_CTRL, OUTPUT);
    pinMode(DEX_W_STATUS, INPUT);
    /// this is to avoid this small toggle create a credit to the machine
    digitalWrite(DEX_W_CTRL, HIGH);
    delay(1000);
    digitalWrite(DEX_W_CTRL, LOW);
  } else if (machineType == "dexDryerDouble") {
    pinMode(DEX_D_IN, INPUT);
    pinMode(DEX_D_CTRL, OUTPUT);
    pinMode(DEX_D1_STATUS, INPUT);
    pinMode(DEX_D2_STATUS, INPUT);
    /// this is to avoid this small toggle create a credit to the machine
    digitalWrite(DEX_D_CTRL, HIGH);
    delay(1000);
    digitalWrite(DEX_D_CTRL, LOW);
  } else if (machineType == "dexDryerSingle") {
    pinMode(DEX_D_IN, INPUT);
    pinMode(DEX_D_CTRL, OUTPUT);
    pinMode(DEX_D1_STATUS, INPUT);
    /// this is to avoid this small toggle create a credit to the machine
    digitalWrite(DEX_D_CTRL, HIGH);
    delay(1000);
    digitalWrite(DEX_D_CTRL, LOW);
  } else if (machineType == "detergent") {
    pinMode(DTG_CA1_IN, INPUT);
    pinMode(DTG_CA_CTRL, OUTPUT);
    pinMode(DTG_CA2_IN, INPUT);
    pinMode(DTG_MTR_SFT, INPUT);
    pinMode(DTG_MTR_DTG, INPUT);
    pinMode(DTG_MTR_BEG, INPUT);
    digitalWrite(DTG_CA_CTRL, LOW);
    //delay(2000);
    //digitalWrite(DTG_CA_CTRL,LOW);
  } else if (machineType == "billAcceptor") {
    pinMode(BILL_IN, INPUT);
    pinMode(BILL_CTRL, OUTPUT);
    digitalWrite(BILL_CTRL, LOW);
  }
  Serial.begin(115200);
  //digitalWrite(LED_BUILTIN, HIGH);
  //////Serial.println();
  //////Serial.print("Connecting to ");
  //////Serial.println(WLAN_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //////Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: "); ////Serial.println(WiFi.localIP());
  //////Serial.println("The firmware has changed");
  mqtt.subscribe(&receivedPayment);
  mqtt.subscribe(&firmwareUpdate);
  mqtt.will(MQTT_USERNAME "connectivity/467b712f6f3dd1baad34f7b11b75f7d1", "OFF");
  timerInit();
  versFeed.publish(FW_VERSION);
  //payDet_NO(9);
}



///////////////////////////////////
/////// MAIN LOOP /////////////////
///////////////////////////////////
int counter = 0;
boolean donefw = false;
String mac;
void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  MQTT_connect();
  if (!donefw) {
    versFeed.publish(FW_VERSION);
    donefw = true;
    mac = getMAC();
    Serial.println (mac);
  }
  Adafruit_MQTT_Subscribe *subscription;
  if (machineType == "ipsoWasher") {
    //////Serial.println("ipsoWasher");
    washerStatusRead(lockState1, &SendOne1, &checkOne1, &lockCounter1, &unlockCounter1);
    //ipsoDryerStatusRead(lockState1, &SendOne1, &checkOne1, &lockCounter1, &unlockCounter1);
    while ((subscription = mqtt.readSubscription())) {
      if (subscription == &receivedPayment) {
        char *message = (char *)receivedPayment.lastread;
        float to_float = atof(message);
        int amount = int(to_float);
        if (isValidNumber(message) == true) {
          payIpsoW_NO(amount);
        }
      } else if (subscription == &firmwareUpdate) {
        //////Serial.println("receive firmware update request");
        char *message = (char *)firmwareUpdate.lastread;
        //////Serial.print(F("Got: "));
        ////Serial.println(message);
        // Check if the message was ON, OFF, or TOGGLE.
        if (strncmp(message, "update", 6) == 0) {
          ////Serial.println("ya, it is confirmed to update the firmware");
          checkForUpdates();
        }
      }
    }
  } else if (machineType == "ipsoDryer") {
    //////Serial.println("ipsoDryer");
    //coinCount(coin_input1, &coinCounter1, &high1, &start);
    ipsoDryerStatusRead(lockState1, &SendOne1, &checkOne1, &lockCounter1, &unlockCounter1);
    while ((subscription = mqtt.readSubscription())) {
      if (subscription == &receivedPayment) {
        char *message = (char *)receivedPayment.lastread;
        float to_float = atof(message);
        int amount = int(to_float);
        if (isValidNumber(message) == true) {
          payIpsoD_NO(amount);
        }
      } else if (subscription == &firmwareUpdate) {
        ////Serial.println("receive firmware update request");
        char *message = (char *)firmwareUpdate.lastread;
        ////Serial.print(F("Got: "));
        ////Serial.println(message);
        // Check if the message was ON, OFF, or TOGGLE.
        if (strncmp(message, "update", 6) == 0) {
          ////Serial.println("ya, it is confirmed to update the firmware");
          checkForUpdates();
        }
      }
    }
  } else if (machineType == "dexWasher") {
    //lockState1 =  digitalRead(DEX_W_STATUS);
    //coin_input1 = digitalRead(DEX_W_IN);
    //coinCount(coin_input1, &coinCounter1, &high1, &start);
    washerStatusRead(lockState1, &SendOne1, &checkOne1, &lockCounter1, &unlockCounter1);
    while ((subscription = mqtt.readSubscription())) {
      if (subscription == &receivedPayment) {
        char *message = (char *)receivedPayment.lastread;
        float to_float = atof(message);
        int amount = int(to_float);
        if (isValidNumber(message) == true) {
          payDexW_NO(amount);
        }
      } else if (subscription == &firmwareUpdate) {
        ////Serial.println("receive firmware update request");
        char *message = (char *)firmwareUpdate.lastread;
        ////Serial.print(F("Got: "));
        ////Serial.println(message);
        // Check if the message was ON, OFF, or TOGGLE.
        if (strncmp(message, "update", 6) == 0) {
          ////Serial.println("ya, it is confirmed to update the firmware");
          checkForUpdates();
        }
      }
    }
  } else if (machineType == "dexDryerSingle") {
    //lockState1 =  digitalRead(DEX_D1_STATUS);
    //coin_input1 = digitalRead(DEX_D_IN);
    //coinCount(coin_input1, &coinCounter1, &high1, &start);
    dexdryerStatusReadSingle(lockState1, &SendOne1, &checkOne1, &lockCounter1, &unlockCounter1);
    while ((subscription = mqtt.readSubscription())) {
      if (subscription == &receivedPayment) {
        ////Serial.println("received payment");
        char *message = (char *)receivedPayment.lastread;
        float to_float = atof(message);
        int amount = int(to_float);
        if (isValidNumber(message) == true) {
          payDexD_NO(amount);
        }
      } else if (subscription == &firmwareUpdate) {
        ////Serial.println("receive firmware update request");
        char *message = (char *)firmwareUpdate.lastread;
        ////Serial.print(F("Got: "));
        ////Serial.println(message);
        // Check if the message was ON, OFF, or TOGGLE.
        if (strncmp(message, "update", 6) == 0) {
          ////Serial.println("ya, it is confirmed to update the firmware");
          checkForUpdates();
        }
      }
    }
  } else if (machineType == "dexDryerDouble") {
    //lockState1 =  digitalRead(DEX_D1_STATUS);
    //lockState2 =  digitalRead(DEX_D2_STATUS);
    //coin_input1 = digitalRead(DEX_D_IN);
    //coinCount(coin_input1, &coinCounter1, &high1, &start);
    dexdryerStatusReadDouble(lockState1, lockState2, &SendOne1, &SendOne2, &checkOne1, &checkOne2, &lockCounter1, &lockCounter2, &unlockCounter1, &unlockCounter2);
    while ((subscription = mqtt.readSubscription())) {
      if (subscription == &receivedPayment) {
        char *message = (char *)receivedPayment.lastread;
        float to_float = atof(message);
        int amount = int(to_float);
        if (isValidNumber(message) == true) {
          payDexD_NO(amount);
        }
      } else if (subscription == &firmwareUpdate) {
        //Serial.println("receive firmware update request");
        char *message = (char *)firmwareUpdate.lastread;
        //Serial.print(F("Got: "));
        //Serial.println(message);
        // Check if the message was ON, OFF, or TOGGLE.
        if (strncmp(message, "update", 6) == 0) {
          //Serial.println("ya, it is confirmed to update the firmware");
          checkForUpdates();
        }
      }
    }
  } else if (machineType == "detergent") {
    sftnrDrop =  digitalRead(DTG_MTR_SFT);
    dtgDrop = digitalRead(DTG_MTR_DTG);
    begDrop = digitalRead(DTG_MTR_BEG);
    coin_input1 = digitalRead(DTG_CA1_IN);
    coin_input2 = digitalRead(DTG_CA2_IN);
    //bill_input = digitalRead(BILL_IN);
    detergentCoinCount(coin_input1, coin_input2, &high1, &high2, &coinCounter1, &coinCounter2);
    detergentReadStatus(&turn_sft, &turn_beg, &turn_dtg, sftnrDrop, dtgDrop, begDrop, &sftnrCounter, &begCounter, &dtgCounter);
    //billInRead(bill_input, &billhigh1, &billCounter);
    while ((subscription = mqtt.readSubscription())) {
      if (subscription == &receivedPayment) {
        char *message = (char *)receivedPayment.lastread;
        float to_float = atof(message);
        int amount = int(to_float);
        if (isValidNumber(message) == true) {
          payDet_NO(amount);
        }
      } else if (subscription == &firmwareUpdate) {
        //Serial.println("receive firmware update request");
        char *message = (char *)firmwareUpdate.lastread;
        //Serial.print(F("Got: "));
        //Serial.println(message);
        // Check if the message was ON, OFF, or TOGGLE.
        if (strncmp(message, "update", 6) == 0) {
          //Serial.println("ya, it is confirmed to update the firmware");
          checkForUpdates();
        }
      }
    }
  } else if (machineType == "billAcceptor") {
    bill_input = digitalRead(BILL_IN);
    billInRead(bill_input, &high1, &billCounter);
    while ((subscription = mqtt.readSubscription())) {
      if (subscription == &receivedPayment) {
        char *message = (char *)receivedPayment.lastread;
        float to_float = atof(message);
        int amount = int(to_float);
        if (isValidNumber(message) == true) {
          payBill_NO(amount);
        }
      } else if (subscription == &firmwareUpdate) {
        //Serial.println("receive firmware update request");
        char *message = (char *)firmwareUpdate.lastread;
        //Serial.print(F("Got: "));
        //Serial.println(message);
        // Check if the message was ON, OFF, or TOGGLE.
        if (strncmp(message, "update", 6) == 0) {
          //Serial.println("ya, it is confirmed to update the firmware");
          checkForUpdates();
        }
      }
    }
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
    ////Serial.println("MQTT Connected!");
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

void payIpsoW_NO(int amt) {
  for (int i = 1; i <= amt; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(IPSO_W_CTRL, HIGH);
    delay(50);//50
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(IPSO_W_CTRL, LOW);
    delay(800);
  }
}

void payDexW_NO (int amt) {
  for (int i = 1; i <= amt; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(DEX_W_CTRL, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(DEX_W_CTRL, LOW);
    delay(2000);
  }
}

void payDexD_NO (int amt) {
  for (int i = 1; i <= amt; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(DEX_D_CTRL, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(DEX_D_CTRL, LOW);
    delay(2000);
  }
}

void payIpsoD_NO (int amt) {
  for (int i = 1; i <= amt; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(IPSO_D_CTRL, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(IPSO_D_CTRL, LOW);
    delay(2000);
  }
}

void payDet_NO (int amt) {
  for (int i = 1; i <= amt; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(DTG_CA_CTRL, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(DTG_CA_CTRL, LOW);
    delay(2000);
  }
}

void payBill_NO (int amt) {
  for (int i = 1; i <= amt; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(BILL_CTRL, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
    digitalWrite(BILL_CTRL, LOW);
    delay(2000);
  }
}

void blink (int times) {
  for (int i = 1; i <= times; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);
    delay(2000);
  }
}

///////////////////////////////////
////// Coin Acceptor //////////////
///////////////////////////////////
//void coinCountIpsoW(int coin_input, int *coinHigh, int *high, int *coinLow) {
//  if(*high == 0) {
//    if(coin_input == HIGH) {
//      //Serial.print("coin high");
//      (*coinHigh)++;
//      //Serial.println(*coinHigh);
//      if (*coinHigh > 3) {
//        *coinHigh = 0;
//        *high = 1;
//      }
//    } else {
//      if (*coinLow > 100) {
//        *coinHigh = 0;
//      }
//      *high = 0;
//    }
//  } else if (*high == 1) {
//    if (coin_input == LOW) {
//      //Serial.print("coin low");
//      (*coinLow)++;
//      //Serial.println(*coinLow);
//      if (*coinLow > 100) {
//        *coinHigh = 0;
//        *coinLow = 0;
//        *high = 0;
//        coinIn.publish("1COIN");
//      }
//    } else {
//      *high = 1;
//    }
//  }
//}

void coinCount(int coin_input, int *coinHigh, int *high, int *coinLow) {
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
        coinIn.publish("1COIN");
      }
    } else {
      *high = 1;
    }
  }
}

void ipsoDryerStatusRead(int lockState1,  int *SendOne1, int *checkOne1, int *lockCounter1, int *unlockCounter1) {
  if (lockState1 == HIGH) {
    if (*SendOne1 <= 5) {
      (*lockCounter1)++;
      if (*lockCounter1 > 700) {
        ////Serial.print("Locked");
        runStatus.publish("Locked");
        *lockCounter1 = 0;
        (*SendOne1)++;
        *checkOne1 = 0;
        *unlockCounter1 = 0;
      }
    }
  } else if (lockState1 == LOW) {
    if (*checkOne1 <= 2) {
      (*unlockCounter1)++;
      if (*unlockCounter1 > 50) {
        ////Serial.print("Unlocked");
        runStatus.publish("Unlocked");
        (*checkOne1)++;
        *SendOne1 = 0;
        *unlockCounter1 = 0;
      }
      *lockCounter1 = 0;
    }
  }
}

void dexdryerStatusReadDouble(int lockState1, int lockState2, int *SendOne1, int *SendOne2, int *checkOne1, int *checkOne2, int *lockCounter1, int *lockCounter2, int *unlockCounter1, int *unlockCounter2) {
  if (lockState1 == LOW) {
    if (*SendOne1 <= 5) {
      (*lockCounter1)++;
      if (*lockCounter1 > 700) {
        ////Serial.print("Locked");
        runStatus.publish("Locked1");
        *lockCounter1 = 0;
        (*SendOne1)++;
        *checkOne1 = 0;
        *unlockCounter1 = 0;
      }
    }
  } else if (lockState1 == HIGH) {
    if (*checkOne1 <= 2) {
      (*unlockCounter1)++;
      if (*unlockCounter1 > 50) {
        ////Serial.print("Unlocked");
        runStatus.publish("Unlocked1");
        (*checkOne1)++;
        *SendOne1 = 0;
        *unlockCounter1 = 0;
      }
      *lockCounter1 = 0;
    }
  }
  if (lockState2 == LOW) {
    if (*SendOne2 <= 5) {
      (*lockCounter2)++;
      if (*lockCounter2 > 700) {
        ////Serial.print("Locked");
        runStatus.publish("Locked2");
        *lockCounter2 = 0;
        (*SendOne2)++;
        *checkOne2 = 0;
        *unlockCounter2 = 0;
      }
    }
  } else if (lockState2 == HIGH) {
    if (*checkOne2 <= 2) {
      (*unlockCounter2)++;
      if (*unlockCounter2 > 50) {
        ////Serial.print("Unlocked");
        runStatus.publish("Unlocked2");
        (*checkOne2)++;
        *SendOne2 = 0;
        *unlockCounter2 = 0;
      }
      *lockCounter2 = 0;
    }
  }
}

void dexdryerStatusReadSingle(int lockState1, int *SendOne1, int *checkOne1, int *lockCounter1, int *unlockCounter1) {
  if (lockState1 == LOW) {
    if (*SendOne1 <= 5) {
      (*lockCounter1)++;
      if (*lockCounter1 > 700) {
        ////Serial.print("Locked");
        runStatus.publish("Locked");
        *lockCounter1 = 0;
        (*SendOne1)++;
        *checkOne1 = 0;
        *unlockCounter1 = 0;
      }
    }
  } else if (lockState1 == HIGH) {
    if (*checkOne1 <= 2) {
      (*unlockCounter1)++;
      if (*unlockCounter1 > 50) {
        ////Serial.print("Unlocked");
        runStatus.publish("Unlocked");
        (*checkOne1)++;
        *SendOne1 = 0;
        *unlockCounter1 = 0;
      }
      *lockCounter1 = 0;
    }
  }
}
void washerStatusRead(int lockState1, int *SendOne1, int *checkOne1, int *lockCounter1, int *unlockCounter1) {
  if (lockState1 == LOW) {
    ////Serial.print("low");
    if (*SendOne1 <= 5) {
      (*lockCounter1)++;
      if (*lockCounter1 > 700) {
        ////Serial.print("Locked");
        runStatus.publish("Locked");
        *lockCounter1 = 0;
        (*SendOne1)++;
        *checkOne1 = 0;
        *unlockCounter1 = 0;
      }
    }
  } else if (lockState1 == HIGH) {
    if (*checkOne1 <= 2) {
      (*unlockCounter1)++;
      if (*unlockCounter1 > 50) {
        ////Serial.print("Unlocked");
        runStatus.publish("Unlocked");
        //payMachine_NC(3);
        (*checkOne1)++;
        *SendOne1 = 0;
        *unlockCounter1 = 0;
      }
      *lockCounter1 = 0;
    }
  }
}

void detergentReadStatus(int *turn_sft, int *turn_beg, int *turn_dtg, int sftnrDrop, int dtgDrop, int begDrop, int *sftnrCounter, int *begCounter, int *dtgCounter) {
  ////////////////////////////////
  ////// Softener motor turning //
  ////////////////////////////////

  if (*turn_sft == 0) {
    if (sftnrDrop == HIGH) {
      (*sftnrCounter)++;
      if (*sftnrCounter > 3) {
        *turn_sft = 1;
        *sftnrCounter = 0;
      }
    } else {
      *turn_sft = 0;
    }
  } else if (*turn_sft == 1) {
    if (sftnrDrop == LOW) {
      (*sftnrCounter)++;
      if (*sftnrCounter > 20) {
        *turn_sft = 0;
        detStatus.publish("SFTNR_DROP");
      }
    } else {
      *turn_sft = 1;
    }
  }

  /////////////////////////////////
  //// Detergent motor turning ////
  /////////////////////////////////

  if (*turn_dtg == 0) {
    if (dtgDrop == HIGH) {
      (*dtgCounter)++;
      if (*dtgCounter > 3) {
        *turn_dtg = 1;
        *dtgCounter = 0;
      }
    } else {
      *turn_dtg = 0;
    }
  } else if (*turn_dtg == 1) {
    if (dtgDrop == LOW) {
      (*dtgCounter)++;
      if (*dtgCounter > 20) {
        *turn_dtg = 0;
        detStatus.publish("DTG_DROP");
      }
    } else {
      *turn_dtg = 1;
    }
  }

  //////////////////////////////////
  ///// Beg Motor turning //////////
  //////////////////////////////////

  if (*turn_beg == 0) {
    if (begDrop == HIGH) {
      (*begCounter)++;
      if (*begCounter > 3) {
        *turn_beg = 1;
        *begCounter = 0;
      }
    } else {
      *turn_beg = 0;
    }
  } else if (*turn_beg == 1) {
    if (begDrop == LOW) {
      (*begCounter)++;
      if (*begCounter > 20) {
        *turn_beg = 0;
        detStatus.publish("BEG_DROP");
      }
    } else {
      *turn_beg = 1;
    }
  }

}

void detergentCoinCount(int coin_input1, int coin_input2, int *high1, int *high2, int *coinCounter1, int *coinCounter2) {
  ///////////////////////////////////
  ////// Coin Acceptor 1 ////////////
  ///////////////////////////////////

  if (*high1 == 0) {
    if (coin_input1 == HIGH) {
      (*coinCounter1)++;
      if (*coinCounter1 > 3) {
        *high1 = 1;
        *coinCounter1 = 0;
        ////Serial.print("One coin inserted\n");
        //coinIn.publish("1COIN");
      }
    } else {
      *high1 = 0;
    }
  } else if (*high1 == 1) {
    if (coin_input1 == LOW) {
      (*coinCounter1)++;
      if (*coinCounter1 > 10) {
        *high1 = 0;
        ////Serial.print("One coin inserted CA1\n");
        coinIn.publish("coin1");
      }
    } else {
      *high1 = 1;
    }
  }

  ///////////////////////////////////
  ////// Coin Acceptor 2 ////////////
  ///////////////////////////////////


  if (*high2 == 0) {
    if (coin_input2 == HIGH) {
      (*coinCounter2)++;
      if (*coinCounter2 > 3) {
        *high2 = 1;
        *coinCounter2 = 0;
        ////Serial.print("One coin inserted\n");
        //coinIn.publish("1COIN");
      }
    } else {
      *high2 = 0;
    }
  } else if (*high2 == 1) {
    if (coin_input2 == LOW) {
      (*coinCounter2)++;
      if (*coinCounter2 > 10) {
        *high2 = 0;
        ////Serial.print("One coin inserted CA2\n");
        coinIn.publish("coin2");
      }
    } else {
      *high2 = 1;
    }
  }
}

void billInRead(int bill_in, int *high1, int *billCounter1 ) {
  if (*high1 == 0) {
    if (bill_in == HIGH) {
      (*billCounter1)++;
      if (*billCounter1 > 3) {
        *high1 = 1;
        *billCounter1 = 0;
        ////Serial.print("One coin inserted\n");
        //coinIn.publish("1COIN");
      }
    } else {
      *high1 = 0;
    }
  } else if (*high1 == 1) {
    if (bill_in == LOW) {
      (*billCounter1)++;
      if (*billCounter1 > 10) {
        *high1 = 0;
        ////Serial.print("One coin inserted CA1\n");
        coinIn.publish("1Ringgit");
      }
    } else {
      *high1 = 1;
    }
  }
}

void checkForUpdates() {
  String mac = getMAC();
  String fwURL = String( fwUrlBase );
  fwURL.concat( mac );
  String fwVersionURL = fwURL;
  fwVersionURL.concat( ".version" );

  //Serial.println( "Checking for firmware updates." );
  //Serial.print( "MAC address: " );
  //Serial.println( mac );
  //Serial.print( "Firmware version URL: " );
  //Serial.println( fwVersionURL );

  HTTPClient httpClient;
  httpClient.begin( fwVersionURL );
  int httpCode = httpClient.GET();
  if ( httpCode == 200 ) {
    String newFWVersion = httpClient.getString();

    //Serial.print( "Current firmware version: " );
    //Serial.println( FW_VERSION );
    //Serial.print( "Available firmware version: " );
    //Serial.println( newFWVersion );

    int newVersion = newFWVersion.toInt();

    if ( newVersion > FW_VERSION ) {
      //Serial.println( "Preparing to update" );

      String fwImageURL = fwURL;
      fwImageURL.concat( ".bin" );
      t_httpUpdate_return ret = ESPhttpUpdate.update( fwImageURL );

      switch (ret) {
        case HTTP_UPDATE_FAILED:
          //Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
          break;

        case HTTP_UPDATE_NO_UPDATES:
          //Serial.println("HTTP_UPDATE_NO_UPDATES");
          break;
      }
    }
    else {
      //Serial.println( "Already on latest version" );
    }
  }
  else {
    //Serial.print( "Firmware version check failed, got HTTP response code " );
    //Serial.println( httpCode );
  }
  httpClient.end();
}

String getMAC()
{
  uint8_t mac[6];
  char result[14];
  WiFi.macAddress(mac);
  sprintf(result, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String( result );
}

boolean isValidNumber(String str) {
  boolean isNum = false;
  for (byte i = 0; i < str.length(); i++) {
    isNum = isDigit(str.charAt(i));
  }
  return isNum;
}




