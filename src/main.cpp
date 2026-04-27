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

//Joystick Fahrdaten (-100 bis 100)
volatile int targetX=0;
volatile int targetY=0;
float smoothX=0;
float smoothY=0;
int cmdLCurrent=0;
int cmdRCurrent=0;
bool hupeAktiv=false;

//Timing für nicht blockierende Ansteuerung
unsigned long lastStepMicrosL=0;
unsigned long lastStepMicrosR=0;
unsigned long lastHupeMillis=0;

const int JOYSTICK_DRIVE_DEADZONE = 8;
const int JOYSTICK_TURN_DEADZONE = 3;
const int CMD_RAMP_STEP = 3;
const int REVERSE_GUARD = 18;
const float INPUT_ALPHA = 0.22f;
const unsigned long HUPE_INTERVAL_MS = 250;

int rampTo(int current, int target, int step) {
  if (current < target) {
    return min(current + step, target);
  }
  if (current > target) {
    return max(current - step, target);
  }
  return current;
}

void setDrive(int x, int y) {
  targetX = constrain(x, -100, 100);
  targetY = constrain(y, -100, 100);
}


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

      .joystick-wrap {
        display: inline-block;
        margin-top: 16px;
      }

      .joystick-pad {
        width: 220px;
        height: 220px;
        border-radius: 50%;
        border: 3px solid #003265;
        background: radial-gradient(circle at 30% 30%, #f7fbff, #d4e3f6);
        position: relative;
        touch-action: none;
        user-select: none;
      }

      .joystick-stick {
        width: 74px;
        height: 74px;
        border-radius: 50%;
        background: #003265;
        position: absolute;
        left: 73px;
        top: 73px;
        box-shadow: 0 8px 16px rgba(0,0,0,0.25);
      }

      .drive-text {
        margin-top: 8px;
        color: #003265;
        font-weight: bold;
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

    <p align=middle>
    __________________________
    </p>

    <div class="joystick-wrap">
      <div class="joystick-pad" id="joyPad">
        <div class="joystick-stick" id="joyStick"></div>
      </div>
      <div class="drive-text" id="driveText">X: 0 | Y: 0</div>
    </div>

    <h1>Viel Spass mit deinem Auto :)


     
    
   <script>
   function toggleCheckbox(x) {
     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/" + x, true);
     xhr.send();
   }

   var joyPad = document.getElementById('joyPad');
   var joyStick = document.getElementById('joyStick');
   var driveText = document.getElementById('driveText');
   var joyRadius = 73;
   var joyCenter = 110;
  var lastSend = 0;
  var joyActive = false;

   function sendDrive(x, y) {
     var now = Date.now();
    if (now - lastSend < 25) return;
     lastSend = now;

     var xhr = new XMLHttpRequest();
     xhr.open("GET", "/drive?x=" + x + "&y=" + y, true);
     xhr.send();
     driveText.textContent = "X: " + x + " | Y: " + y;
   }

   function updateStick(clientX, clientY) {
     var rect = joyPad.getBoundingClientRect();
     var x = clientX - rect.left - joyCenter;
     var y = clientY - rect.top - joyCenter;
     var dist = Math.sqrt(x * x + y * y);

     if (dist > joyRadius) {
       x = x * joyRadius / dist;
       y = y * joyRadius / dist;
     }

     joyStick.style.left = (x + joyCenter - 37) + "px";
     joyStick.style.top = (y + joyCenter - 37) + "px";

     var driveX = Math.round((x / joyRadius) * 100);
     var driveY = Math.round((-y / joyRadius) * 100);
     sendDrive(driveX, driveY);
   }

   function resetStick() {
     joyStick.style.left = "73px";
     joyStick.style.top = "73px";
     sendDrive(0, 0);
   }

   joyPad.addEventListener('pointerdown', function(e) {
     joyActive = true;
     joyPad.setPointerCapture(e.pointerId);
     updateStick(e.clientX, e.clientY);
   });

   joyPad.addEventListener('pointermove', function(e) {
     if (!joyActive) return;
     updateStick(e.clientX, e.clientY);
   });

   joyPad.addEventListener('pointerup', function() {
     joyActive = false;
     resetStick();
   });

   joyPad.addEventListener('pointercancel', function() {
     joyActive = false;
     resetStick();
   });

   joyPad.addEventListener('pointerleave', function() {
     if (!joyActive) return;
     joyActive = false;
     resetStick();
   });
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
    hupeAktiv=false;
    setDrive(0, 100);
    request->send(200, "text/plain", "ok");
  });


  //Befehl vom Webserver, Schalter für links gedrückt
  server.on("/links", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=2;
    hupeAktiv=false;
    setDrive(-100, 0);
    request->send(200, "text/plain", "ok");
  });

  server.on("/rechts", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=3;
    hupeAktiv=false;
    setDrive(100, 0);
    request->send(200, "text/plain", "ok");
  });
 
  server.on("/back", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=4;
    hupeAktiv=false;
    setDrive(0, -100);
    request->send(200, "text/plain", "ok");
  });

  server.on("/HUPE", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=5;
    hupeAktiv=true;
    request->send(200, "text/plain", "ok");
  });

  server.on("/drive", HTTP_GET, [] (AsyncWebServerRequest *request) {
    int x = 0;
    int y = 0;

    if (request->hasParam("x")) {
      x = request->getParam("x")->value().toInt();
    }

    if (request->hasParam("y")) {
      y = request->getParam("y")->value().toInt();
    }

    hupeAktiv=false;
    setDrive(x, y);
    request->send(200, "text/plain", "ok");
  });

  server.on("/off", HTTP_GET, [] (AsyncWebServerRequest *request) {
    schalter=0;
    hupeAktiv=false;
    setDrive(0, 0);
    request->send(200, "text/plain", "ok");
  });

 server.onNotFound(notFound);
  server.begin();

}

