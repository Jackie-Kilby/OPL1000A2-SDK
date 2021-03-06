## Release Version:
SDK Package: MP_2.12
Patch_Lib : 4061

## Release Date: 2019/10/12

## Function & Feature Update 
1. Documents updating:
   1.1 OPL1000-multiple-dev-download-tool-user-guide,R01-v07,updated according to MP download tool v2.8 new functions.
   1.2 OPL1000-SDK-Development-guide,R01-v17, Add a chapter to introduce how to adjust heap size.

   1.3 OPL1000-AT-instruction-set-and-examples,R05-v38,Add Add AT+RFHP = 32 ,Wi-Fi LPA boost 2db and BLE LPA

2. Tool version information and update:
   2.1 download tool: v0.27. no update.
   2.2 pin-mux tool: v0.8. No update.
   2.3 MP RF test tool: v2.3. no update.
   2.4 MP multi-dev download tool: v2.8, resource FW download;mac address gets from csv file.

3. Add 32k Real Clock support API 

4. AT instruction support Low-Power WiFi TX setting


## Bug Fix List 
1. fix "Dropping packets frequently by heavy interference" issue
2. fix "OPL1000A2 TX registers adjustment (+2db)"issue
3. fix "Sleep wakeup CPOR_N release time" issue


## Notes List 

*****

## Release Version:
SDK Package: MP_2.11
Patch_Lib : 4048

## Release Date: 2019/9/27

## Function & Feature Update 
1. Documents updating:
   1.1 OPL1000-multiple-dev-download-tool-user-guide,R01-v06, add pc binding, flash write protection. 
   1.2 add OPL1000-Demo-MQTT-guide_ENG,R01-v01.
   1.3 add OPL1000-Demo-tcp-client-guide_ENG,R01-v04.
   1.4 add OPL1000-Demo-ota-wifi-guide_ENG,R01-v03.
   1.5 add OPL1000-Demo-BLE-setup-network-and-BLE-OTA-guide_ENG,R01-v07.
   1.6 update OPL1000-Power-Consumption-Measurement-Guide_ENG and OPL1000-Power-Consumption-Measurement-Guide,update figure 10,R01-v04.
2. Tool version information and update:
   2.1 download tool: v0.27. no update.
   2.2 pin-mux tool: v0.8. No update.
   2.3 MP RF test tool: v2.3. no update
   2.4 MP multi-dev download tool: v2.6, add pc binding, flash write protection.
3. LWIP:optimize tcp pcb allocation algorithm.
4. Improve offset channel performance.
5. opulinks_iot_app.apk update to v1.51.

## Bug Fix List 
1. fix "when customer post data,sometimes there may be unable to go to sleep condition for 5s" issue.
2. fix" httpclient  get file failed in cumstomer's Server" issue
3. fix "mqtt example GCC bin: OPL1000 may be hung when connecting wifi" issue
4. fix "RSSI vary with different scan mode" issue
5. fix "awsIOT_blewifi example wifi mac addr fail to update" issue



## Notes List 

*****

## Release Version:
SDK Package: MP_2.10
Patch_Lib : 4024

## Release Date: 2019/9/7

## Function & Feature Update 
1. Documents updating:
   1.1 OPL1000-multiple-dev-download-tool-user-guide,R01-v05, fix some typo. 
2. Tool version information and update:
   2.1 download tool: v0.27. no update.
   2.2 pin-mux tool: v0.8. No update.
   2.3 MP RF test tool: v2.3. no update
   2.4 MP multi-dev download tool: v2.2, support A2 chip firmware download and BLE/WIFI MAC programming.
3. Add ali_blewifi example.


## Bug Fix List 
1. fix "BLE scan fial in MP2.9 ( SCA VS VCO output bevavir abnormal)" issue
2. fix "there is no linking error report when the size is overflow" issue.
3. fix "the initial flow of driver part" issue
4. fix "RF power setting failed at BleWifiAppInit" issue
5. fix "UART rx interrupt triggered wrongly" issue.

## Notes List 

*****

## Release Version:
SDK Package: MP2.9
Patch_Lib : 4004

## Release Date: 2019/8/30

## Function & Feature Update 
1. Documents updating:
   1.1 OPL1000-SDK-Development-guide,R01-v16, Add  Setting up GUN ARM GCC toolchain and build tool Section. 
   1.2 OPL1000-AT-instruction-set-and-examples,R05-v37, Modify AT+CWLAP section’s description
   1.3 OPL1000-WIFI-BLE-API-guide,MP2.9, Support specific ssid/bssic for wifi scan
2. Tool version information and update:
   2.1 download tool: v0.27. Enhance compatibility of different boards.
   2.2 pin-mux tool: v0.8. No update.
   2.3 MP_Tool: v2.3. Fix ble mac address writing issue.
3. Add ext LDO binary to SDK.
4. support building project by GNU ARM GCC
5. Add a new MQTT example under SDK\APS_PATCH\examples\protocols\mqtt folder; This example shows how to establish a private MQTT service in local area network.
6. Add a new demo "MQTT" under Demo\MQTT to show how to use "MQTT" example to establish a local MQTT service in LAN. Here EMQ is used as MQTT broker. 
7. Support specific ssid/bssic for wifi scan in WIFI-API.

## Bug Fix List 
1. fix "peer address type is wrong for LE Enhanced Connection Complete Event" issue
2. fix "M3 may send BLE commands before M0 IPC initialization done" issue.
3. fix "Even AT command echo is off, AT task still prompts unexpected CR+LF before processing command" issue
4. fix "AP is far from LM80, DTIM will have more power consumption" issue
5. fix "peak current exceeding to 40mA during system initializing" issue.
6. fix "the size of M3 Bin built with GCC is too large" issue
7. fix "update write Wi-Fi MAC address format in OTP" issue

## Notes List 

*****

## Release Version:

SDK Package: MP v2.6
Patch_Lib : 3840

## Release Date: 2019/07/22

## Function & Feature Update 
​    1. Documents updating:

​        1.1 OPL1000-Iperf-Measurement-Guide,R01-v01, Initial Release 

​        1.2 OPL1000-Demo-BLE-setup-network-guide,R01-v05, introduce IOS APP operation and change  BLEWIFI example location file path from Bluetooth to System                     

​        1.3 OPL1000-BLEWIFI-Application-Dev-Guide, R01-v07,Modify blewifi example location path

​        1.4 OPL1000-DEVKIT-getting-start-guide, R02-v06,Update Fw_binary folder to FW_Pack folder

​        1.5 OPL1000-SDK-getting-start-guide,R02-v07, modify fw_binary folder to fw_pack folder

​        1.6 OPL1000-Demo-ota-wifi-guide,R01-v03, modify fw_binary folder to fw_pack folder

2. Tool version information and update:

​       2.1 download tool: v0.25.

​       2.2 pin-mux tool: v0.8. 

​       2.3 MP_Tool: v2.2.

3. Add IO state control before entering sleep mode.

4. Sync. AT+BLECONNPARAM to A2;

5. AT+SAVETRANSLINK set mode ERROR check.

6. Support massive and continuous parameters for AT command.


## Bug Fix List 

## Notes List 
1. Documents and Tools are inherited and updated from OPL1000-SDK MP1.8 



