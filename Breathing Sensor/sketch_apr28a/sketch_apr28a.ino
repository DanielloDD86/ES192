#include "WiFiS3.h" 
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID; 
char pass[] = SECRET_PASS; 
int keyIndex = 0; 

int status = WL_IDLE_STATUS; 

WiFiServer server(80); 

const int apin = A0;
float vBase = 1.60;
const float vThresh = 0.08;
const int lpin = 9;
bool fellBelow = false;
unsigned long millitime = 0;
int breathC = 0;
int breathBPM = 0;

void setup() {
  // Serial.begin(115200);
  millitime = millis();

  long sum = 0;
  const int samples = 50;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(apin);
  }

  float raw = sum / (float)samples ;
  float result  = (raw / 1023.0) * 5.0;
  vBase = result - 0.2;

  if (WiFi.status() == WL_NO_MODULE) { 
    while (true); // if no wifi module is present the loop never exits and the program freezes at the point
  }
  
  while (status != WL_CONNECTED) { // while there is no connection to WiFi …
    // Serial.print("Attempting to connect to SSID: ");
    // Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  server.begin(); 
  printWifiStatus();
}

void loop() {
  long sum = 0;
  const int samples = 50;

  for (int i = 0; i < samples; i++) {
    sum += analogRead(apin);
  }

  float raw = sum / (float)samples ;
  float result  = (raw / 1023.0) * 5.0;
  // Serial.println(result);
  delay(50);

  if (fellBelow == false && (vBase - result) > vThresh) {
    fellBelow = true;
    delay(100);
  } else if (fellBelow == true && (vBase - result) < vThresh) {
    fellBelow = false;
    digitalWrite(lpin, HIGH);
    // Serial.println("Breath!");
    breathC = breathC + 1;
    delay(200);
    digitalWrite(lpin, LOW);
  }

  if ((millis() - millitime) > 15000) {
    breathBPM = breathC * 4;
    breathC = 0;
    // Serial.println("Breath count per minute");
    // Serial.println(breathBPM);
    millitime = millis();
    delay(500);
  }

  WiFiClient client = server.available(); 

  if (client) {
    // Serial.println("new client");
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read(); 
        // Serial.write(c);
        if (c == '\n' && currentLineIsBlank) { 
          client.println("HTTP/1.1 200 OK"); 
          client.println("Content-Type: text/html"); 
          client.println("Connection: close"); 
          client.println(); 
          client.println("<!DOCTYPE html>");
          client.println("<html>"); 
          client.println("<head>"); 
          client.println("<meta charset='UTF-8'>");
          client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
          client.println("<meta http-equiv='refresh' content='1'>"); // Refresh every second
          client.println("<title>Sensor Data</title>"); 
          client.println("<style>"); 
          client.println("body { font-family: Arial, sans-serif; text-align: center; margin: 20px; }");
          client.println(".container { max-width: 600px; margin: auto; padding: 20px; border: 1px solid #ddd; border-radius: 10px; }"); // main box style
          client.println(".value { font-size: 1.2rem; margin: 10px 0; }"); 
          client.println(".header { font-size: 1.5rem; font-weight: bold; margin-bottom: 10px; }"); 
          client.println("</style>"); 
          client.println("</head>");
          client.println("<body>"); 
          client.println("<div class='container'>"); 
          client.println("<h1>Live Sensor Data</h1>"); 
          client.println("<div class='header'>Breathing Sensor</div>");
          client.print("<div class='value'>Voltage: "); 
          client.print(result, 2); 
          client.println(" V</div>");
          client.print("<div class='value'>breaths per minute: ");
          client.print(breathBPM);
          client.println("<p>Page refreshes every 5 seconds to update data.</p>");
          client.println("</div>"); 
          client.println("</body>");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;
        } else if (c != '\r') {
          currentLineIsBlank = false;
        }
      }
    }
    delay(1);
    client.stop(); 
    // Serial.println("client disconnected");
  }

}

void printWifiStatus() {
  // Serial.print("SSID: ");
  // Serial.println(WiFi.SSID());
  IPAddress ip = WiFi.localIP();
  // Serial.print("IP Address: ");
  // Serial.println(ip);
  long rssi = WiFi.RSSI();
  // Serial.print("signal strength (RSSI):");
  // Serial.print(rssi);
  // Serial.println(" dBm");
}
