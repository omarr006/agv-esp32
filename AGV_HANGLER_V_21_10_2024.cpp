//Hangler Anton V_21_10_2024
//AGV Software für Schüler Version 3.0
//Durchkommentierte Version
//langsames Anfahren
//RGB über Webserver steuerbar
//Verfasst am 18.02.2024 / letzte Aenderung: 21.10.2024
//Getestet auf ESP32-Wroom-32D -> voll funktionstüchtig
//Funktional für ESP32


#include <WiFi.h> //Wlanverbindung
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>  //Web-Server
#include <LiquidCrystal_I2C.h>  //LCD Display

//LCD Display
int lcdColumns = 16;
int lcdRows = 2;

//LCD Display: Ardesse setzen (0x27 vom Display vorgegeben), Zeilen und Reihen angeben
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

//Variablen Motoransteuerung (Pins von der Motorsteuerung)
//linker Motor
int EN_L = 25; //Pin zum Motor enabeln, bei LOW führt Motortreiber Schritte durch (LOW-> Motor enabled, HIGH-> Motor nicht enabled)
int DIR_L = 26; //Pin zum einstellen der Richtung des Motors
int STP_L=27; //Pin über den der Schrittbefehl an Motortreiber weitergegeben wird
bool STEP_L=LOW; //mit der Variable werden die Schritte ausgeführt
//rechter Motor
int EN_R = 14;
int DIR_R = 12;
int STP_R=13;
bool STEP_R=LOW;

//Veriablen zur Veränderung des Modus
int schalter=0;
int altschalter;     //zur Vestellung von Veränderung von schalter

int multi;          //damit langsamer dreht

//LED-PINNUMMERN
int rotLED=16;
int gruenLED=0;
int blauLED=4;

//LED-Zustände
bool rot=LOW;
bool gruen=LOW;
bool blau=LOW;
int LED=0;
int altLED=0;

//PIN der Hupe
int hupePIN=17;

//Variable zum langsamen anfahren
int zeit=0;
//varible zum langsam fahren
int i=0;    //variable für beschleunigung


//Website
//keine Kommentare möglich
//color -> Schriftfarbe
//background-color -> Farbe der Buttons
//#003265->Farbe des HTL-Logos
//box-shadow -> Schatten
//button:active -> Farben wenn der Schalter gedrückt wird

//Button erstellen:       Befehl wenn mit Maus gedrückt         Befehl wenn mit Touch gedrückt          Befehl wenn Maus nicht mehr drückt  -"- mit touch                     >Beschriftung des Buttons>
//<button class="button" onmousedown="toggleCheckbox('links');" ontouchstart="toggleCheckbox('links');" onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');"><</button>

