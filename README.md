# CryptoDisplay
A MAX7219 and ESP8266 based cryptocurrency display

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

!! CAREFUL !! At the time of writing (22/05/18), the Arduino Board Manager has the v2.4.1 of ESP8266. This version is known to have a memory leak problem and I was struggling alot with this version ! The solution is to manually add the board with the lastest Git version. Follow the link to do it : https://github.com/esp8266/Arduino/blob/master/README.md#using-git-version

If the Arduino Board Manager version is > 2.4.1 or that you don't mind the memory link, follow these steps  :

First thing is to add board support to your board manager. Inside the Arduino IDE, click on "Preferences". Under "Additional Boards Manager URLs, add : 

```
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

Next, by going to "Boards Manager" you should be able to add "esp8266" by ESP8266 Community.

After that, you want to install three libraries : 
```
ArduinoJson 5.13.2 by Benoit Blanchon
MD_MAX72XX v2.10.0 by majicDesigns
MD_Parola v2.7.4 by majicDesigns
```


### Installing

There's only one thing that you need to do in order for the libraries to work.
Inside ```Arduino > libraries > MD_MAX72XX > src > MD_MAX72xx.h```, you need to choose the right hardware your display is using.

Put a 1 to the corresponding line.

Example :
For FC-16 based display :
```
#define	USE_FC16_HW	1
```


## Authors

* **Victor MEUNIER** - *CryptoDisplay* - [MrEliptik](https://github.com/MrEliptik)


## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details