void loop() { //Endlosschleife

    altschalter=schalter;
    altLED=LED;

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

    int x = targetX;
    int y = targetY;

    //Joystick Eingaben glätten
    smoothX += INPUT_ALPHA * ((float)x - smoothX);
    smoothY += INPUT_ALPHA * ((float)y - smoothY);

    int filteredX = (int)roundf(smoothX);
    int filteredY = (int)roundf(smoothY);

    if (abs(filteredX) < JOYSTICK_TURN_DEADZONE) filteredX = 0;
    if (abs(filteredY) < JOYSTICK_DRIVE_DEADZONE) filteredY = 0;

    //Car-ähnliches Lenkverhalten: bei wenig Gas weniger harte Lenkung
    int steer = (filteredX * (50 + abs(filteredY))) / 150;
    int cmdLTarget = constrain(filteredY + steer, -100, 100);
    int cmdRTarget = constrain(filteredY - steer, -100, 100);

    //Befehle weich rampen statt springen
    cmdLCurrent = rampTo(cmdLCurrent, cmdLTarget, CMD_RAMP_STEP);
    cmdRCurrent = rampTo(cmdRCurrent, cmdRTarget, CMD_RAMP_STEP);

    //Kurz um Null halten, damit Richtung nicht flattert
    if (abs(cmdLCurrent) < REVERSE_GUARD && ((cmdLCurrent > 0 && cmdLTarget < 0) || (cmdLCurrent < 0 && cmdLTarget > 0))) {
      cmdLCurrent = 0;
    }

    if (abs(cmdRCurrent) < REVERSE_GUARD && ((cmdRCurrent > 0 && cmdRTarget < 0) || (cmdRCurrent < 0 && cmdRTarget > 0))) {
      cmdRCurrent = 0;
    }

    bool enableL = abs(cmdLCurrent) > JOYSTICK_DRIVE_DEADZONE;
    bool enableR = abs(cmdRCurrent) > JOYSTICK_DRIVE_DEADZONE;
    bool driveActive = enableL || enableR;

    //Motoren nur aktivieren wenn wirklich Fahrbefehl anliegt
    digitalWrite(EN_L, driveActive ? LOW : HIGH);
    digitalWrite(EN_R, driveActive ? LOW : HIGH);

    if (enableL) {
      digitalWrite(DIR_L, (cmdLCurrent > 0) ? HIGH : LOW);
    }

    if (enableR) {
      digitalWrite(DIR_R, (cmdRCurrent > 0) ? LOW : HIGH);
    }

    unsigned long nowMicros = micros();

    if (enableL) {
      int absL = abs(cmdLCurrent);
      unsigned long intervalL = map(absL, JOYSTICK_DRIVE_DEADZONE, 100, 5200, 1000);
      if (nowMicros - lastStepMicrosL >= intervalL) {
        STEP_L = !STEP_L;
        digitalWrite(STP_L, STEP_L);
        lastStepMicrosL = nowMicros;
      }
    } else {
      STEP_L = LOW;
      digitalWrite(STP_L, STEP_L);
      lastStepMicrosL = nowMicros;
    }

    if (enableR) {
      int absR = abs(cmdRCurrent);
      unsigned long intervalR = map(absR, JOYSTICK_DRIVE_DEADZONE, 100, 5200, 1000);
      if (nowMicros - lastStepMicrosR >= intervalR) {
        STEP_R = !STEP_R;
        digitalWrite(STP_R, STEP_R);
        lastStepMicrosR = nowMicros;
      }
    } else {
      STEP_R = LOW;
      digitalWrite(STP_R, STEP_R);
      lastStepMicrosR = nowMicros;
    }

    //Nicht blockierende Hupe, damit Fahren gleichzeitig möglich bleibt
    if (hupeAktiv) {
      if (millis() - lastHupeMillis >= HUPE_INTERVAL_MS) {
        lastHupeMillis = millis();
        digitalWrite(hupePIN, !digitalRead(hupePIN));
      }
    } else {
      digitalWrite(hupePIN, LOW);
    }
}
