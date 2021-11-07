
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <Crc16.h>

#define BAUDR 9600 // baud rate of serial to software serial of arduino uno
#define MAX_RECV_BYTES 101 // maximal buffer size for recieving progress
ESP8266WebServer server(80); // web server object

char param_buffer[MAX_RECV_BYTES]; // container for recieved parameters which were send to website

// html table for parameters and javascript code to fill the table
const char * htmlMessage = R"rawliteral(<!DOCTYPE HTML>

<html>
    <head>
        <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
        <title>Wetterstation - Parameters</title>
    </head>
    <style>
        *{font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;}
            #parameterTable {
              border-collapse: collapse;
              width: 100%;
            }
            
            #parameterTable td, #parameterTable th {
              border: 1px solid #ddd;
              padding: 8px;
            }
            
            #parameterTable tr:nth-child(even){background-color: #f2f2f2;}
            
            #parameterTable tr:hover {background-color: #ddd;}
            
            #parameterTable th {
              padding-top: 12px;
              padding-bottom: 12px;
              text-align: left;
              background-color: red;
              color: white;
            }
            </style>
    <body>
        <h2 style="size: 1em;">Current Parameters</h2>
        <table id="parameterTable" style="width:100%">
            <tr>
              <th>Parameter</th>
              <th>Value</th>
              <th>Unit</th>
            </tr>
            <tr>
              <td>Current Humidity</td>
              <td>NaN</td>
              <td>Percent (%)</td>
            </tr>
            <tr>
              <td>Current Temperature</td>
              <td>NaN</td>
              <td>Centigrade</td>
            </tr>
            <tr>
                <td>Current P2.5</td>
                <td>NaN</td>
                <td>Ratio</td>
              </tr>
              <tr>
                <td>Current P10</td>
                <td>NaN</td>
                <td>Ratio</td>
              </tr>
          </table>
    </body>
    <script>

      var url = "/getParams";
      var xhr = new XMLHttpRequest();
      xhr.open('GET', url, true);

      // If specified, responseType must be empty string or "text"
      xhr.responseType = 'text';

      xhr.onload = function () {
          if (xhr.readyState === xhr.DONE) {
              if (xhr.status === 200) {
                  fillTable(xhr.responseText);
              }else
              {
                fillTable("");
              }
          }
      };

      xhr.send(null);

      function fillTable(text)
      {
        var table = document.getElementById("parameterTable").rows;
        if(text == "" || text == "error")
        {
          for(var i = 1; i<table.length; ++i)
          {
            table[i].cells[1].textContent = "Not available";
          }
          return;
        }

        var params = text.split(',');
        params[0] = params[0].substring(1);
        params[params.length - 1] = params[params.length-1].substring(0, params.length);
        var k = 0;
        for(var i = 1; i<table.length; ++i)
        {
          table[i].cells[1].textContent = params[k++];
        }
      }
      
    </script>
</html>)rawliteral";

String answer; // string to read parameters of arduino

void setup()
{
  Serial.begin(BAUDR);

  const char* ssid = ""; // enter wifi ssd here
  const char* passwd = ""; // enter wifi password here

  WiFi.begin(ssid, passwd);

  Serial.write("\nConnecting");
  while (WiFi.status() != WL_CONNECTED) // waiting state-not connected
  {
    delay(500);
    Serial.write(".");
  }
  Serial.write("Connected to Wifi: ");
  Serial.write(WiFi.SSID().c_str());
  Serial.write("\nConnected, IP address: ");
  char ip_address[18]; // buffer for current ip address
  IPAddress ip = WiFi.localIP();
  sprintf(&ip_address[0], "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  Serial.write(ip_address);

  // website with html table, javascript updates values of table of url /getParams
  server.on("/", [](){
      server.send(200, "text/html", htmlMessage);      
  });

  // print param_buffer to website as text
  server.on("/getParams", [](){
   server.send(200, "text/plain", param_buffer);
  });
  
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin();                           // Actually start the server
}

void loop()
{
  server.handleClient(); // Listen for HTTP requests from clients

  while(Serial.available() > 20)
  {
    Crc16 crc; // crc object
    char tmp_buffer[MAX_RECV_BYTES]; // hold recieved data
    answer = Serial.readString(); // read whole content into string
    memset(tmp_buffer, 0, sizeof(tmp_buffer)); // clear buffer with memset
    strncpy(tmp_buffer, answer.c_str(), answer.length()); // copy to tmp_buffer
    byte* ptr = (byte*) &tmp_buffer[0] + strlen(tmp_buffer) - 2; // set pointer to upper crc byte
    unsigned short crc_checksum = (unsigned short) (*ptr) << 8; // convert upper byte of crc to short
    *ptr++;
    crc_checksum |= (unsigned short) (*ptr); // convert lower byte of crc
    tmp_buffer[strlen(tmp_buffer) - 2] = 0; // remove crc (2 bytes) of tmp_buffer
    unsigned short calc_checksum = crc.XModemCrc((byte*) tmp_buffer, 0, strlen(tmp_buffer)); // calculate crc of data

    if(crc_checksum != calc_checksum) // check if calculated crc is equal to recieved crc
    {
      char error_str[] = "error";
      strcpy(param_buffer, error_str); // copy error string to global param_buffer
    }
    strncpy(param_buffer, tmp_buffer, strlen(tmp_buffer)); // update global param_buffer to dispaly on website
  }
}

// invalid sources
void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
