/*================================================================================

	Copyright 2024 Grant Engineering ULC. All rights reserved.
	Barrack Street,
	Crinkle,
	Birr,
	Co Offaly
	Ireland. +353(0)57 912 0089

	File								: MQTT.cpp

	Description					:	Deals with comunicating with AWS

	Project Reference		:	TB6

	Project Name				:	ESP32 Adapter

	Client Name					:	Grant

	Date								: 04/06/2026

	Revision						:	1?

	Comments						:

================================================================================*/

/********************************************************************************
 * Includes
 **
 ********************************************************************************/
#include "MQTT.h"

/********************************************************************************
 * Definitions
 **
 ********************************************************************************/

/********************************************************************************
 * Prototypes
 **
 ********************************************************************************/

/********************************************************************************
 * Variables
 **
 ********************************************************************************/
/***********************************initialise instance libraries******************************************/
WebServer    server(80); // Object of WebServer(HTTP port, 80 is defult)
Preferences  preferences;
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

/***********************************************************************************************************/

// wifi - MQTT
uint16_t status = WL_IDLE_STATUS;
// String wifi_ssid, wifi_password, deviceidpreftemp;
String deviceidpreftemp;
int    __RESTART = 0;

// transmitt
uint8_t publishCounter, initialReadingCounter, mqtt_con_status, sub_status, mode;
const char *clientId;

bool	      publishStatus;
unsigned char COMMS_STATE     = FRAME_CHECK;
uint8_t	      initialFrameBit = 0, write_buffer_index = 0, Index = 0;
uint8_t	      numberOfBytes;
uint16_t      counter;
unsigned char log_buffer[COMMS_BUFFER_LENGTH];
unsigned char val_unchar;
uint16_t      signed_convert;
uint8_t	      write_data_buffer[WRITE_BUFFER_LEN];
char	      datetime[35];
const char   *ntpServer1	    = "pool.ntp.org";
const char   *ntpServer2	    = "time.nist.gov";
const long    gmtOffset_sec	    = 0;
const int     daylightOffset_sec    = 3600;
uint8_t	      softwareInterruptFlag = 0;
char	      payloadReceive[50];
uint16_t      deviceidPreference[12];

/********************************** OTA Stuff ******************************/
constexpr uint8_t ID_BYTES   = 24; // 12 chars x 2 bytes
constexpr uint8_t MIN_FRAME  = 4;  // smallest valid length byte from the device
uint32_t	  lastByteMs = 0;

uint32_t expectedSize  = 0;
uint32_t receivedSize  = 0;
uint32_t expectedChunk = 0;

bool uploading = false;

File uploadFile2;   //todo called 2 as duplicate from "File_Server" can fix when file server is deleted.

const char *topicStart = "esp32/file/start";
const char *topicChunk = "esp32/file/chunk";
const char *topicEnd   = "esp32/file/end";
const char *topicAck   = "esp32/file/ack";

char sizeBuffer[128];

/**********************************WiFi - AP - MQTT******************************/
const char *accessPSsid	    = "ESP32_AP";
const char *accessPPassword = "123";
char	    broker[]   = "ec2-51-21-3-129.eu-north-1.compute.amazonaws.com";
uint16_t    port       = 1883;
const char  topic[]    = "NTC/data/transmit-EP011";
const char  topicR[]   = "NTC/data/receive-EP011_NR";
const char  topicOTA[] = "ep011_ota";

// hw_timer_t *ms_Timer2 = NULL;        // dosent seem to like these in the header files?

// int Timer_Period2 = 500;             // the period between calling the timer
// interupt in ms bool LED_State_MQTT = false;         // dosent need to be set to true to invert with other core as they are asyncronus and start at difrent times...

String cmdstr = String(0); // string for incoming comand serial data

/********************************************************************************
 * Constants
 **
 ********************************************************************************/
// HTML & CSS contents which display on web server
String INDEX_HTML =
    "<!DOCTYPE HTML><html><head><meta content=\"text/html; charset=ISO-8859-1\" http-equiv=\"content-type\"><meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">\
<title>Grant Engineering</title><style>\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; text-align:center;}\"</style>\
</head><body><h2>Grant Engineering</h2><h3>Enter your WiFi credentials</h3><form method=\"post\"><p>\
<label>Device ID:</label>&nbsp<input id=\"id\" maxlength=\"30\" name=\"DeviceID\"><br>\
<label>SSID:</label>&emsp;&emsp;&nbsp<input maxlength=\"30\" name=\"ssid\" id=\"ssid\"><br>\
<label>Key:</label>&emsp;&emsp;&ensp;&nbsp;<input maxlength=\"30\" name=\"password\" id=\"password\"><br><br>\
<input type=\"submit\"  name=\"submit\" id=\"sub\" value=\"Submit\" onclick=\"myFunction()\" />\
<script>function myFunction() {alert(document.getElementById(\"id\").value);}</script>\
</p></form></body></html>";

