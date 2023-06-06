/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 by Daniel Eichhorn
 * Copyright (c) 2016 by Fabrice Weinberg
 * Modified by Kevin Dolan on 2018
 * Original File: SSD1306UiDemo.ino
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "SSD1306.h"

#include "OLEDDisplayUi.h"

// Include custom images
#include "images.h"

#include "ArduinoJson.h"

#include "WiFi.h"

#include "HTTPClient.h"


const char *SSID = "****"; // only you know
const char *password = "****";


// #include "WiFi_Login.h"

#include "esp_sleep.h"

#include "esp_wifi.h"


const byte TOUCH_PIN = 12;

bool connected = false;

HTTPClient http;

// Initialize the OLED display using Wire library
//sda pin 4, scl pin 15

SSD1306  display(0x3c, 4, 15, GEOMETRY_128_64);

OLEDDisplayUi ui     ( &display );

const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 100; //Set size of the Json object
const String cryptoCompare = "https://min-api.cryptocompare.com/data/pricemultifull?fsyms=";

const String exchange = "Coinbase";
const uint8_t numOfCoins = 3;
String coinNames[numOfCoins] = {"BTC","ETH","DOGE"};

struct cryptoCoin{
	String name;
	String price;
	String hr_percent_change;
	String day_percent_change;
};

//https://min-api.cryptocompare.com/data/pricemulti?fsyms=BTC,ETH&tsyms=USD&e=Coinbase&extraParams=your_app_name
class cryptoCoins{
		
	cryptoCoin coins[numOfCoins];

	public:
	bool updating;

	cryptoCoins(){		
		for(int x=0; x < numOfCoins; x++) coins[x] =  cryptoCoin{coinNames[x],"","",""};
		this->updating = false;		
	}	
  bool update(){

	  // Connect to WiFi
	  this->updating = true;
	  bool success = false;	  
	  if(WiFi.status() != WL_CONNECTED)  WiFi.begin(SSID, password);
	  	  
	  while (WiFi.status() != WL_CONNECTED) {
	    delay(1000);
	  	Serial.println("Connecting to WiFi..");
	  }

	  Serial.println("Connected to SSID: " + WiFi.SSID());	  	  
	  String site = cryptoCompare;

	  for(int x=0; x < numOfCoins; x++) site += coinNames[x] + ",";
	  site += "&tsyms=USD";


	  site += "&e=";
	  site += exchange;


	  http.begin(site);
	  int httpCode = http.GET();

	  if (httpCode > 0) { //Check for the returning code

	        success = true;
	        String payload = http.getString();
		    
		    http.end(); //Free the resources
		    WiFi.disconnect(true);
		  	WiFi.mode(WIFI_OFF);
	       // Serial.println(httpCode);
	       // Serial.println(payload);

	    // Parse JSON object
	  	DynamicJsonBuffer jsonBuffer(capacity);
	 	JsonObject& root = jsonBuffer.parseObject(payload);
		  if (!root.success()) {
		    Serial.println(F("Parsing failed!"));		    
			}	
		

		/*Serial.print(F("BTC: "));
		Serial.println(root["USD"].as<char*>());
		Serial.print(F("ETH: "));	
		Serial.println(root["ETH"].as<char*>()); */

		for(int x=0; x < numOfCoins; x++) 	{
			this->coins[x].price = root["DISPLAY"][coinNames[x]]["USD"]["PRICE"].as<char*>();   
			this->coins[x].day_percent_change = root["DISPLAY"][coinNames[x]]["USD"]["CHANGEPCT24HOUR"].as<char*>();

 			}

	    }

	    else {
	      //Serial.println("Error on HTTP request");
	      http.end(); //Free the resources
		  WiFi.disconnect(true);
		  WiFi.mode(WIFI_OFF);	      
	    }

	  	this->updating = false;  	
	    return success;
	}
	cryptoCoin* getCoin(int coinIndex){
		return &this->coins[coinIndex];
	}

};
cryptoCoins crypto;

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis() / 1000));
  
  display->setTextAlignment(TEXT_ALIGN_LEFT);   
  if(crypto.updating)	display->drawString(0, 0, "Updating");
  //Debug for the touch button
  //if(digitalRead(TOUCH_PIN))	display->drawString(0, 0, "Touched");
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 10 + y, "ESP32-OLED Crypto Ticker"); 	

}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 10 + y,"BTC: " + crypto.getCoin(0)->price);
  display->drawString(0 + x, 30 + y,"24hr: " + crypto.getCoin(0)->day_percent_change  + "%");

}
void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 10 + y,"ETH: " + crypto.getCoin(1)->price);
  display->drawString(0 + x, 30 + y,"24hr: " + crypto.getCoin(1)->day_percent_change + "%");

}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 10 + y,"DOGE: " + crypto.getCoin(2)->price);
  display->drawString(0 + x, 30 + y,"24hr: " + crypto.getCoin(2)->day_percent_change  + "%");

}

