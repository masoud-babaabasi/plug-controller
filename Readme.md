# Smart plug controller

This is a collaboration between me and `Radesh-F`.

In this project, a wall plug is controlled over the wifi using REST API. It currently uses some Android app to communicate with the board written by `Radesh`.

First version:
![plug-first version](./smart-plug.jpg)

It can handle output power below 2000 Watts. The green LED indicates the state of the output relay.

The red LED shows the status of the WIFI connection. When it turns on, it will search for the last WIFI that it had connected successfully. If it connects successfully to that WIFI router, the red LED blinks every 1 second. During attempting to connect to WIFI, the red LED would blink every 250 ms. If the connection fails, it would make an access point named `Rosha_Controller` whose password is `123456789`. The Webserver IP on this access point is always fixed on `192.168.4.1` and you can communicate with the device via this IP address.

After a successful connection, this device would send a request to a server which is saved on the device's internal memory. The server address could be modified via an API. This request contains this information in `JSON` format.
- field : "device_ip"
- ip : (device IP address on the network)
- ssid : (network ssid which is connected to)
- device_type : "plug"
- unique_id : (device 64-bit UID)

## device APIs
Here is the complete list of the APIs to communicate with the plug controller device.

|   API Address    | Type |       Body        |   Respones    |
|------------------|------|-------------------|---------------|
|/api/v1/wifi/config | POST |{"ssid": , "pass" : , "id": }|{"success": ,"msg": , "data": }|
|/api/v1/wifi/status | GET |-|{"SSID": , "password" : , "hotspot": , "IP": }|
|/api/v1/wifi/address | POST |{"address": }|{"success": ,"msg": }|
|/api/v1/wifi/address | GET |-|{"http address for request": }|
|/api/v1/change/status | POST |{"status": "off" / "on"}|{"success": ,"status": ,"msg": }|
|/api/v1/change/key | POST |{"status": "disable" / "enable"}|{"success": ,"status": ,"msg": }|
|/api/v1/status | GET |-|{"status": "off" / "on"}|
|/api/v1/key | GET |-|{"status": "disable" / "enable"}|
|/api/v1/time/get | GET |-|{"date": , "time": }|
|/api/v1/time/set| POST |{"date": , "time": }|{"success": ,"msg": }|
|/api/v1/firmware/update| POST |-|{"success": ,"msg": }|
|/api/v1/schedule/status| POST |{"num": , "crons": }|{"success": ,"msg": }|
|/api/v1/schedule/status| GET |-|{"num": , "crons": }|

### Output APIs
To change the output switch on or off using `/api/v1/change/status` API.
To get the status of output put switch use `/api/v1/status`,

To disable the hardware input button on the device `/api/v1/change/key` and to check to see if it is enabled or disabled use `/api/v1/key`.
### Wifi APIs
Use `/api/v1/wifi/config` to send the desired WIFI configuration to the device to try to connect to that access point.

Use `/api/v1/wifi/status` to check the device connection to WIFI and see if its own access point is on or off. If the access point is on the web server IP on its hotspot is `192.168.4.1`.

`/api/v1/wifi/address` POST HTTP request is used to send the main server address to the device for communication. The GET HTTP request is to receive the saved address on the device's memory.
### WIFI firmware update
You can upload the binary data on the main server and command the device to download the binary code from the main server and update itself.
First, make a POST HTTP request to `/api/v1/firmware/update` API with an empty body. The device then sends a POST request to the main server on the `/api/v1/files/update/` API. The body contains the device type and software version.{"type": "plug , "version": }. If the request is successful, the response will contain a binary file download address.{"data": {"file": } }.

### Schedule API
You can send schedules to the device to turn on or off at certain times.
The body should contain the number of crons. The "crons" field contains the schedule data and they are separated with a `~` sign. Each schedule has 8 fields:
1) minute(0-59) 
2) hour(0-23) 
3) date(1-31) 
4) month(1-12) 
5) Day of the week(0-6) 
6) year 
7) action(0:off 1:on 2:toggle) 
8) activation(0:disable 1:enable)

Second is always considered zero.

The crons field date and time field could be `*` which means all valid values for that field, `/` to make a period, `-` to specify a range and `,` to specify separate values. Here is an example:
```JSON
{
    "num": "3",
    "crons": "20 12 * * * * 1 1~*/15 * * * 1,3,5 * 2 0"
}
```