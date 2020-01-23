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
//#define WLAN_SSID       "antlysis_meadow_2.4G@unifi"
//#define WLAN_PASS       "!Ath4ht@w4dt!"
#define WLAN_SSID       "ideal 2nd floor"
#define WLAN_PASS       "0379805027ideal"
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
#define DTG_CA_CTRL 2     //JP6
#define DTG_CA1_IN 5       //JP8
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

#define SHIFT_IN_DATA 14     //JP1 (first two)
#define SHIFT_IN_LATCH 4      //JP3 (second two)
#define SHIFT_IN_CLOCK 16      //JP5 (third two)

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

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
//Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);
Adafruit_MQTT_Publish lastwill = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "connectivity/e15ceae8fa62a160cebe0a1644e6116c");

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish runStatus = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "Lock/e15ceae8fa62a160cebe0a1644e6116c");
Adafruit_MQTT_Publish detStatus = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "detDrop/e15ceae8fa62a160cebe0a1644e6116c");
Adafruit_MQTT_Publish coinIn = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "coinIn/e15ceae8fa62a160cebe0a1644e6116c");
Adafruit_MQTT_Publish versFeed = Adafruit_MQTT_Publish(&mqtt, MQTT_USERNAME "versionFeed/e15ceae8fa62a160cebe0a1644e6116c");
// Setup a feed called 'esp8266_led' for subscribing to changes.
Adafruit_MQTT_Subscribe receivedPayment = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "e15ceae8fa62a160cebe0a1644e6116c");
Adafruit_MQTT_Subscribe firmwareUpdate = Adafruit_MQTT_Subscribe(&mqtt, MQTT_USERNAME "firmwareUpdate");

////////////////////////////////////
//////// SOME VARIABLE INIT ////////
////////////////////////////////////

unsigned int sftnrCounter = 0;
unsigned int sftnrDrop = 0;
unsigned int turn_sft = 0;
unsigned int dtgCounter1 = 0;
unsigned int dtgCounter2 = 0;
unsigned int dtgDrop = 0;
unsigned int turn_dtg1 = 0;
unsigned int turn_dtg2 = 0;
unsigned int dsCounter1 = 0;
unsigned int dsDrop = 0;
unsigned int turn_drySoft = 0;
unsigned int blchCounter = 0;
unsigned int blchDrop = 0;
unsigned int turn_blch2 = 0;
unsigned int begCounter = 0;
unsigned int begDrop = 0;
unsigned int turn_beg = 0;
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
boolean cneted = true;
unsigned long previousMillis = 0;
const long interval = 5000;
unsigned int register_value = 0;
// ipsoWasher | ipsoDryer | dexWasher | dexDryerSingle | dexDryerDouble | detergent | billAcceptor
String machineType = String("detergent");

////////////////////////////////////
//////// TIME AND INTERRUPT ////////
////////////////////////////////////
// start the timer for the heartbeat
os_timer_t myHeartBeat;
os_timer_t IOReadout;
os_timer_t ShiftRegister;

void timerCallback(void *pArg) {
  //////Serial.print("timer is functioning\b");
  lastwill.publish("ON");
}
void IOtimerISR(void *pArg) {
  //////Serial.print("timer is functioning\b");
  if (machineType == "ipsoWasher") {
    lockState1 =  digitalRead(IPSO_W_STATUS);
    coin_input1 = digitalRead(IPSO_W_IN);
    coinCount2(coin_input1, &coinCounter1, &high1, &start);
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
    coinCount(coin_input1, &coinCounter1, &high1, &start);
  }
}

unsigned int shift_in(void) {
  unsigned int byte = 0;
  unsigned int pin_value;

  digitalWrite(SHIFT_IN_LATCH, LOW);
  delayMicroseconds(200);
  digitalWrite(SHIFT_IN_LATCH, HIGH);
  delayMicroseconds(200);

  for(int i=0; i<8; i++) {
      pin_value = digitalRead(SHIFT_IN_DATA);  
      byte |= (pin_value << ((8 - 1) - i));

//      Serial.print(pin_value, BIN);
//      Serial.println(" ");
//      delay(1000);
      /* Pulse clock to write next bit. */
      digitalWrite(SHIFT_IN_CLOCK, LOW);
      delayMicroseconds(500);
      digitalWrite(SHIFT_IN_CLOCK, HIGH);
      delayMicroseconds(500);
  }
    return byte;
}