//void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
//  display->setTextAlignment(TEXT_ALIGN_LEFT);
//  display->setFont(ArialMT_Plain_16);
//  display->drawString(0 + x, 10 + y,"BNB: " + crypto.getCoin(3)->price);
//  display->drawString(0 + x, 30 + y,"24hr: " + crypto.getCoin(3)->day_percent_change  + "%");

//}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 16 + y, 128, "Connected to SSID: " + WiFi.SSID());
}

// void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5};

// how many frames are there?
int frameCount = 5;

//Remembers the last frame before it went to sleep.
RTC_DATA_ATTR uint8_t prevFrame = 0;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

//Touch interrupt setup

volatile int interruptCounter = 0;
int numberOfInterrupts = 0;


portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

//Interrupt handler for the touch button
void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

const uint8_t OLED_RESET_PIN = 16;


TaskHandle_t update_screen_handle = NULL;

void setup() {

  WiFi.begin(SSID, password);

	//Reset the OLED
  pinMode(OLED_RESET_PIN,OUTPUT);
  digitalWrite(OLED_RESET_PIN, LOW); // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(OLED_RESET_PIN, HIGH); // while OLED is running, must set GPIO16 in high

  //Setup the touch button
  pinMode(TOUCH_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(TOUCH_PIN), handleInterrupt, RISING);  

  //Initialize the serial terminal
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  //Lower the FPS to reduce power?
  ui.setTargetFPS(30);

	// Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);
  
  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  // display.flipScreenVertically();

  ui.setTimePerFrame(10000);

  //ui.disableAutoTransition();

  ui.switchToFrame(prevFrame);

  Serial.println("Initialized display");  

  //Create a new task to update the screen
  xTaskCreatePinnedToCore(
                    update_screen,   /* Function to implement the task */
                    "update_screen", /* Name of the task */
                    2000,      /* Stack size in words */
                    NULL,       /* Task input parameter */
                    1,          /* Priority of the task */
                    &update_screen_handle,       /* Task handle. */
                    0);			/* Core number */
    
    crypto = cryptoCoins();
    crypto.update();    
   
}
int cryptoUpdate = 0;

const int updateInterval = 12000; //10 second update
const int sleepTimeout = 1000000; //20 second shutoff

volatile int lastButtonPush;

void sleep(){
	//Save info before going to sleep
	prevFrame = ui.getUiState()->currentFrame;

	//delete the update_screen task
	if( update_screen_handle != NULL ) vTaskDelete( update_screen_handle );    

	display.end();

	esp_wifi_stop();

	digitalWrite(OLED_RESET_PIN, LOW); //Turn off OLED display. I'm not sure if this is necessary since the pins go floating during deep sleep (I think)		
	esp_sleep_enable_ext0_wakeup((gpio_num_t) TOUCH_PIN,1);
	esp_deep_sleep_start();

}
void loop() {
 
  if(millis() - cryptoUpdate > updateInterval && !(millis() - lastButtonPush > updateInterval)){
	  
  	  cryptoUpdate = millis();  	  
  	  crypto.update();	  
  }	
  //if(millis() - lastButtonPush > sleepTimeout){
	  
	//  sleep();
  //}	
}
//Infinite loop to update the screen and handle the touch button. Run as a task.
void update_screen(void* arg){

	while(true){
	int remainingTimeBudget = ui.update();

	    // You can do some work here
	    // Don't do stuff if you are below your
	    // time budget.

		//Handle touch button interrupts	
	  if (remainingTimeBudget > 0) {
		  if(interruptCounter>0){

		      portENTER_CRITICAL(&mux);
		      interruptCounter--;
		      portEXIT_CRITICAL(&mux);
		  	  lastButtonPush = millis();
		  	  
		  	  ui.nextFrame();

		      numberOfInterrupts++;
		      //Button Debuging
		      //Serial.print("An interrupt has occurred. Total: ");
		      //Serial.println(numberOfInterrupts);
		  }  	


	    delay(remainingTimeBudget);
    	}
    	//Used to check if the stack is full
    	//int highMark = uxTaskGetStackHighWaterMark(NULL);
    	//Serial.println("Remaining stack: " + String(highMark));
	}
}
