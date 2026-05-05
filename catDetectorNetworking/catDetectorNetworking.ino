
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
    while (client.connected()) {            // loop while the client's connected
      delayMicroseconds(10);                // This is required for the Arduino Nano RP2040 Connect - otherwise it will loop so fast that SPI will never be served.
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor

        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            if (statusRequested) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:application/json");
              client.print("Connection:close");
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
              client.print("Connection:close");
              client.println();
              client.print("<p id=\"messageText\" style=\"font-size:7vw;\">No data received yet</p><br>");              
              client.print("<p style=\"font-size:7vw;\"> <a href=\"/L\">Snooze</a></p>");
              client.print("<script>");
              client.print("async function update() {");
              // Surveying happens here...
              // Fetch request
              client.print("try {");
              client.print("const response = await fetch('/status');");
              client.print("const data = await response.json();");
              client.print("document.getElementById(\"messageText\").textContent =");
              client.print("data.alarm");
              client.print(" ? 'Sensor is active'");
              client.print(" : 'Sensor is not active';");
              client.print("} catch (err) {");
              client.print("document.getElementById(\"messageText\").textContent = 'Error';");
              client.print("}");
              client.print("}");
              client.print("update();");
              client.print("setInterval(update, 200);");
              client.print("</script>");
              // The HTTP response ends with another blank line:
              client.println();
              // break out of the while loop:
            }
            break;
          }
          else {      // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        }
        else if (c != '\r') {    // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        if (currentLine.endsWith("GET /L")) {
          snooze = 1;
          Serial.println("Snoozed");
        }
        if (currentLine.endsWith("GET /status")) {
          statusRequested = true;
        }
      }
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
