
#include "WiFiEsp.h"

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(3, 2); // RX, TX
#endif

#include <PubSubClient.h>
#include <Msgflo.h>

char ssid[] = "AndroidAPll";            // your network SSID (name)
char pass[] = "testardu89";        // your network password
int status = WL_IDLE_STATUS;

int ledStatus = LOW;
int thresholdValue=900;

WiFiEspServer server(3566);


struct Config {
  const String prefix = "msgflo/arduino/";
  const String role = "button/one";

  const int ledPin = LED_BUILTIN;
  const int buttonPin = 5;

  const char *mqttHost = "test.mosquitto.org";
  const int mqttPort = 1883;

  const char *mqttUsername = NULL;
  const char *mqttPassword = NULL;
} cfg;

WiFiEspClient wifiClient;
PubSubClient mqttClient;
msgflo::Engine *engine;
msgflo::OutPort *buttonPort;
msgflo::InPort *ledPort;
long nextButtonCheck = 0;

auto participant = msgflo::Participant("iot/Button", cfg.role);

void setup() {
  Serial.begin(115200);   // initialize serial for debugging
  Serial1.begin(9600);    // initialize serial for ESP module
  WiFi.init(&Serial1);    // initialize ESP module
  delay(100);
  Serial.println();
  Serial.println();
  Serial.println();

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  printWifiStatus();

  // Provide a Font Awesome (http://fontawesome.io/icons/) icon for the component
  participant.icon = "toggle-on";

  mqttClient.setServer(cfg.mqttHost, cfg.mqttPort);
  mqttClient.setClient(wifiClient);

  String clientId = cfg.role;
  clientId += WiFi.macAddress();

  engine = msgflo::pubsub::createPubSubClientEngine(participant, &mqttClient, clientId.c_str(), cfg.mqttUsername, cfg.mqttPassword);

  buttonPort = engine->addOutPort("button", "any", cfg.prefix+cfg.role+"/event");

  ledPort = engine->addInPort("led", "boolean", cfg.prefix+cfg.role+"/led",
  [](byte *data, int length) -> void {
      const std::string in((char *)data, length);
      const boolean on = (in == "1" || in == "true");
      digitalWrite(cfg.ledPin, on);
  });

  Serial.printf("Led pin: %d\r\n", cfg.ledPin);
  Serial.printf("Button pin: %d\r\n", cfg.buttonPin);

  pinMode(cfg.buttonPin, INPUT);
  pinMode(cfg.ledPin, OUTPUT);
}

void loop() {
  static bool connected = false;

  if (WiFi.status() == WL_CONNECTED) {
    if (!connected) {
      Serial.printf("Wifi connected: ip=%s\r\n", WiFi.localIP().toString().c_str());
    }
    connected = true;
    engine->loop();
  } else {
    if (connected) {
      connected = false;
      Serial.println("Lost wifi connection.");
    }
  }

  // TODO: check for statechange. If changed, send right away. Else only send every 3 seconds or so
  if (millis() > nextButtonCheck) {
    const bool pressed = digitalRead(cfg.buttonPin);
    buttonPort->send(pressed ? "true" : "false");
    nextButtonCheck += 500;
  }
}

