//Include all the libraries for the microcontroller, sensors, display, and MQTT.
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <DHT_U.h>
#define DATA_PIN 12
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_MPL115A2.h>
#include <Adafruit_Si7021.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32 
#define OLED_RESET 13
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//The identifiers and passwords to connect to the internet and MQTT.
#define ssid ""
#define pass ""

#define mqtt_server ""
#define mqtt_name ""
#define mqtt_pass ""

//Create instances of the sensors and execute the clients.
DHT_Unified dht(DATA_PIN, DHT22);
Adafruit_MPL115A2 mpl115a2;
WiFiClient espClient;
PubSubClient mqtt(espClient);

//Create the variables that will be used later.
const int buttonPin = 2;
int buttonState = 1;
unsigned long previousMillis;
char espUUID[8] = "ESP8602";
char mac[6];
boolean buttonFlag = false;

//Receive data from MQTT as a subscriber and print the characters of the payload to the serial monitor.
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

//Reconnect to MQTT if it gets disconnected; publish a message and subscribe to a topic.
void reconnect() {
  while (!espClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqtt.connect(mac, mqtt_name, mqtt_pass)) {
      Serial.println("connected");
      char announce[40];
      strcat(announce, mac);
      strcat(announce, "is connecting. <<<<<<<<<<<");
      
      mqtt.publish("stevenyong", announce);
      mqtt.subscribe("stevenyong/+");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

//Initialize the sensors, serial, button pin, WiFi, display, and MQTT. Connect to the internet and display messages to help debug issues.
void setup() {
  dht.begin();
  mpl115a2.begin();
  
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println();
  Serial.print("Your ESP has been assigned the internal IP address ");
  Serial.println(WiFi.localIP());

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.display();
  delay(2000);
  display.clearDisplay();
  display.drawPixel(10, 10, WHITE);
  display.display();
  
  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);
}

//Set the defaults of the display, start a timer, and maintain MQTT connection.
//Check to see if the button is pressed and publish to the topic accordingly. Print a message to the display.
//Publish weather data every 5 seconds and print a message to the display.
void loop() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  unsigned long currentMillis = millis();

  mqtt.loop();
  if (!mqtt.connected()) {
    reconnect();
  }

  buttonState = digitalRead(buttonPin);

  if (buttonState == HIGH && buttonFlag == false) {
    Serial.println("Motion detected");
    char message[20];
    sprintf(message, "{\"uuid\": \"%s\", \"button\": \"%s\" }", espUUID, "1");
    mqtt.publish("stevenyong/button", message);
    buttonFlag = true;
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(F("Motion detected"));
    display.display();
  }
  if (buttonState == LOW && buttonFlag == true) {
    Serial.println("Area is clear.");
    char message[20];
    sprintf(message, "{\"uuid\": \"%s\", \"button\": \"%s\" }", espUUID, "0");
    mqtt.publish("stevenyong/button", message);
    buttonFlag = false;
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(F("Area is clear."));
    display.display();
  }

  if (currentMillis - previousMillis > 5000) {
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    float celsius = event.temperature;
    float temp = (celsius * 1.8) + 32;
    dht.humidity().getEvent(&event);
    float humd = event.relative_humidity;
    float pres = 0;
    pres = mpl115a2.getPressure();
    
    char str_temp[6];
    char str_humd[6];
    char str_pres[7];
    char message[50];

    dtostrf(temp, 4, 2, str_temp);
    dtostrf(humd, 4, 2, str_humd);
    dtostrf(pres, 4, 2, str_pres);
    
    sprintf(message, "{\"uuid\": \"%s\", \"temperature\": \"%s\", \"humidity\": \"%s\", \"pressure\": \"%s\" }", espUUID, str_temp, str_humd, str_pres);
    mqtt.publish("stevenyong/sensors", message);
    
    Serial.println("publishing");
    previousMillis = currentMillis;
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(temp);
    display.println(F("F"));
    display.print(humd);
    display.println(F("% Humidity"));
    display.print(pres);
    display.print(F("kPA"));
    display.display();
  }
}