void ShiftRegisterISR(void *pArg) {
   /* Read in first 74HC165 data. */
   register_value = shift_in();
   Serial.print(register_value, BIN);
   Serial.println(" ");
}

void timerInit(void) {
  os_timer_setfn(&myHeartBeat, timerCallback, NULL);
  os_timer_arm(&myHeartBeat, 30000, true);

  os_timer_setfn(&IOReadout, IOtimerISR, NULL);
  os_timer_arm(&IOReadout, 5, true);

  os_timer_setfn(&ShiftRegister, ShiftRegisterISR, NULL);
  os_timer_arm(&ShiftRegister, 15, true);
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
    pinMode(SHIFT_IN_CLOCK, OUTPUT);
    pinMode(SHIFT_IN_LATCH, OUTPUT);
    pinMode(SHIFT_IN_DATA, INPUT);
    digitalWrite(SHIFT_IN_LATCH, HIGH);
    digitalWrite(SHIFT_IN_CLOCK, HIGH);
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
  mqtt.will(MQTT_USERNAME "connectivity/e15ceae8fa62a160cebe0a1644e6116c", "OFF");
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
  MQTT_connect(&cneted);
  if (!donefw && cneted) {
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
    coin_input1 = digitalRead(DTG_CA1_IN);
    coin_input2 = digitalRead(DTG_CA2_IN);
    //bill_input = digitalRead(BILL_IN);
    detergentCoinCount(coin_input1, coin_input2, &high1, &high2, &coinCounter1, &coinCounter2);
    DetReadStatus(&register_value, &turn_sft, &turn_beg, &turn_drySoft, &turn_blch2,  &turn_dtg1, &turn_dtg2, &sftnrCounter, &begCounter, &dtgCounter1, &dtgCounter2, &dsCounter1, &blchCounter);
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

void MQTT_connect(boolean *cneted) {
  int8_t ret;
  int mycount;
  mycount = 0;
  // Stop if already connected.
  if (mqtt.connected()) {
    *cneted = true;
    ////Serial.println("MQTT Connected!");
    return;
  }
  *cneted = false;
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
  //Serial.println("MQTT Connected!");
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
      if (*coinLow > 30) {
        *coinHigh = 0;
      }
      *high = 0;
    }
  } else if (*high == 1) {
    if (coin_input == LOW) {
      //Serial.print("coin low");
      (*coinLow)++;
      //Serial.println(*coinLow);
      if (*coinLow > 30) {
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

void coinCount2(int coin_input, int *coinHigh, int *high, int *coinLow) {
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
    if (coin_input1 == LOW) {
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
    if (coin_input1 == HIGH) {
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
    if (coin_input2 == LOW) {
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
    if (coin_input2 == HIGH) {
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

void DetReadStatus(unsigned int *shiftReg, unsigned int *turn_sft, unsigned int *turn_beg, unsigned int *turn_drySoft, unsigned int *turn_bleach,  unsigned int *turn_dtg1, unsigned int *turn_dtg2, unsigned int *sftnrCounter, unsigned int *begCounter, unsigned int *dtgCounter1, unsigned int *dtgCounter2, unsigned int *dsCounter1, unsigned int *blchCounter) {
  unsigned int compSoft =            *shiftReg & 128;
  unsigned int compOxyBleach =       *shiftReg & 64; //done
  unsigned int compDrySoft =         *shiftReg & 32; //done
  unsigned int compLB =               *shiftReg & 16; //done
  unsigned int compDet1 =            *shiftReg & 8; //done
  unsigned int compDet2 =              *shiftReg & 4;
  //unsigned int compbill =      *shiftReg & 0b00000010;
  //delay(1000);
  //// Compare first slot for Det 1 ////////
  //detStatus.publish(*shiftReg);
  if (*turn_dtg1 == 0) {
    if (compDet1 == 8) {
      (*dtgCounter1)++;
      if (*dtgCounter1 > 3) {
        *turn_dtg1 = 1;
        *dtgCounter1 = 0;
      }
    } else {
      *turn_dtg1 = 0;
    }
  } else if (*turn_dtg1 == 1) {
    if (compDet1 == 0) {
      (*dtgCounter1)++;
      if (*dtgCounter1 >20) {
        *turn_dtg1 = 0;
        detStatus.publish("DTG_DROP1");
      } 
    } else {
      *turn_dtg1 = 1;
    }
  }

//// Compare second slot for Det 2 ////////
  
  if (*turn_dtg2 == 0) {
    if (compDet2 == 4) {
      (*dtgCounter2)++;
      if (*dtgCounter2 > 3) {
        *turn_dtg2 = 1;
        *dtgCounter2 = 0;
      }
    } else {
      *turn_dtg2 = 0;
    }
  } else if (*turn_dtg2 == 1) {
    if (compDet2 == 0) {
      (*dtgCounter2)++;
      if (*dtgCounter2 >20) {
        *turn_dtg2 = 0;
        detStatus.publish("DTG_DROP2");
      } 
    } else {
      *turn_dtg2 = 1;
    }
  }

//// Compare third slot for Softener ////////

  if (*turn_sft == 0) {
    if (compSoft == 128) {
      (*sftnrCounter)++;
      if (*sftnrCounter > 3) {
        *turn_sft = 1;
        *sftnrCounter = 0;
      }
    } else {
      *turn_sft = 0;
    }
  } else if (*turn_sft == 1) {
    if (compSoft == 0) {
      (*sftnrCounter)++;
      if (*sftnrCounter >20) {
        *turn_sft = 0;
        detStatus.publish("SFT_DROP");
      } 
    } else {
      *turn_sft = 1;
    }
  }
  
//// Compare 4th slot for Bleach 1 ////////
  
  if (*turn_drySoft == 0) {
    if (compDrySoft == 32) {
      (*dsCounter1)++;
      if (*dsCounter1 > 3) {
        *turn_drySoft = 1;
        *dsCounter1 = 0;
      }
    } else {
      *turn_drySoft = 0;
    }
  } else if (*turn_drySoft == 1) {
    if (compDrySoft == 0) {
      (*dsCounter1)++;
      if (*dsCounter1 >20) {
        *turn_drySoft = 0;
        detStatus.publish("DrYSOFT_DROP");
      } 
    } else {
      *turn_drySoft = 1;
    }
  }

//// Compare 5th slot for Bleach 2 ////////
  
  if (*turn_bleach == 0) {
    if (compOxyBleach == 64) {
      (*blchCounter)++;
      if (*blchCounter > 3) {
        *turn_bleach = 1;
        *blchCounter = 0;
      }
    } else {
      *turn_bleach = 0;
    }
  } else if (*turn_bleach == 1) {
    if (compOxyBleach == 0) {
      (*blchCounter)++;
      if (*blchCounter >20) {
        *turn_bleach = 0;
        detStatus.publish("OXY_DROP"); 
      } 
    } else {
      *turn_bleach = 1;
    }
  }

//// Compare 6th slot for LB ////////

  if (*turn_beg == 0) {
    if (compLB == 16) {
      (*begCounter)++;
      if (*begCounter > 3) {
        *turn_beg = 1;
        *begCounter = 0;
      }
    } else {
      *turn_beg = 0;
    }
  } else if (*turn_beg == 1) {
    if (compLB == 0) {
      (*begCounter)++;
      if (*begCounter >20) {
        *turn_beg = 0;
        detStatus.publish("BEG_DROP");
      } 
    } else {
      *turn_beg = 1;
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