//wenn Button auch nach drücken aktiviert bleiben soll, statt off, Befehl rein schreiben
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
  <head>
    <title>ESP Pushbutton Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px;}
      .button {
        padding: 10px 20px;
        font-size: 24px;
        text-align: center;
        outline: none;
        color: #fff;
        background-color: #003265; 
        border: none;
        border-radius: 5px;
        box-shadow: 0 6px #999;
        cursor: pointer;
        -webkit-touch-callout: none;
        -webkit-user-select: none;
        -khtml-user-select: none;
        -moz-user-select: none;
        -ms-user-select: none;
        user-select: none;
        -webkit-tap-highlight-color: rgba(0,0,0,0);
      }  
      .button:hover {background-color: #091b44}
      .button:active {
        background-color: #091b44;
        box-shadow: 0 4px #666;
        transform: translateY(2px);
      }
    </style>
  </head>
  <body>
    <h1>AGV Version 2.0

    <p align=middle>  
   
    </p>
    

    
      
     <p align=middle>
    __________________________   
    </p>

    <p align=middle>
    <button class="button" onmousedown="toggleCheckbox('vor');" ontouchstart="toggleCheckbox('vor');" onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');">/\</button>
    </p>

    <p align=middle>
    <button class="button" onmousedown="toggleCheckbox('links');" ontouchstart="toggleCheckbox('links');" onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');"><</button>
    <button class="button" onmousedown="toggleCheckbox('off');" ontouchstart="toggleCheckbox('off');" onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');">STOP</button>
    <button class="button" onmousedown="toggleCheckbox('rechts');" ontouchstart="toggleCheckbox('rechts');" onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');">></button>
    </p>

     <p align=middle>
    <button class="button" onmousedown="toggleCheckbox('back');" ontouchstart="toggleCheckbox('back');" onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');">\/</button>
    </p>

    <p align=middle>
    __________________________
    </p>

     <p align=middle>
    <button class="button" onmousedown="toggleCheckbox('HUPE');" ontouchstart="toggleCheckbox('HUPE');" onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');"> HUPE</button>
    </p>

    <p align=middle>
    __________________________
    </p>

     <p align=middle>
    <button class="button" onmousedown="toggleCheckbox('rot');" ontouchstart="toggleCheckbox('rot');" onmouseup="toggleCheckbox('rot');" ontouchend="toggleCheckbox('rot');"> ROT</button>
    <button class="button" onmousedown="toggleCheckbox('gelb');" ontouchstart="toggleCheckbox('gelb');" onmouseup="toggleCheckbox('gelb');" ontouchend="toggleCheckbox('gelb');"> GELB</button>
    <button class="button" onmousedown="toggleCheckbox('gruen');" ontouchstart="toggleCheckbox('gruen');" onmouseup="toggleCheckbox('gruen');" ontouchend="toggleCheckbox('gruen');"> GRUEN</button>
    </p>

     <p align=middle>
    <button class="button" onmousedown="toggleCheckbox('cyan');" ontouchstart="toggleCheckbox('cyan');" onmouseup="toggleCheckbox('cyan');" ontouchend="toggleCheckbox('cyan');"> CYAN</button>
    <button class="button" onmousedown="toggleCheckbox('blau');" ontouchstart="toggleCheckbox('blau');" onmouseup="toggleCheckbox('blau');" ontouchend="toggleCheckbox('blau');"> BLAU</button>
    <button class="button" onmousedown="toggleCheckbox('pink');" ontouchstart="toggleCheckbox('pink');" onmouseup="toggleCheckbox('pink');" ontouchend="toggleCheckbox('pink');"> PINK</button>
    </p>

     <p align=middle>
    <button class="button" onmousedown="toggleCheckbox('aus');" ontouchstart="toggleCheckbox('aus');" onmouseup="toggleCheckbox('off');" ontouchend="toggleCheckbox('off');"> AUS</button>
     </p>

    <h1>Viel Spass mit deinem Auto :)


     
    
   <script>
   function toggleCheckbox(x) {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/" + x, true);
     xhr.send();
   }
  </script>
  </body>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

AsyncWebServer server(80);

void setup() {
  //aktivieren serieller Monitor:
  Serial.begin(9600); //9600->Baudrate
  
  //LCD Display:
   lcd.init();                  
  lcd.backlight();
   lcd.setCursor(0, 0);
  lcd.print("AGV_V3_HTL_1");
  lcd.setCursor(0,1);
  lcd.print("IP Adresse");

  //Ausgänge zu Motortreiber auf OUTPUT setzten
  pinMode(EN_L, OUTPUT);
  pinMode(DIR_L, OUTPUT);
  pinMode(STP_L, OUTPUT);
  pinMode(EN_R,  OUTPUT);
  pinMode(DIR_R, OUTPUT);
  pinMode(STP_R, OUTPUT);

  //RGB LED-Pins auf OUTPUT setzten
  pinMode (gruenLED,OUTPUT);
  digitalWrite (gruenLED,LOW);
  pinMode (blauLED,OUTPUT);
  digitalWrite(blauLED,LOW);
  pinMode (rotLED,OUTPUT);
  digitalWrite(rotLED,LOW);
  pinMode (rotLED,OUTPUT);

  //Hupen-Pin auf OUTPUT setzten
  pinMode (hupePIN,OUTPUT);
  

 
  //Webserver
  WiFi.softAP("omar's agv", "Oo06302008"); // Name vom WLAN und Passwort, beim Passwort evtl nur Zahlen möglich
  //Name des WLANS möglischts von Schülern ändern lassen, damit klar ist, mit welchem AGV man verbunden ist
  IPAddress myIP = WiFi.softAPIP();
  //Ausgabe der IP-Adresse auf seriellem Monitor
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  //Ausgabe am LCD-Display
  lcd.setCursor(0,1);
  lcd.print(myIP);

  
 server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send_P(200, "text/html", index_html);
   request->send(200, "text/html", index_html);
  });


  //Befehl vom Webserver, für RGB-LED
  server.on("/rot", HTTP_GET, [] (AsyncWebServerRequest *request) {
    LED=1;
    request->send(200, "text/plain", "ok");
  });

  server.on("/gelb", HTTP_GET, [] (AsyncWebServerRequest *request) {
    LED=2;
    request->send(200, "text/plain", "ok");
  });
 
  server.on("/gruen", HTTP_GET, [] (AsyncWebServerRequest *request) {
    LED=3;
    request->send(200, "text/plain", "ok");
  });

  server.on("/cyan", HTTP_GET, [] (AsyncWebServerRequest *request) {
    LED=4;
    request->send(200, "text/plain", "ok");
  });

  server.on("/blau", HTTP_GET, [] (AsyncWebServerRequest *request) {
    LED=5;
    request->send(200, "text/plain", "ok");
  });

  server.on("/pink", HTTP_GET, [] (AsyncWebServerRequest *request) {
    LED=6;
    request->send(200, "text/plain", "ok");
  });
  
  server.on("/aus", HTTP_GET, [] (AsyncWebServerRequest *request) {
    LED=7;
    request->send(200, "text/plain", "ok");
  });

  //Befehl vom Webserver, Schalter für vor gedrückt
  server.on("/vor", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=1;
    request->send(200, "text/plain", "ok");
  });


  //Befehl vom Webserver, Schalter für links gedrückt
  server.on("/links", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=2;
    request->send(200, "text/plain", "ok");
  });

  server.on("/rechts", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=3;
    request->send(200, "text/plain", "ok");
  });
 
  server.on("/back", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=4;
    request->send(200, "text/plain", "ok");
  });

  server.on("/HUPE", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=5;
    request->send(200, "text/plain", "ok");
  });

  server.on("/off", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=0;
    request->send(200, "text/plain", "ok");
  });

 server.onNotFound(notFound);
  server.begin();

}

