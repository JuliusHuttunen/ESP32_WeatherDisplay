// This code is remixed from a bunch of different code snippets.
// Jury rigged by Julius

#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <TimeLib.h>

//display libraries
#include <GxEPD.h>
//replace this with whichever is compatible with your display (these can be found from GxEPD examples):
#include <GxGDEH029A1/GxGDEH029A1.h>

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// FreeFonts from Adafruit_GFX (Must be included as a library)
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>

GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16); // arbitrary selection of 17, 16
GxEPD_Class display(io, /*RST=*/ 16, /*BUSY=*/ 4); // arbitrary selection of (16), 4

//network credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
String openWeatherMapApiKey = "YOUR_OPENWEATHERMAP_APIKEY";

// Replace with your country code and city
String city = "Helsinki";
String countryCode = "FI";

//defining some of the variables
float temperature;
float feelslike;
String desc;
int hourOffSet = 2;
int srhours;
int sshours;
int srminutes;
int ssminutes;

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

// Variable to save current epoch time
unsigned long epochTime; 

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

// THE DEFAULT TIMER IS SET TO 10 SECONDS FOR TESTING PURPOSES
// For a final application, check the API call limits per hour/minute to avoid getting blocked/banned
// THIS DELAY (10secs) DOES NOT EXCEED THE API CALL LIMIT!
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 10 seconds (10000)
unsigned long timerDelay = 10000;

String jsonBuffer;

void setup() {
  //initialize serial and display
  Serial.begin(115200);
  display.init(115200);
  //change orientation of the screen and flush the screen
  display.setRotation(5); //display.setRotation(1) breaks the functionality, but somehow value 5 works
  display.update();
  display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT);

  //connecting esp32 to the network
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  //Time configuration parameters
  configTime(0, 0, ntpServer);
  
  Serial.println("Timer set to 10 seconds (timerDelay variable), it will take 10 seconds before publishing the first reading.");
}

void loop() {
  // Send an HTTP GET request
  if ((millis() - lastTime) > timerDelay) {
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;

      jsonBuffer = httpGETRequest(serverPath.c_str());
      JSONVar myObject = JSON.parse(jsonBuffer);

      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }

      Serial.print("JSON object = ");
      Serial.println(myObject);

      //variables displayed in serial
      //assigning values for the variables
      temperature = (double)(myObject["main"]["temp"]) - 273.15;
      desc = (JSONVar)(myObject["weather"][0]["main"]);
      feelslike = (double)(myObject["main"]["feels_like"]) - 273.15;
      int sunrise = (int)(myObject["sys"]["sunrise"]);
      srhours = hour(sunrise) + hourOffSet;
      int sunset = (int)(myObject["sys"]["sunset"]);
      sshours = hour(sunset) + hourOffSet;
      srminutes = minute(sunrise);
      ssminutes = minute(sunset);

      Serial.print(srhours);
      Serial.print(srminutes);
      Serial.print(sshours);
      Serial.print(ssminutes);
      
      Serial.print("Temperature: ");
      Serial.println(temperature);
      Serial.print("Feels like: ");
      Serial.println(feelslike);
      Serial.print("Description: ");
      Serial.println(desc);
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

  //Get epoch/unix time;
  epochTime = getTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  delay(1000);

  //displaying the stuff on the e paper, it is kind of messy!
  const char* name = "FreeMonoBold9pt7b";
  const GFXfont* f = &FreeMonoBold9pt7b;
  const GFXfont* c = &FreeMonoBold12pt7b;
  const GFXfont* b = &FreeMonoBold24pt7b;
  display.fillScreen(GxEPD_WHITE);
  display.fillRect(195 , 0, 2 ,200 ,GxEPD_BLACK);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(200, 30);
  display.println(city);
  display.setCursor(200,50);
  display.println("Sunrise");
  display.setCursor(200,65);
  display.print(srhours);
  display.print(":");
  display.println(srminutes);
  display.setCursor(200,85);
  display.println("Sunset");
  display.setCursor(200,100);
  display.print(sshours);
  display.print(":");
  display.println(ssminutes);
  display.setFont(b);
  display.setCursor(10, 40);
  display.print(temperature, 1);
  //display.print("-20.9");
  display.println("C");
  display.setFont(c);
  display.setCursor(10, 65);
  display.print("(");
  display.print(feelslike, 1);
  display.println("C)");
  display.setCursor(10,90);
  display.println(desc);
  display.setCursor(10,115);
  display.print(hour(epochTime) + hourOffSet);
  display.print(":");
  display.println(minute(epochTime));
  
  //display.update();
  display.updateWindow(0, 0, GxEPD_WIDTH, GxEPD_HEIGHT, false);
  //delay(1000);
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "{}";

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
