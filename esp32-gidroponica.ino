#include <Arduino.h>

#define WIFI_SSID "black"
#define WIFI_PASS "123qwe123"
#define BOT_TOKEN "1126355382:AAFJ-z5wEl01Pmb3tLmV8TFmwo_s8vSJ8z8"
#define CHAT_ID "269356443"

#include <FastBot2.h>
FastBot2 bot;

#include <iarduino_I2C_pH.h>                      //   Подключаем библиотеку для работы с pH-метром I2C-flash.
iarduino_I2C_pH sensor(0x09); 

#include <Thread.h>  // подключение библиотеки ArduinoThread
Thread pHThread = Thread(); // создаём поток управления pH сенсором
Thread GetTimeThread = Thread(); // создаём поток получения времени

//WiFiServer TelnetServer(23);
//WiFiClient Telnet;

int ledState = LOW;  // ledState used to set the LED
unsigned long previousMillis = 0;  // will store last time LED was updated

float pHvalue = 0;
int pHProbeInterval = 5; // время опроса датчика pH, в минутах
int GetTimeInterval = 180; // время обновления внутренних часов МК, в минутах

char* __stack_start = nullptr;
void stackStart() {
    char c;
    __stack_start = &c;
}
void stackPrint() {
    char c;
    Serial.println(int32_t(__stack_start - &c));
}
uint32_t _ms;
void loopStart() {
    _ms = millis();
}
void loopPrint() {
    Serial.print("exec. time: ");
    Serial.println(millis() - _ms);
}

void handleCommand(fb::Update& u) {
  Text chat_id = u.message().chat().id();

  switch (u.message().text().hash()) {
    case SH("/start"): {
      fb::Message msg;
      msg.text = F(
        "Help:\r\n"
        "/send_msg\r\n");
      msg.chatID = chat_id;
      bot.sendMessage(msg);
    } break;

    case SH("/send_msg"): {
      bot.sendMessage(fb::Message("Some text", chat_id));
    } break;
  }
}

void handleMessage(fb::Update& u) {
    if (u.isMessage()) {
        // Serial.println(u.message().date());
        // Serial.println(u[tg_apih::text]);
        // Serial.println(u.message().text().decodeUnicode());
        Serial.println(u.message().text());
        Serial.println(u.message().from().username());
        Serial.println(u.message().from().id());

        // handleCommand
        if (u.message().text().startsWith('/')) {
            handleCommand(u);
        } else {
            // эхо, вариант 1
            // fb::Message msg;
            // msg.text = u.message().text().toString();
            // msg.chatID = u.message().chat().id();
            // bot.sendMessage(msg, false);

            // эхо, вариант 2
            bot.sendMessage(fb::Message(u.message().text().toString(), u.message().chat().id()));

            // ============================
            // удалить сообщение юзера
            // bot.deleteMessage(u.message().chat().id(), u.message().id());
        }

        // ============================
        // изменить последнее сообщение на текст из чата
        // if (bot.lastBotMessage()) {
        //     fb::TextEdit et;
        //     et.text = u.message().text().toString();
        //     et.chatID = u.message().chat().id();
        //     et.messageID = bot.lastBotMessage();
        //     bot.editText(et);
        // } else {
        //     bot.sendMessage(fb::Message(u.message().text(), u.message().chat().id()));
        // }
    }
}

void updateh(fb::Update& u) {
    Serial.println("NEW MESSAGE");
    Serial.println(u.message().from().username());
    Serial.println(u.message().text());

    if (u.message().hasDocument() && u.message().document().name().endsWith(".bin") && u.message().caption() == WiFi.localIP().toString()) {
      bot.sendMessage(fb::Message("OTA update for " + WiFi.localIP().toString(), u.message().chat().id()));
      bot.updateFlash(u.message().document(), u.message().chat().id());
    }

    if (u.isMessage()) handleMessage(u);
}

