# arduino_proxy
Wifi to ethernet proxy using an Arduino. Primarily used to access service interface on SunPower PVS5 & PVS6 residential solar monitor to integrate with Home Assistant. Inspired by Raspberry Pi proxy linked below.

## Related projects
[Home Assistant](https://www.home-assistant.io/)  
[HASS Sunpower](https://github.com/krbaker/hass-sunpower)  
[Raspberry Pi ethernet proxy {PDF}](https://starreveld.com/PVS6%20Access%20and%20API.pdf)  

## Hardware
Built and tested with Arduino Uno Wifi v2 & Arduino Ethernet shield v2.  
[Arduino Uno Wifi v2](https://www.digikey.com/en/products/detail/ABX00021/1050-1166-ND/9486717)  
[Arduino Ethernet shield v2](https://www.digikey.com/en/products/detail/A000024/1050-1039-ND/3476356)  
Approximate hardware cost (2022-10-01): $89 USD + tax and shipping.

## How to use
Add wireless SSID & password to secrets.h. Deploy to arduino and watch logs.
