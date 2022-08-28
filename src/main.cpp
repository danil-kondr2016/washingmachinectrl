#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const int MIN_COUNT = 2;
const int MAX_ATTEMPTS = 20;
const int SIGNAL = D5;

volatile uint32_t count = 0;

bool wifi = false;
bool washMachine = false;

IPAddress address(192, 168, 4, 1);
ESP8266WebServer server(80);

IRAM_ATTR void _int(void) { count++; }

void sendSignal(const char * signal);

void handleRoot()
{
  String answer;
  answer = String("Wash machine signal: ") + (washMachine ? "on" : "off") + "\r\n";
  answer += String("Impulses: ") + count;

  server.send(200, "text/plain", answer);
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.print("Connecting to HOMECTRL");
  WiFi.mode(WIFI_STA);
  WiFi.begin("HOMECTRL");
  int n_attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    n_attempts++;
    if (n_attempts > MAX_ATTEMPTS)
      break;
  }
  if (n_attempts <= MAX_ATTEMPTS)
    wifi = true;
  if (wifi) {
    Serial.println(" Connected successfully");
  } else {
    Serial.println(" Connection failed");
  }

  server.on("/", handleRoot);
  server.begin();

  pinMode(SIGNAL, INPUT);
  attachInterrupt(
    digitalPinToInterrupt(SIGNAL),
    _int,
    RISING
  );
}

void loop()
{
  digitalWrite(LED_BUILTIN, wifi ? HIGH : LOW);

  sei();
  delay(1000);
  cli();

  if ((count >= MIN_COUNT)) {
    sendSignal("W1");
  }
  else
  if ((count < MIN_COUNT)) {
    sendSignal("W0");
  }

  count = 0;
  sei();
  server.handleClient();
}

void sendSignal(const char * signal)
{
  sei();

  WiFiClient client;

  if (!client.connect(address, 23)) {
    Serial.println("Connection failed");
    return;
  }

  if (client.connected()) {
    client.println(signal);

    unsigned int t;

    t = millis();
    while (!client.available()) {
      yield();
      if (millis() - t >= 500) {
        Serial.println("Timeout");
        return;
      }
    }

    while (client.available()) {
      client.read();
      yield();
    }

    client.stop();
  } else {
    Serial.println("Connection refused");
  }

  switch (signal[0]) {
    case 'W':
      if (signal[1] == '0')
        washMachine = false;
      else if (signal[1] == '1')
        washMachine = true;
      break;
  }
}
