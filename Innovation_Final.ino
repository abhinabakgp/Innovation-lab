#include <Wire.h>
#include <LCD-I2C.h>
#include <DHT.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>

String apiKey = "5D0AH3BF0L22P210";     //  Enter your Write API key from ThingSpeak
 
const char *ssid_global =  "Sanjoy15";     // replace with your wifi ssid and wpa2 key
const char *pass_global =  "87654321";
const char* server_global = "api.thingspeak.com";
 
WiFiClient client;

// Initialize the LCD (I2C address is usually 0x27 or 0x3F)
LCD_I2C lcd(0x27, 16, 2);

// DHT11 sensor settings
#define DHTPIN D3        // Pin connected to DHT11 data pin (GPIO2)
#define DHTTYPE DHT11    // Define the type of DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// MQ135 sensor pin
#define MQ135_PIN A0     // Analog pin for MQ135 sensor

// Buzzer pin
#define BUZZER_PIN D5    // Pin connected to the buzzer (GPIO14)

//LED PIN
#define LED_PIN_1 D6    // Pin connected to the LED
#define LED_PIN_2 D7
# define LED_PIN_3 D8
// -------------------------------------------------------------------------------------------

// Replace with your desired Access Point credentials
const char* ssid = "Air Quality Monitoring";  // This is the name of the Wi-Fi network you create
const char* password = "87654321";       // Password for the Wi-Fi network

float t = 0.0;
float h = 0.0;
float ppm = 0.0; // Added variable for PPM

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 1000;  

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
  
    body {
      font-family: Arial, sans-serif;
      background-color: #f4f4f9;
      margin: 0;
      padding: 0;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
    }
    .container {
      background-color: #ffffff;
      border-radius: 10px;
      box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
      padding: 20px;
      max-width: 400px;
      width: 100%;
      text-align: center;
    }
    h2 {
      color: #4CAF50;
      margin-bottom: 10px;
    }
    h3 {
      color: #555555;
      font-size: 1.2em;
      margin-bottom: 20px;
    }
    p {
      font-size: 1.1em;
      color: #333333;
      margin: 10px 0;
    }
    span {
      font-weight: bold;
      color: #2196F3;
      font-size: 1.3em;
    }
    .info-block {
      margin-bottom: 15px;
      padding: 10px;
      background-color: #f9f9f9;
      border-radius: 5px;
      box-shadow: 0 2px 2px rgba(0, 0, 0, 0.05);
    }

  </style>
</head>
<body>
  <div class="container">
    <h2>Air Quality Monitoring System</h2>
    <h3>Abhinaba, Sanjay, Arpon, Arnab</h3>
    
    <div class="info-block">
      <p>Temperature</p>
      <span id="temperature">%TEMPERATURE%</span>
    </div>
    
    <div class="info-block">
      <p>PPM (Parts per Million):</p>
      <span id="ppm">%PPM%</span>
    </div>
    
    <div class="info-block">
      <p>Humidity</p>
      <span id="humidity">%HUMIDITY%</span>
    </div>
  </div>
</body>

<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("ppm").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/ppm", true);
  xhttp.send();
}, 1000 ) ;

</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  if(var == "TEMPERATURE"){
    return String(t);
  }
  else if(var == "HUMIDITY"){
    return String(h);
  }
  else if(var == "PPM"){ // Added PPM processing
    return String(ppm);
  }
  return String();
}

void setup() {
  // Initialize the LCD
  Wire.begin();
  lcd.begin(&Wire);
  lcd.display();
  lcd.backlight();

  pinMode(LED_PIN_1,OUTPUT);
  pinMode(LED_PIN_2,OUTPUT);
  pinMode(LED_PIN_3,OUTPUT);
  // Initialize the DHT sensor
  dht.begin();
  
  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer initially
  Serial.print("stops here");
  // Display a startup message
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(2000);
  lcd.clear();
  Serial.print("stops here 2");
  // Serial port for debugging purposes
  Serial.begin(115200);
  dht.begin();
  Serial.print("stops here 3");
  // Set up the ESP8266 as an access point
  WiFi.softAP(ssid, password);
  Serial.print(ssid);
  Serial.println(password);
  
  // Print the IP address of the ESP8266 AP
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Access Point IP: ");
  Serial.println(IP);

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(t).c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(h).c_str());
  });
  server.on("/ppm", HTTP_GET, [](AsyncWebServerRequest *request){ // Added route for PPM
    request->send_P(200, "text/plain", String(ppm).c_str());
  });

  // Start server
  server.begin();



  // --------------------------------------------------------------------------------------------------------

   Serial.begin(115200);
       delay(10);
       dht.begin();
       Serial.println("Connecting to ");
       
       WiFi.begin(ssid_global, pass_global);
 
    //   while (WiFi.status() != WL_CONNECTED) 
    //  {
    //         delay(500);
    //         Serial.print(".");
    //  }






    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      lcd.setCursor(0, 0);
      lcd.print("Global Server ");
      lcd.setCursor(0, 1);
      lcd.print("Connecting ... ");
      delay(8000);
      lcd.clear();
      delay(500);
      if (WiFi.status() == WL_CONNECTED) {
      delay(500);
      lcd.setCursor(0, 0);
      lcd.print("Global Server ");
      lcd.setCursor(0, 1);
      lcd.print("Connected");
      delay(1500);
      lcd.clear();
      delay(500);
      }
      else{
      delay(500);
      lcd.setCursor(0, 0);
      lcd.print("Global Server ");
      lcd.setCursor(0, 1);
      lcd.print("Did not Connect");
      delay(1500);
      lcd.clear();
      delay(500);
      break;
      }
      lcd.clear();
    }




    // else {
    //   delay(500);
    //   lcd.setCursor(0, 0);
    //   lcd.print("Global Server ");
    //   lcd.setCursor(0, 1);
    //   lcd.print("Connected ...");
    //   delay(5000);
    // }
      Serial.println("");
      Serial.println("WiFi connected");

  // .......................................................................................................
}