void loop() { //Endlosschleife

    digitalWrite(rotLED,HIGH);

    altschalter=schalter;
    altLED=LED;

    //Zustand vom Schalter abfragen
    if(schalter!=0) //bei nicht STOP, Motoren enabeln
    {
      digitalWrite(EN_L,LOW);
      digitalWrite(EN_R,LOW);
    }
    else  //bei STOP Motoren nicht enabeln
    {
      digitalWrite(EN_L,HIGH);
      digitalWrite(EN_R,HIGH);
    }

    switch(LED){ //Je nach Zustand wird RGB-LED verstellt
      case 1: //LED rot
        rot=HIGH;
        blau=LOW;
        gruen=LOW;
        break;
      case 2: //LED gelb
        rot=HIGH;
        blau=LOW;
        gruen=HIGH;
        break;
      case 3: //LED gruen
        rot=LOW;
        blau=LOW;
        gruen=HIGH;
        break;
      case 4: //LED cyan
        rot=LOW;
        blau=HIGH;
        gruen=HIGH;
        break;
      case 5: //LED blau
        rot=LOW;
        blau=HIGH;
        gruen=LOW;
        break;
      case 6: //LED magenta
        rot=HIGH;
        blau=HIGH;
        gruen=LOW;
        break;
      case 7:  //LED aus
        rot=LOW;
        blau=LOW;
        gruen=LOW;
        break;
    }

    digitalWrite(rotLED,rot);
    digitalWrite(gruenLED, gruen);
    digitalWrite(blauLED, blau);


    switch(schalter){  //je nach Zustand wird die Richtung von Motoren verändert
      case 1:   //vor
          digitalWrite(DIR_R, LOW);      // Motor rechts vor
          digitalWrite (DIR_L,HIGH);     // Motor links vor ( andere Drehrechtung wegen gespiegelter Montage
          i = 3500;
        break;
      case 2: //links Eck 
          digitalWrite(DIR_R, LOW);
          digitalWrite (DIR_L,LOW);
          i = 4500;
          break;
      case 3:  //rechts Eck
          digitalWrite(DIR_R, HIGH);
          digitalWrite (DIR_L,HIGH);
          i = 4500;
          break;
      case 4:  //zurück
          digitalWrite(DIR_R, HIGH);
          digitalWrite (DIR_L,LOW);
          i = 3500;
          break;
      case 5:  //Hupe
          while(schalter==altschalter&&LED==altLED)
          {
            digitalWrite (17,HIGH);
            delay(500); 
            digitalWrite(17,LOW);
            delay(500);
          }
          break;
    }

      
      //Fahren

      zeit=20;
        
        while(schalter==altschalter&&LED==altLED)   //langsames Anfahren
        {
          if (i >= 1000){
          digitalWrite(STP_L,!digitalRead(STP_L));    
          digitalWrite(STP_R,!digitalRead(STP_R));
          delayMicroseconds(i);  //Zeit zwischen den Schritten
          i = i-2;
          }
          else
          {
            digitalWrite(STP_L,!digitalRead(STP_L));    
            digitalWrite(STP_R,!digitalRead(STP_R));
            delayMicroseconds(1000);  //Zeit zwischen den Schritten
          }
        }   
}