/********************************************************************************
 * Code
 **
 ********************************************************************************/

void Init_MQTT()
{
  preferences.begin("credential", false);
  digitalWrite(INDICATOR_TX_RX, LOW);

  while (!loadIndexPage())
    ; // todo dose this hang everything on both cores untill you have put wifi
      // creds in?
  connectWifi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  LocalTime();
  clientId = datetime;
  Serial.println(clientId);
  mqtt_setup();

  Serial.println(deviceidpreftemp);
  for (uint8_t count = 0; count < 12; count++)
  {
    deviceidPreference[count] = (uint16_t)deviceidpreftemp[count];
    // Serial.println(deviceidPreference[count]);
  }
}

void pollUART(void)
{
   // Serial.println(COMMS_STATE);
    while (Serial1.available() && COMMS_STATE != TRANSMIT)
    {
        uint8_t b = Serial1.read();
        lastByteMs = millis();
        switch (COMMS_STATE) 
        {
            case FRAME_CHECK:
            
                if (b == 0xAA)
                {
                    clear_transmit_buffer();
                    write_buffer_index = 0;
                    write_data_buffer[write_buffer_index++] = b;
                    COMMS_STATE = FRAME_LENGTH;
                }
                break;

            case FRAME_LENGTH:
                // reject impossible lengths so a stray 0xAA can't throw the parser off
                if (b < MIN_FRAME || (uint16_t)b + ID_BYTES > WRITE_BUFFER_LEN)
                {
                    COMMS_STATE = FRAME_CHECK;
                    break;
                }
                numberOfBytes = b;
                write_data_buffer[write_buffer_index++] = b + ID_BYTES;
                for (uint8_t i = 0; i < 12; i++)
                {
                    write_data_buffer[write_buffer_index++] = deviceidPreference[i] >> 8;
                    write_data_buffer[write_buffer_index++] = deviceidPreference[i] & 0xFF;
                }
                COMMS_STATE = FRAME_LOG;
                break;

            case FRAME_LOG:
                write_data_buffer[write_buffer_index++] = b;
                if (write_buffer_index >= numberOfBytes + ID_BYTES)
                {
                    COMMS_STATE = TRANSMIT;
                }
                break;
        }
    }
}

// void Task_Manage_MQTT(void *pvParameters)
void Task_Manage_MQTT(void)
{
    if (!mqttClient.loop())
    {
        mqtt_setup();               // reconnect if the connection dropped
        return;
    }

    // downlink message received from MQTT
    if (softwareInterruptFlag)
    {
        digitalWrite(INDICATOR_TX_RX, LOW);
        // Serial1.print(messageTemp);   // forward to the device here if needed
        softwareInterruptFlag = 0;
        messageTemp = "";
    }

    pollUart();                   // non-blocking: reads whatever bytes have arrived

    if (COMMS_STATE == TRANSMIT)
    {
        // digitalWrite(INDICATOR_TX_RX, !digitalRead(INDICATOR_TX_RX));
        heartbeat_MQTT();
        mqtt_publish(write_data_buffer, write_buffer_index);

        Serial.printf("len=%u idx=%u rxWaiting=%d\n",
                    numberOfBytes, write_buffer_index, Serial1.available());

        COMMS_STATE = FRAME_CHECK;
    }

    read_serial(0, 0);
}

void Clear_Wifi_Credentials()
{
  Serial.println(
      "Clearing the ssid and password. Please reset the unit and join AP.");
  preferences.clear();

  preferences.end();
}

void connectWifi()
{
  String wifi_ssid, wifi_password;

  // WiFi.mode(WIFI_AP_STA);
  uint8_t status;
  wifi_ssid	   = preferences.getString("ssid", "");
  wifi_password	   = preferences.getString("pass", "");
  deviceidpreftemp = preferences.getString("deviceid", "");
  __RESTART	   = preferences.getInt("restart");

  preferences.end();
  // print the network name (SSID);
  Serial.print("Connecting to wifi named: ");
  Serial.println(wifi_ssid);
  Serial.print("with password ");
  Serial.println(wifi_password);

  if ((WiFi.getMode() != WIFI_AP_STA))
  {
    // WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_AP_STA);
    delay(100);
  }
  WiFi.begin(wifi_ssid, wifi_password);
  status = WiFi.waitForConnectResult();
  Serial.print("wifi status: ");
  Serial.println(status);
  // connect to wifi
  delay(1000);
  while (status != WL_CONNECTED)
  {
    // failed, retry
    status = WiFi.waitForConnectResult();
    Serial.print(".");
  }
  // wait 1 seconds for connection:
  delay(1000);

  /********* printWiFiStatus*********/
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.println(ip);
}

