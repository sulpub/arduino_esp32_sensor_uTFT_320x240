// Demo based on:
// UTFT_Demo_320x240 by Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
/*

  This sketch uses the GLCD and font 2 only.

  Make sure all the display driver and pin comnenctions are correct by
  editting the User_Setup.h file in the TFT_eSPI library folder.

  #########################################################################
  ###### DON'T FORGET TO UPDATE THE User_Setup.h FILE IN THE LIBRARY ######
  #########################################################################
*/
// Include the libraries we need
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 0

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);


#include "SPI.h"

#include "TFT_eSPI.h"

#define TFT_GREY 0x7BEF

TFT_eSPI myGLCD = TFT_eSPI(320,240);       // Invoke custom library

const long interval_send_ble  = 20;     //7:new smartphone 20:old smartphone millis OK
const long interval_poll_ble  = 1000;   //1000 millis OK

int delay_wait = 2000;

String string_texte = "                         ";
float float_temperature = 0;
float yy=0;

unsigned long runTime = 0;

static long currentMillis_send_ble = 0;
static long currentMillis_imu = 0;
static long currentMillis_poll_ble = 0;
static long currentMillis_tof = 0;
static long currentMillis_pressure = 0;
static long millis_measure_cycles = 0;
static long millis_measure_cycles_old = 0;

String input_json = "{\"Freq\":250 ,\"AccRange\":2  ,\"GyrRange\":250 }          ";
String output     = "{\"Freq\":250 ,\"AccRange\":2  ,\"GyrRange\":250 }          ";




class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
    }
};



