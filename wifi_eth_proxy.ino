/*
  Deisnged for Arduino Uno WiFi v2 + Ethernet Shield v2 w/ POE.

  Wifi-to-ethernet gateway with a single destination IP. Wifi can be DHCP
  or static (currently static). Correctly forwards JSON responses, but does
  not render a full website well (connection reset errors).

  MAC address for ethernet adapter is required / hardcoded on ethernet shield.

  Note: only allows for 256 chars in first line of HTTP request header.

  Randy Almand | ralmand@gmail.com | 2022-01-23
*/
#include <SPI.h>
#include <WiFiNINA.h>
#include <Ethernet.h>

#include "arduino_secrets.h"
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status
byte mac[] = {
  0xA8, 0x61, 0x0A, 0xAE, 0x89, 0x1D
};

// Static IP & DNS configuration for Wifi.
IPAddress wifiIP(192, 168, 0, 15);
IPAddress wifiDNS(192, 168, 0, 15);

// Server IP address (SunPower PVS6)
IPAddress serverIP(172, 27, 153, 1);

// Webserver running on port 80 on wireless interface.
WiFiServer wServer(80);

// Ethernet client for outgoing requests.
EthernetClient eClient;

void setup() {
  // Configure CS pin for ethernet shield
  Ethernet.init(10);

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  setupWifi();

  setupEthernet();

  Serial.println("\n*****************************\n");

  // Start webserver
  wServer.begin();
}

void setupWifi() {
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  //  WiFi.setTimeout(120 * 1000);

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:

    WiFi.config(wifiIP, wifiDNS);
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("Connected to wireless");
  printCurrentNet();
  printWifiData();
}

void setupEthernet() {
  Serial.println("\nInitialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
  Serial.println("Connected to ethernet!");
  Serial.print("Ethernet IP address: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Ethernet MAC address: ");
  printMacAddress(mac);
}

void loop() {
  if (WiFi.status() == WL_DISCONNECTED || WiFi.status() == WL_CONNECTION_LOST)
  {
    // attempt to connect to WiFi network:
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("\nReconnecting to  SSID: ");
      Serial.println(ssid);
      // Connect to WPA/WPA2 network:
      WiFi.config(wifiIP, wifiDNS);
      status = WiFi.begin(ssid, pass);

      // wait 10 seconds for connection:
      delay(10000);
    }
    Serial.println("Connected to wireless");
    printWifiData();
  }

  // listen for incoming clients
  WiFiClient wClient = wServer.available();
  if (wClient) {
    char req[256];
    int i = 0;

    // Need to read first line of HTTP request to forward to ethernet.
    while (wClient.connected()) {
      // Note: will continue running if there is data available to consume.
      // Available bytes to be read.
      if (wClient.available()) {
        // Read single char from input
        char c = wClient.read();
        // Save in request char array
        req[i] = c;
        i++;

        if (c == '\n') {
          // End of first line in header, don't need the rest.
          req[i] = 0; // Set last byte as zero to end string.
          break;
        }
      }
    }

    String reqS = String(req);
    if (!reqS.startsWith("GET")) {
      // No need to process additional lines of header.
      return;
    } else {
      Serial.print("\nRequest:");
      Serial.write(req); // Note: ends with new line.

      Serial.print("Connecting to ");
      Serial.print(serverIP);
      Serial.print(" ... ");
    }

    // Connection succeeded
    if (eClient.connect(serverIP, 80)) {
      Serial.print("Connected to ");
      Serial.println(eClient.remoteIP());

      eClient.print(req);
      eClient.println("Host: 172.27.153.1");
      eClient.println("Connection: close");
      eClient.println();
    } else {
      Serial.println("connection failed");
    }

    Serial.print("Waiting for response...");
    while (!eClient.available()) {
      delay(1);
    }

    Serial.println("Got response!");
    int bytes = 0;
    do {
      // Read response into 1kb buffer and relay to wireless client.
      int len = eClient.available();
      byte buffer[1024];
      if (len > 1024) len = 1024;
      bytes += len;

      eClient.read(buffer, len);
      wClient.write(buffer, len);

      if (!eClient.connected()) {
        eClient.stop();
        break;
      }
      // Continue to loop until no bytes left on ethernet connection.
    } while (eClient.available());
    double kbytes = bytes / 1024.0;
    Serial.print(kbytes);
    Serial.println(" kiB transmitted");
  }
}

void printWifiData() {
  // Print wifi IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("Wifi IP Address: ");
  Serial.println(ip);

  // Print wifi MAC address
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("Wifi MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // Print SSID of the wifi network
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Print the MAC address of the wireless router/AP
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // Print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // Print the encryption type
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

// Helper function to format and print mac address
void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}
