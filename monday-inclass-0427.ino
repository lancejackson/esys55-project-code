
#include "WiFiS3.h"
#include "arduino_secrets.h" 

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

int led =  LED_BUILTIN;
int status = WL_IDLE_STATUS;

const int sensorPin = 8;
const int ACTIVATED = LOW;
int sensorValue; 
int alarm;
int snooze;
bool statusRequested;

WiFiServer server(80);

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Access Point Web Server");

  pinMode(led, OUTPUT);      // set the LED pin mode

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // by default the local IP address will be 192.168.4.1
  // you can override it with the following:
  //WiFi.config(IPAddress(192,168,4,1));

  // print the network name (SSID);
  Serial.print("Creating access point named: ");
  Serial.println(ssid);

  // Create open network. Change this line if you want to create an WEP network:
  status = WiFi.beginAP(ssid, pass);
  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true);
  }

  // wait 10 seconds for connection:
  delay(10000);

  // start the web server on port 80
  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();

  pinMode(sensorPin, INPUT_PULLUP);

  alarm = 0;
  snooze = 1;
}

void loop() {
  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }

  sensorValue = digitalRead(sensorPin); // Get sensor data
  if (sensorValue == ACTIVATED) { // Compare sensor input to what is considered "Activated"
    Serial.println("Sensor pressed");
    snooze = 0;
    alarm = 1;
  } else if (snooze) { // Snooze button handling logic
    snooze = 0;
    alarm = 0;
  }

  WiFiClient client = server.available();   // listen for incoming clients
  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    statusRequested = false;
    String currentLine = "";                // make a String to hold incoming data from the client
    String request = "";
    unsigned long start = millis();
    while (client.connected() && millis() - start < 200) {            // loop while the client's connected
      while (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        request += c;

        if (request.indexOf("\r\n\r\n") != - 1) {
          break;
        }
        
      }
      if (request.indexOf("\r\n\r\n") != - 1) {
        break;
      }

      delay(1);
    }

    
    bool isStatus = request.indexOf("GET /status") >= 0;
    bool isSnooze = request.indexOf("GET /L") >= 0;


    if (isSnooze) {
      snooze = 1;
      Serial.println("Snoozed");
    }

    
    if (isStatus) {
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:application/json");
      client.println("Connection: close");
      client.println();
      client.print("{");
      client.print("\"alarm\":");
      client.print(alarm ? "true" : "false");
      client.print(",");
      client.print("\"snooze\":");
      client.print(snooze ? "true" : "false");
      client.print("}");
      client.println();
    } else { // otherwise
      client.println("HTTP/1.1 200 OK");
      client.println("Content-type:text/html");
      client.println("Connection: close");
      client.println();
      client.print("<html><head></head><body>");
      client.print("<p id=\"messageText\" style=\"font-size:7vw;\">No data received yet</p><br>");
      client.print("<p style=\"font-size:7vw;\"> <a href=\"/L\">Snooze</a></p>");
      client.print("<div style=\"margin-left: 6px; border:1px solid #008; width:100%; background-color:#aaf;\"><p id=\"debugger\" style=\"color:white\";>Debugging info appears here</p></div>");
      client.print("<script>");
      /* Fetch with timeout function */
      client.print("async function fetchWithTimeout(url, timeout=2000) {");
      client.print("const controller = new AbortController();");
      client.print("const id = setTimeout(() => controller.abort(), timeout);");              
      client.print("try { const res = await fetch(url, {signal: controller.signal }); return res; }");
      client.print("finally { clearTimeout(id); } ");
      client.print("}");

      client.print("var inFlight = false;");
      /* Update function */
      client.print("async function update() {");
      client.print("if (inFlight) return;");
      client.print("inFlight = true;");
      client.print("try {");
      client.print("const response = await fetchWithTimeout('/status');");
      client.print("document.getElementById(\"debugger\").innerHTML = \"Got status\"");
      client.print("const data = await response.json();");
      client.print("document.getElementById(\"debugger\").innerHTML = \"Got some JSON\"");
      client.print("document.getElementById(\"messageText\").textContent =");
      client.print("data.alarm");
      client.print(" ? 'Sensor is active'");
      client.print(" : 'Sensor is not active';");
      client.print("} catch (err) {");
      client.print("document.getElementById(\"debugger\").textContent = err;");
      client.print("}");
      client.print("inFlight = false;");
      client.print("}");
      client.print("update();");
      client.print("setInterval(update, 2000);");
      client.print("</script>");
      client.print("</body></html>");
      // The HTTP response ends with another blank line:
      client.println();
      // break out of the while loop:
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}