void setup()
{
  // start serial port
  Serial.begin(115200);

  randomSeed(analogRead(A0));
  // Setup the LCD
  myGLCD.init();
  myGLCD.setRotation(2);

  // Start up the library
  sensors.begin();

  //BLE init
  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop()
{
  //BLE
  //**********************************
      if (deviceConnected) {
        pTxCharacteristic->setValue(&txValue, 1);
        pTxCharacteristic->notify();
        txValue++;
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }




  //update screen
  //**********************************
  randomSeed(millis());
  //randomSeed(1234); // This ensure test is repeatable with exact same draws each loop
  int buf[318];
  int x, x2;
  int y, y2;
  int r;
  runTime = millis();
  // Clear the screen and draw the frame
  myGLCD.fillScreen(TFT_BLACK);


  myGLCD.fillRect(0, 0, 319, 14, TFT_RED);

  myGLCD.fillRect(0, 226, 319, 14, TFT_GREY);

  myGLCD.setTextColor(TFT_BLACK, TFT_RED);
  myGLCD.drawCentreString("* Test Sensor *", 160, 4, 1);
  myGLCD.setTextColor(TFT_YELLOW, TFT_GREY);
  myGLCD.drawCentreString("Temperature DS18B20", 160, 228, 1);

  myGLCD.drawRect(0, 14, 319, 211, TFT_BLUE);

  // Draw crosshairs
  myGLCD.drawLine(159, 15, 159, 224, TFT_BLUE);
  myGLCD.drawLine(1, 119, 318, 119, TFT_BLUE);
  for (int i = 9; i < 310; i += 10)
    myGLCD.drawLine(i, 117, i, 121, TFT_BLUE);
  for (int i = 19; i < 220; i += 10)
    myGLCD.drawLine(157, i, 161, i, TFT_BLUE);

  // Draw sin-, cos- and tan-lines
  /*
    myGLCD.setTextColor(TFT_CYAN);
    myGLCD.drawString("Sin", 5, 15,2);
    for (int i=1; i<318; i++)
    {
      myGLCD.drawPixel(i,119+(sin(((i*1.13)*3.14)/180)*95),TFT_CYAN);
    }
    myGLCD.setTextColor(TFT_RED);
    myGLCD.drawString("Cos", 5, 30,2);
    for (int i=1; i<318; i++)
    {
      myGLCD.drawPixel(i,119+(cos(((i*1.13)*3.14)/180)*95),TFT_RED);
    }
    myGLCD.setTextColor(TFT_YELLOW);
    myGLCD.drawString("Tan", 5, 45,2);
    for (int i=1; i<318; i++)
    {
      myGLCD.drawPixel(i,119+(tan(((i*1.13)*3.14)/180)),TFT_YELLOW);
    }

    delay(delay_wait);

    myGLCD.fillRect(1,15,317,209,TFT_BLACK);

    myGLCD.drawLine(159, 15, 159, 224,TFT_BLUE);
    myGLCD.drawLine(1, 119, 318, 119,TFT_BLUE);

  */
  int col = 0;
  // Draw a moving sinewave
  x = 1;
  int i = 1;
  //for (int i=1; i<(317*20); i++)
  while (1)
  {
      delay(10);
    x++;
    if (x == 318)
    {
      x = 1;
      i = 4000;
    }

    if (i > 318)
    {
      i = 400;
      if ((x == 159) || (buf[x - 1] == 119))
        col = TFT_BLUE;
      else
        myGLCD.drawPixel(x, buf[x - 1], TFT_BLACK);
    }

    //y=119+(sin(((i*1.1)*3.14)/180)*(90-(i / 100)));
    //y sensor DS18B20 maxim temperature onewire
    sensors.requestTemperatures(); // Send the command to get temperatures
    //Serial.print("Temperature for the device 1 (index 0) is: ");
    //Serial.print("Temperature:");
    float_temperature = sensors.getTempCByIndex(0);

    //dummy for test
    yy++;
    float_temperature = random(5, 10)+30*sin(yy*3.14/180);
    Serial.println(float_temperature);
    //plage 240
    y = (119 - float_temperature * 2);

    myGLCD.drawPixel(x, y, TFT_RED);
    myGLCD.drawPixel(x, buf[x - 317], TFT_BLUE);

    delay(2);
    buf[x - 1] = y;

    //infos
    myGLCD.setTextColor(TFT_YELLOW, TFT_BLACK);
    //myGLCD.drawCentreString("                    ", 0, 20,2);
    myGLCD.drawString("T: ", 10, 20, 2);
    myGLCD.drawString(String(float_temperature, 2) + "       ", 30, 20, 2);
    //myGLCD.drawNumber(float_temperature, 20, 70,2);
  }

  delay(delay_wait);

  myGLCD.fillRect(1, 15, 317, 209, TFT_BLACK);

  // Draw some filled rectangles
  for (int i = 1; i < 6; i++)
  {
    switch (i)
    {
      case 1:
        col = TFT_MAGENTA;
        break;
      case 2:
        col = TFT_RED;
        break;
      case 3:
        col = TFT_GREEN;
        break;
      case 4:
        col = TFT_BLUE;
        break;
      case 5:
        col = TFT_YELLOW;
        break;
    }
    myGLCD.fillRect(70 + (i * 20), 30 + (i * 20), 60, 60, col);
  }

  delay(delay_wait);

  myGLCD.fillRect(1, 15, 317, 209, TFT_BLACK);

  // Draw some filled, rounded rectangles
  for (int i = 1; i < 6; i++)
  {
    switch (i)
    {
      case 1:
        col = TFT_MAGENTA;
        break;
      case 2:
        col = TFT_RED;
        break;
      case 3:
        col = TFT_GREEN;
        break;
      case 4:
        col = TFT_BLUE;
        break;
      case 5:
        col = TFT_YELLOW;
        break;
    }
    myGLCD.fillRoundRect(190 - (i * 20), 30 + (i * 20), 60, 60, 3, col);
  }

  delay(delay_wait);

  myGLCD.fillRect(1, 15, 317, 209, TFT_BLACK);

  // Draw some filled circles
  for (int i = 1; i < 6; i++)
  {
    switch (i)
    {
      case 1:
        col = TFT_MAGENTA;
        break;
      case 2:
        col = TFT_RED;
        break;
      case 3:
        col = TFT_GREEN;
        break;
      case 4:
        col = TFT_BLUE;
        break;
      case 5:
        col = TFT_YELLOW;
        break;
    }
    myGLCD.fillCircle(100 + (i * 20), 60 + (i * 20), 30, col);
  }

  delay(delay_wait);

  myGLCD.fillRect(1, 15, 317, 209, TFT_BLACK);

  // Draw some lines in a pattern

  for (int i = 15; i < 224; i += 5)
  {
    myGLCD.drawLine(1, i, (i * 1.44) - 10, 223, TFT_RED);
  }

  for (int i = 223; i > 15; i -= 5)
  {
    myGLCD.drawLine(317, i, (i * 1.44) - 11, 15, TFT_RED);
  }

  for (int i = 223; i > 15; i -= 5)
  {
    myGLCD.drawLine(1, i, 331 - (i * 1.44), 15, TFT_CYAN);
  }

  for (int i = 15; i < 224; i += 5)
  {
    myGLCD.drawLine(317, i, 330 - (i * 1.44), 223, TFT_CYAN);
  }

  delay(delay_wait);


  myGLCD.fillRect(1, 15, 317, 209, TFT_BLACK);

  // Draw some random circles
  for (int i = 0; i < 100; i++)
  {
    x = 32 + random(256);
    y = 45 + random(146);
    r = random(30);
    myGLCD.drawCircle(x, y, r, random(0xFFFF));
  }

  delay(delay_wait);

  myGLCD.fillRect(1, 15, 317, 209, TFT_BLACK);

  // Draw some random rectangles
  for (int i = 0; i < 100; i++)
  {
    x = 2 + random(316);
    y = 16 + random(207);
    x2 = 2 + random(316);
    y2 = 16 + random(207);
    if (x2 < x) {
      r = x; x = x2; x2 = r;
    }
    if (y2 < y) {
      r = y; y = y2; y2 = r;
    }
    myGLCD.drawRect(x, y, x2 - x, y2 - y, random(0xFFFF));
  }

  delay(delay_wait);


  myGLCD.fillRect(1, 15, 317, 209, TFT_BLACK);

  // Draw some random rounded rectangles
  for (int i = 0; i < 100; i++)
  {
    x = 2 + random(316);
    y = 16 + random(207);
    x2 = 2 + random(316);
    y2 = 16 + random(207);
    // We need to get the width and height and do some window checking
    if (x2 < x) {
      r = x; x = x2; x2 = r;
    }
    if (y2 < y) {
      r = y; y = y2; y2 = r;
    }
    // We need a minimum size of 6
    if ((x2 - x) < 6) x2 = x + 6;
    if ((y2 - y) < 6) y2 = y + 6;
    myGLCD.drawRoundRect(x, y, x2 - x, y2 - y, 3, random(0xFFFF));
  }

  delay(delay_wait);

  myGLCD.fillRect(1, 15, 317, 209, TFT_BLACK);

  //randomSeed(1234);
  int colour = 0;
  for (int i = 0; i < 100; i++)
  {
    x = 2 + random(316);
    y = 16 + random(209);
    x2 = 2 + random(316);
    y2 = 16 + random(209);
    colour = random(0xFFFF);
    myGLCD.drawLine(x, y, x2, y2, colour);
  }

  delay(delay_wait);

  myGLCD.fillRect(1, 15, 317, 209, TFT_BLACK);

  // This test has been modified as it takes more time to calculate the random numbers
  // than to draw the pixels (3 seconds to produce 30,000 randoms)!
  for (int i = 0; i < 10000; i++)
  {
    myGLCD.drawPixel(2 + random(316), 16 + random(209), random(0xFFFF));
  }

  // Draw 10,000 pixels to fill a 100x100 pixel box
  // use the coords as the colour to produce the banding
  //byte i = 100;
  //while (i--) {
  //  byte j = 100;
  //  while (j--) myGLCD.drawPixel(i+110,j+70,i+j);
  //  //while (j--) myGLCD.drawPixel(i+110,j+70,0xFFFF);
  //}
  delay(delay_wait);

  myGLCD.fillScreen(TFT_BLUE);
  myGLCD.fillRoundRect(80, 70, 239 - 80, 169 - 70, 3, TFT_RED);

  myGLCD.setTextColor(TFT_WHITE, TFT_RED);
  myGLCD.drawCentreString("That's it!", 160, 93, 2);
  myGLCD.drawCentreString("Restarting in a", 160, 119, 2);
  myGLCD.drawCentreString("few seconds...", 160, 132, 2);

  runTime = millis() - runTime;
  myGLCD.setTextColor(TFT_GREEN, TFT_BLUE);
  myGLCD.drawCentreString("Runtime: (msecs)", 160, 210, 2);
  myGLCD.setTextDatum(TC_DATUM);
  myGLCD.drawNumber(runTime, 160, 225, 2);
  delay (5000);
}