bool writeToMemory(String ssid, String pass, String id, int __RESTART)
{
    // By default, 20 KB of space is reserved for preferences
    preferences.putString("ssid", ssid);
    preferences.putString("pass", pass);
    preferences.putString("deviceid", id);
    preferences.putInt("restart", __RESTART);
    delay(100);
    String s = preferences.getString("ssid", "");
    String p = preferences.getString("pass", "");
    String i = preferences.getString("deviceid", "");

    /************debug logs************/

    if (ssid == s && pass == p && i == id)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void handleSubmit()
{
    String response_success = "<h1>Success</h1>";
    response_success += "<h2>Device will restart in 3 seconds</h2>";

    String response_error = "<h1>Error</h1>";
    response_error += "<h2><a href='/'>Go back</a>to try again";

    __RESTART = __RESTART + 1;
    if (writeToMemory(String(server.arg("ssid")), String(server.arg("password")),
                String(server.arg("DeviceID")), __RESTART))
    {
        server.send(200, "text/html", response_success);
        delay(3000);
        ESP.restart();
    }
    else
    {
        server.send(200, "text/html", response_error);
    }
}

void mqtt_setup(void)
{
    Serial.print("Attempting to connect to the MQTT broker: ");
    Serial.println(broker);

    mqttClient.setBufferSize(1200); // or 1024; put it before connect()

    // Serial.println(clientId);
    mqttClient.setServer(broker, port);
    mqttClient.setCallback(callback);
    mqtt_con_status = mqttClient.connect(clientId);
    delay(500);
    if (mqtt_con_status == true)
    {
        Serial.print("connected with clientID : ");
        Serial.print(clientId);
        Serial.println();

        mqttClient.subscribe(topicStart, 1);
        mqttClient.subscribe(topicChunk, 1);
        mqttClient.subscribe(topicEnd, 1);

        // Serial.print("Subscription status: ");
        // Serial.print(bool(sub_status));
        // Serial.println();

        Serial.println("You're connected to the MQTT broker!");
        Serial.println();
    }
    else
    {
        Serial.print("failed, rc=");
        Serial.print(mqttClient.state());
        Serial.println(" try again in 1 seconds");
        delay(500);
        // mqtt_setup();         //todo why dose this call itsself??? this is causing an overflow issue???
    }
}

void handleRoot()
{
    if (server.hasArg("ssid") && server.hasArg("password"))
    {
        handleSubmit();
    }
    else
    {
        server.send(200, "text/html", INDEX_HTML);
    }
}

bool loadIndexPage()
{
    // Connect to Wi-Fi network with SSID and password
    String ssid = preferences.getString("ssid");
    String pass = preferences.getString("pass");
    String id   = preferences.getString("deviceid");
    Serial.print("Setting AP (Access Point)…");
    // Remove the password parameter, if you want the AP (Access Point) to be open
    WiFi.softAP(accessPSsid, accessPPassword);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    server.on("/", handleRoot);
    server.begin();
    // NetworkServer.begin();
    Serial.println("HTTP server started");
    delay(100);
    Serial.println("waiting for ssid and password !");
    while (ssid.length() <= 0 && pass.length() <= 0 &&
        id.length() <= 0) // todo dose this hang everything untill you have put
                // wifi creds in?
    {
        server.handleClient();
        delay(50);
    }
    return true;
}

void mqtt_publish(byte write_data_buffer[WRITE_BUFFER_LEN], uint8_t write_buffer_index)
{
  counter++;
  publishStatus =
      mqttClient.publish(topic, write_data_buffer, write_buffer_index, false);
  delay(1);
  Serial.print("mqtt publish status: ");
  Serial.print(publishStatus);
  Serial.println();
  if (publishStatus == 1)
  {
    Serial.print("publish counter: ");
    Serial.print(counter);
    Serial.println();
    Serial.println();
    Serial.println();
    return;
  }
  else
  {
    Serial.println("***********publish Fail**********");
    return;
  }
}

void clear_transmit_buffer(void)
{
  // write_data_buffer[WRITE_BUFFER_LEN]
  for (uint8_t count = 0; count < WRITE_BUFFER_LEN; count++)
  {
    write_data_buffer[count] = 0;
  }
  return;
}

void LocalTime(void)
{
  struct tm timeinfo;
  Serial.println("Getting local time");
  while (!getLocalTime(&timeinfo))
  {
    Serial.print(".");
    // return;
  }
  String clientid = deviceidpreftemp + "-%d-%m-%Y-%H-%M-%S";
  strftime(datetime, 35, clientid.c_str(), &timeinfo);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String File_Path = "/EP011_New.bin";
  //================================ new ==================================//
  // START FILE TRANSFER
  if (strcmp(topic, topicStart) == 0)
  {

    // expectedSize = atoi((char*)payload);
    if (length >= sizeof(sizeBuffer))
    {
      Serial.println("Invalid file size payload");
      return;
    }

    memcpy(sizeBuffer, payload, length);
    sizeBuffer[length] = '\0';

    expectedSize = strtoul(sizeBuffer, NULL, 10);

    Serial.printf("Expected file size: %lu bytes\n", (unsigned long)expectedSize);

    receivedSize  = 0;
    expectedChunk = 0;

    if (uploadFile2)
    {
      uploadFile2.close();
    }

    uploadFile2 = LittleFS.open(File_Path, "w");

    if (!uploadFile2)
    {
      mqttClient.publish(topicAck, "ERROR");
      return;
    }

    uploading = true;

    mqttClient.publish(topicAck, "READY");

    Serial.println("File transfer started");
  }

  // RECEIVE FILE CHUNK
  else if (strcmp(topic, topicChunk) == 0)
  {

    if (!uploading || length < 4)
    {
      return;
    }

    // Extract chunk index
    uint32_t chunkIndex = ((uint32_t)payload[0] << 24) | ((uint32_t)payload[1] << 16) | ((uint32_t)payload[2] << 8) | ((uint32_t)payload[3]);

    if (chunkIndex != expectedChunk)
    {
      return;
    }

    // Write binary data
    size_t dataLength = length - 4;

    size_t written = uploadFile2.write(payload + 4, dataLength);

    if (written != dataLength)
    {
      mqttClient.publish(topicAck, "ERROR");
      return;
    }

    receivedSize += written;

    char ack[32];

    sprintf(ack, "CHUNK:%lu", (unsigned long)chunkIndex);

    expectedChunk++;

    mqttClient.publish(topicAck, ack);

    Serial.printf("Received chunk: %lu\n", (unsigned long)chunkIndex);
  }

  // FINISH FILE TRANSFER
  else if (strcmp(topic, topicEnd) == 0)
  {
    // Serial.print("completed");

    if (uploading)
    {

      uploadFile2.close();
      Serial.printf("receivedSize=%d\n", receivedSize);

      if (receivedSize == expectedSize)
      {
        uploading = false;

        mqttClient.publish(topicAck, "COMPLETE");

        Serial.println("File received successfully!");

        Send_Updated_Software_To_PCB(File_Path);
      }
      else
      {
	      mqttClient.publish(topicAck, "SIZE_ERROR");
      }
    }
  }
  softwareInterruptFlag = 1;
}

void heartbeat_MQTT()
{
  digitalWrite(LED_2, !Get_LED_State());
}

// void read_serial()
void read_serial(byte *message, unsigned int size)
{
  if (Serial.available() > 0)
  {
    // read the incoming comand string:
    cmdstr = Serial.readStringUntil(10);

    // say what you got:
    Serial.print("I received: ");
    Serial.println(cmdstr);

    if (cmdstr == "rst") // rst = reset wifi creds
    {
      Serial.println("reset wifi creds");
      Clear_Wifi_Credentials();
      Init_MQTT();
    }
    else if (cmdstr == "tst") // tst = make test.txt
    {
      Serial.println("Updating binary in the main board!!");
      Serial.println("make test.txt inside MQTT.cpp");
      // todo how get to core 1
      int    File_Size;
      String File_Path = "/test.txt";
      File_Size = Get_File_Size(LittleFS, File_Path.c_str());
      Write_File_Binary(LittleFS, File_Path.c_str(), message, size);
      File_Size = Get_File_Size(LittleFS, File_Path.c_str());
    }
    else if (cmdstr == "prg") // prg = enter programing mode
    {
      Serial.println("entering programing mode");
      Set_Programing_Mode_Flag(true);
    }
    else if (cmdstr == "slow") // slow = program slow ep11 to board
    {
      Serial.println("program slow ep11");
      // File_Path = "/EP011_slow.bin";
      Send_Updated_Software_To_PCB("/EP011_slow.bin"); // todo at any point you want to update the software just call this function with the file path and it should sort everything else out.
    }
    else if (cmdstr == "fast") // fast = program fast ep11 to board
    {
      Serial.println("program fast ep11");
      String File_Path = "/EP011_fast.bin";
      Send_Updated_Software_To_PCB(File_Path); // todo you can ether pass it the file path directly or a string variable with the path in it.
    }
    else if (cmdstr == "del") // del = delete the /EP011_New.bin file from the board
    {
      Serial.println("deleting file /EP011_New.bin");
      String File_Path = "/EP011_New.bin";
      Delete_File(LittleFS, File_Path.c_str()); // todo added to check if file needs deleted before we add new data to it?
    }
  }
}
