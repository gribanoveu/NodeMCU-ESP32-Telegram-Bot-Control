#ifdef ESP32
  #include <WiFi.h>
  #include <WebServer.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <ElegantOTA.h>       
#include <WiFiClientSecure.h> 
#include <UniversalTelegramBot.h> 
#include <ArduinoJson.h> // необходима 5 версия библиотеки
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#define ONE_WIRE_BUS 14 // DS18B20 D5 pin of nodemcu

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensors(&oneWire);
uint8_t sensor_DS18B2[8] = {0x28, 0x17, 0xBC, 0xBC, 0x33, 0x20, 0x01, 0x88};
Adafruit_BME280 bme; 

const char *ssid = "ИМЯ ТВОЕГО WiFi";
const char *password = "ПАРОЛЬ ТВОЕГО WiFi";

#ifdef ESP32
  WebServer server(80);
#else
  ESP8266WebServer server(80);
#endif

// Список разрешенных пользователей, каждый новый id в новой строке:
define CHAT_ID_1 "XXXXXXXXX"

// Токен, полученный от BotFather
define BOTtoken "XXXXXX:XXXXXXXXX" 
// подключение к сети
WiFiClientSecure client; 
UniversalTelegramBot bot(BOTtoken, client);
// проверка сообщений раз в секунду
int botRequestDelay = 1000; 
unsigned long lastTimeBotRun;
// массив разрешенных id 
String allowed_id[] = {CHAT_ID_1};


void setup()
{
  Serial.begin(115200);
  #ifdef ESP8266
    client.setInsecure();
  #endif
  checkBme(); 
  tempSensors.begin(); 
  connectWiFi();
  startOTA();
  
}

void loop() {
  server.handleClient();
  telegramCheckMessage();
}

// работа телеграмм бота
void telegramCheckMessage() {

  if (millis() > lastTimeBotRun + botRequestDelay)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages)
    {
      Serial.println("Сервер получил запрос: ");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRun = millis();
  }
}

// проверка подключения BME280
void checkBme() {
  if (!bme.begin(0x76)) {
    Serial.println("Не найден сенсор BME280, проверьте подключение");
    while (1);
  }
}

// старт обновления по воздуху
void startOTA(){
  server.on("/", []() {
    server.send(200, "text/plain", "Hi! Let`s upgrade)");
  });
  ElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  Serial.println("HTTP сервер запущен");
  Serial.println(WiFi.localIP());
}

// подключение к сети
void connectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Подключение к WiFi...");
  }
  Serial.print("Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}

// функция получения показаний с датчика BME280
String getReadingsBME280(){ 
  float temperature, humidity, preassure;

  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  // перевод давления в мм. рт. с.
  preassure = bme.readPressure() / 133.32F;

  // формирование стороки
  String message = "Температура: " + String(temperature) + " ºC \n";
  message += "Влажность: " + String(humidity) + " % \n";
  message += "Давление: " + String(preassure) + " мм. рт. ст.";

  return message;
}

// функция получения показаний с датчика DS18B20
String getTempOutside()
{
  int temp;

  tempSensors.requestTemperatures();
  temp = tempSensors.getTempC(sensor_DS18B2);

  String message = "Температура на улице: " + String(temp) + " ºC \n";
  return message;
}

// что произойдет если получено сообщение
void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    int allowed = 0;

    // если пользователь в списке разрешенных
    for (int j = 0; allowed_id[j].length() != 0; j++) {
      if (allowed_id[j] == chat_id)
        allowed = 1;
    }

    // если да, то вывод его id в консоль
    if (allowed == 1) {
      Serial.println("Allowed user: " + chat_id);
    } else {
      Serial.println("Not Allowed user: " + chat_id);
    }

    // если пользователь допущен
    if (chat_id != 0) {

      // телеграмм возвращает русскоязычные запросы в формате кодов символов юникода,
      // для добавления новых команд необходимо перевести текс в код символов юникода
      // и удалить слеш (\) между символами

      if (text == "/start") {
        String keyboardJson = "[[\"Температура на улице\"],[\"Температура в доме\"]]";
        bot.sendMessageWithReplyKeyboard(chat_id, "-----Запуск метеостанции-----", "", keyboardJson, true);
      }

      // кнопка температура в доме
      if (text == "u0422u0435u043cu043fu0435u0440u0430u0442u0443u0440u0430 u0432 u0434u043eu043cu0435") {
        String readings = getReadingsBME280();
        bot.sendMessage(chat_id, readings, "");
      }

      // кнопка температура на улице
      if (text == "u0422u0435u043cu043fu0435u0440u0430u0442u0443u0440u0430 u043du0430 u0443u043bu0438u0446u0435") {
        String readings = getTempOutside();
        bot.sendMessage(chat_id, readings, "");
      }
    }
  }
}
