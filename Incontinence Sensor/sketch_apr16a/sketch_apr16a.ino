#include "WiFiS3.h"
#include "arduino_secrets.h"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

int status = WL_IDLE_STATUS;
WiFiServer server(80);

// Matrix pins
const int rows[3] = {4, 3, 2};      // 2N3904 control pins
const int cols[4] = {9, 10, 11, 12}; // LM393 outputs

bool matrix[3][4];

void setup() {
  Serial.begin(115200);

  // Row setup
  for (int i = 0; i < 3; i++) {
    pinMode(rows[i], OUTPUT);
    digitalWrite(rows[i], LOW);
  }

  // Column setup
  for (int i = 0; i < 4; i++) {
    pinMode(cols[i], INPUT);
  }

  // WiFi check
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module failed!");
    while (true);
  }

  // Connect to WiFi
  while (status != WL_CONNECTED) {
    Serial.print("Connecting to: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(5000);
  }

  server.begin();
  printWifiStatus();
}

void loop() {
  scanMatrix();

  WiFiClient client = server.available();
  if (client) {
    Serial.println("Connected to the website");

    String request = "";

    // Read request line
    while (client.available()) {
      char c = client.read();
      request += c;
    }

    // ===== DATA ENDPOINT =====
    if (request.indexOf("GET /data") >= 0) {   //if website is now up, send data instead of sending html code again

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.println();

      client.print("{\"matrix\":[");

      for (int r = 0; r < 3; r++) {
        client.print("[");
        for (int c = 0; c < 4; c++) {
          client.print(matrix[r][c] ? "1" : "0");
          if (c < 3) client.print(",");
        }
        client.print("]");
        if (r < 2) client.print(",");
      }

      client.print("]}");
    }

    // ===== MAIN PAGE =====
    // all of this crap was originally written in VSCode, so i can actually understand what im writing as there is no intellisense for quotation marked code, then transferred it over here 
    else {

      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();

      client.println("<!DOCTYPE html>");
      client.println("<html>");
      client.println("<head>");
      client.println("<meta charset='UTF-8'>");
      client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
      client.println("<title>Incontinence Sensor Monitor</title>");

      client.println("<style>");
      client.println("body { font-family: Arial; text-align:center; }");
      client.println("table { margin:auto; border-collapse: collapse; }");
      client.println("td, th { padding:12px; border:1px solid #ccc; font-size:18px; }");
      client.println(".on { background:#ff4444; color:white; }");
      client.println(".off { background:#44cc44; color:white; }");
      client.println("</style>");

      client.println("</head>");
      client.println("<body>");
      client.println("<h1>Live Incontinence Sensor Display</h1>");
      client.println("<button id='btn' onclick='toggleSound()'>Enable Alarm</button>");
      client.println("<table id='matrix'></table>");

      // JavaScript, eughh
      client.println("<script>");

      client.println("let soundEnabled = false;");
      client.println("let alarmPlaying = false;");

      client.println("const alarm = new Audio('https://www.myinstants.com/media/sounds/danger-alarm-sound-effect-meme.mp3');");

      // default the alarm to off
      client.println("alarm.onended = () => { alarmPlaying = false; };");

      // Toggle button
      client.println("function toggleSound(){");
      client.println("  soundEnabled = !soundEnabled;");
      client.println("  document.getElementById('btn').innerText = soundEnabled ? 'Disable Alarm' : 'Enable Alarm';");

      // Browser audio unlocking feature as modern browsers dont allow audio playback without user interaction
      client.println("  alarm.play().then(() => { alarm.pause(); alarm.currentTime = 0; }).catch(() => {});");

      client.println("  if(!soundEnabled){ alarm.pause(); alarm.currentTime = 0; alarmPlaying = false; }");
      client.println("}");

      client.println("function anyActive(matrix){");
      client.println("  for(let r=0;r<matrix.length;r++){");
      client.println("    for(let c=0;c<matrix[r].length;c++){");
      client.println("      if(matrix[r][c] == 1) return true;");
      client.println("    }");
      client.println("  }");
      client.println("  return false;");
      client.println("}");

      client.println("function updateMatrix(){");
      client.println("fetch('/data')");
      client.println(".then(r => r.json())");
      client.println(".then(data => {");

      client.println("let html = '';");
      client.println("for(let r=0;r<data.matrix.length;r++){");
      client.println("html += '<tr>';");

      client.println("for(let c=0;c<data.matrix[r].length;c++){");
      client.println("let val = data.matrix[r][c];");
      client.println("let cls = val ? 'on' : 'off';");
      client.println("html += `<td class='${cls}'>${val}</td>`;");
      client.println("}");

      client.println("html += '</tr>'; }");
      client.println("document.getElementById('matrix').innerHTML = html;");

      // Alarm logic
      client.println("let active = anyActive(data.matrix);");

      client.println("if(soundEnabled && active){");
      client.println("  if(!alarmPlaying){");
      client.println("    alarmPlaying = true;");
      client.println("    alarm.play().catch(() => {});");
      client.println("  }");
      client.println("} else {");
      client.println("  if(alarmPlaying){");
      client.println("    alarm.pause();");
      client.println("    alarm.currentTime = 0;");
      client.println("    alarmPlaying = false;");
      client.println("  }");
      client.println("}");

      client.println("});");
      client.println("}");

      client.println("setInterval(updateMatrix, 200);");
      client.println("updateMatrix();");

      client.println("</script>");

      client.println("</body>");
      client.println("</html>");
    }

    delay(1);
    client.stop();
    Serial.println("Data sent successfully, disconnecting");
  }
}

void scanMatrix() {
  for (int r = 0; r < 3; r++) {

    // Turn off all rows
    for (int i = 0; i < 3; i++) {
      digitalWrite(rows[i], LOW);
    }

    // Activate one row
    digitalWrite(rows[r], HIGH);

    delay(100); // allow capacitor to settle, in a perfect world settle time is 48.6ms but we are not living in the perfect world so 100ms seems to be more than reliable enough to prevent double toggling

    for (int c = 0; c < 4; c++) {
      matrix[r][c] = (digitalRead(cols[c]) == LOW);
    }
  }
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP: ");
  Serial.println(ip);

  Serial.print("RSSI: ");
  Serial.println(WiFi.RSSI());
}