void loop() {
  // Read temperature and humidity from DHT11
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature(); // Reading temperature in Celsius

  // Check if readings from DHT11 are valid
  if (isnan(humidity) || isnan(temperature)) {
    lcd.setCursor(0, 0);
    lcd.print("Sensor Error");
  } else {
    // Read air quality from MQ135 sensor
    int mq135Value = analogRead(MQ135_PIN);
    ppm = (mq135Value / 1024.0) * 1000;  // Approximate PPM calculation

    // Check PPM and activate buzzer if exceeds 400
    if (ppm > 700) {
      digitalWrite(BUZZER_PIN, HIGH); // Turn on buzzer
      // digitalWrite(LED_PIN, HIGH);
      //delay(70);
    } 
    else {
      digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer
      // digitalWrite(LED_PIN, HIGH);
    }

    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("Hum: ");
    lcd.print(humidity);
    lcd.print(" %");
    delay(2000);

    lcd.clear();
    delay(50);
    lcd.setCursor(0, 0);
    lcd.print("PPM: ");
    lcd.print(ppm);

    if(ppm < 500){
    lcd.setCursor(0,1);
    lcd.print(" Fresh Air");
    digitalWrite(LED_PIN_1, HIGH);
    digitalWrite(LED_PIN_2, LOW);
    digitalWrite(LED_PIN_3, LOW);
    
    } else if(ppm > 500 && ppm <= 800){
      lcd.setCursor(0,1);
      lcd.print("Moderate Air");
      digitalWrite(LED_PIN_2, HIGH);
      digitalWrite(LED_PIN_1, LOW);
      digitalWrite(LED_PIN_3, LOW);
    
    } else if(ppm > 800){
      lcd.setCursor(0,1);
      lcd.print("Poor Air !!"); 
      digitalWrite(LED_PIN_3, HIGH); // Turn on LED
      digitalWrite(LED_PIN_2, LOW);
      digitalWrite(LED_PIN_1, LOW);
    
      //delay(70);
      }

    delay(2000);
  }

  // Code for Showing in Web
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float newT = temperature;
    if (isnan(newT)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      t = newT;
      Serial.print("Temperature:");
      Serial.println(t);
    }

    float newH = humidity;
    if (isnan(newH)) {
      Serial.println("Failed to read from DHT sensor!");
    }
    else {
      h = newH;
      Serial.print("Humidity:");
      Serial.println(h);
    }

    float newPPM = ppm;
    if (isnan(newPPM)) {
      Serial.println("Failed to read from MQ135 sensor!");
    }
    else {
      ppm = newPPM;
      Serial.print("ppm:");
      Serial.println(ppm);
    }
  }
      

      // --------------------------------------------------------------------------------------------------

        if (isnan(h) || isnan(t)) 
            {
                Serial.println("Failed to read from DHT sensor!");
                return;
            }

                    if (client.connect(server_global,80))   //   "184.106.153.149" or api.thingspeak.com
                {  
                      
                  String postStr = apiKey;
                  postStr +="&field1=";
                  postStr += String(t);
                  postStr +="&field2=";
                  postStr += String(h);
                  postStr +="&field3=";
                  postStr += String(ppm);
                  postStr += "\r\n\r\n\r\n";
                  client.print("POST /update HTTP/1.1\n");
                  client.print("Host: api.thingspeak.com\n");
                  client.print("Connection: close\n");
                  client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
                  client.print("Content-Type: application/x-www-form-urlencoded\n");
                  client.print("Content-Length: ");
                  client.print(postStr.length());
                  client.print("\n\n");
              client.print(postStr);
                  }
    client.stop();

    Serial.println("Waiting...");
    Serial.println(t);
    Serial.println(h);
    Serial.println(ppm);
  
  
  // ------------------------------------------------------------------------------------------------------------
  
  
  
  
  
  
  }
    // Update PPM value for web display
   




// <p>
//     <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
//     <span class="dht-labels">Temperature</span> 
//     <span id="temperature">%TEMPERATURE%</span>
//     <sup class="units">&deg;C</sup>
//   </p>
//   <p>
//     <i class="fas fa-tint" style="color:#00add6;"></i> 
//     <span class="dht-labels">Humidity</span>
//     <span id="humidity">%HUMIDITY%</span>
//     <sup class="units">%</sup>
//   </p>
//   <p>
//     <i class="fas fa-thermometer" style="color:#00add6;"></i> 
//     <span class="dht-labels">PPM</span>
//     <span id="ppm">%ppm%</span>
//     <sup class="units">%</sup>
//   </p>