void setup() {
    pinMode(2, OUTPUT); // встроенный светодиод на D2

    Serial.begin(115200);
    Serial.println();

    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected");

    digitalWrite(2, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(500);                      // wait for a second
    digitalWrite(2, LOW);   // turn the LED off by making the voltage LOW
    delay(500);   
    digitalWrite(2, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(500);                      // wait for a second
    digitalWrite(2, LOW);   // turn the LED off by making the voltage LOW
    delay(500);  
    digitalWrite(2, HIGH);  // turn the LED on (HIGH is the voltage level)
    delay(500);                      // wait for a second
    digitalWrite(2, LOW);   // turn the LED off by making the voltage LOW
    
    GetTime(); // Обновление внутреннего времени МК

    sensor.begin(&Wire);          // Инициируем работу с pH-метром I2C-flash, указав ссылку на объект для работы с шиной I2C на которой находится модуль (по умолчанию &Wire).
  
    pHThread.onRun(pHRead);  // назначаем потоку задачу
    pHThread.setInterval(pHProbeInterval * 60 * 1000); // задаём интервал срабатывания - минимум минута, если pHProbeInterval = 1

    GetTimeThread.onRun(GetTime);  // назначаем потоку задачу
    GetTimeThread.setInterval(GetTimeInterval * 60 * 1000); // задаём интервал обновления втутренних часов МК

    //TelnetServer.begin();
    //TelnetServer.setNoDelay(true);

    // ============
    bot.attachUpdate(updateh);   // подключить обработчик обновлений
    bot.setToken(F(BOT_TOKEN));  // установить токен

    // режим опроса обновлений. Самый быстрый - Long
    // особенности читай тут в самом низу
    // https://github.com/GyverLibs/FastBot2/blob/main/docs/3.start.md

    // bot.setPollMode(fb::Poll::Sync, 4000);  // умолч
    // bot.setPollMode(fb::Poll::Async, 4000);
    bot.setPollMode(fb::Poll::Long, 20000);

    // welcome message
    bot.sendMessage(fb::Message(GetLocalTimeF() 
      + ", IP address: " + WiFi.localIP().toString() 
      + "\r\nОпрос датчика pH - каждые: " + String(pHProbeInterval) + " минут."
      + "\r\nОбновление внутреннего времени МК - каждые: " + String(GetTimeInterval) + " минут."
      , CHAT_ID));
}

void loop() {
    uint32_t ms = millis();

    // тикаем в loop!
    bot.tick();

    if (bot.canReboot()) ESP.restart();

    //if (ms > 60000) Serial.println(String("1m: pH measures time!"));
    //if (ms > 120000) Serial.println(String("2m: solute-temp measures time!"));

    // if ((ms - previousMillis >= 60000) && (ledState == LOW)) {
    //   Serial.println(String("1m: pH & solute-temp measures time!"));
    //   ledState = HIGH;
    //   digitalWrite(2, ledState);

    //   Serial.print("Кислотность = " );             //
    //   pHvalue = sensor.getPH();
    //   Serial.print(pHvalue ,1);             //   Выводим водородный показатель жидкости с 1 знаком после запятой.
    //   //Serial.print(sensor.getPH() ,1);             //   Выводим водородный показатель жидкости с 1 знаком после запятой.
    //   Serial.print(" pH.\r\n"       );
    //   bot.sendMessage(fb::Message("Ph: " + String (pHvalue), CHAT_ID));
    // }
    // if ((ms - previousMillis >= 67000) && (ledState == HIGH)) {
    //   //previousMillis = 0;
    //   previousMillis = ms;
    //   ledState = LOW;
    //   digitalWrite(2, ledState);
    // }

    // Проверим, пришло ли время прочитать pH:
    if (pHThread.shouldRun())
      pHThread.run(); // запускаем поток

    // Проверим, пришло ли время обновить внутреннее время МК
    if (GetTimeThread.shouldRun())
      GetTimeThread.run(); // запускаем поток

    // // look for Client connect trial
    // if (telnetServer.hasClient()) {
    //   if (!serverClient || !serverClient.connected()) {
    //     if (serverClient) {
    //       serverClient.stop();
    //       Serial.println("Telnet Client Stop");
    //     }
    //     serverClient = telnetServer.available();
    //     Serial.println("New Telnet client");
    //     serverClient.flush();  // clear input buffer, else you get strange characters 
    //   }
    // }

    // while(serverClient.available()) {  // get data from Client
    //   erial.write(serverClient.read());
    // }

      // if (TelnetServer.hasClient()){
      //   if (!Telnet || !Telnet.connected()){
      //     if(Telnet) Telnet.stop();
      //     Telnet = TelnetServer.available();
      //   } else {
      //     TelnetServer.available().stop();
      //   }
      // }
      // if (Telnet && Telnet.connected() && Telnet.available()){
      //   while(Telnet.available())
      //     Serial.write(Telnet.read());
      // }

      // ms = millis() - ms;
      // if (ms > 100) Serial.println(String("--------- looptime: ") + ms);
    }

// Поток pH:
void pHRead() { 
  pHvalue = sensor.getPH();
  if (pHvalue != 0)
  {
    Serial.print("Кислотность = " );             //
    Serial.print(pHvalue ,1);             //   Выводим водородный показатель жидкости с 1 знаком после запятой.
    //Serial.print(sensor.getPH() ,1);             //   Выводим водородный показатель жидкости с 1 знаком после запятой.
    Serial.print(" pH.\r\n"       );
    bot.sendMessage(fb::Message(GetLocalTimeF() + ", pH: " + String (pHvalue), CHAT_ID));
  }
}

// ----- возвращает текущее время -----
String GetLocalTimeF() {
  String month;
  String hour, minute, second;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return ("No time!");
  //return (&timeinfo, "%d.%m.%Y %H:%M:%S");

  if (String(timeinfo.tm_hour).length() == 1) 
    {hour = "0" + String(timeinfo.tm_hour);}
  else
    {hour = String(timeinfo.tm_hour);}
  
  if (String(timeinfo.tm_min).length() == 1) 
    {minute = "0" + String(timeinfo.tm_min);}
  else
    {minute = String(timeinfo.tm_min);}
  
  if (String(timeinfo.tm_sec).length() == 1) 
    {second = "0" + String(timeinfo.tm_sec);}
  else
    {second = String(timeinfo.tm_sec);}

  return (String)
    int(timeinfo.tm_year + 1900) + "-" + int(timeinfo.tm_mon + 1) + "-" + timeinfo.tm_mday + " " + hour + ":" + minute + ":" + second;
}
// ----- возвращает текущее время -----
    
// ============
void GetTime() {
  configTime(7200, 3600, "pool.ntp.org");
  Serial.print("Retrieving time ");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(". ");
    delay(1000);
    now = time(nullptr);
  }
  Serial.print(GetLocalTimeF());
}
// ============