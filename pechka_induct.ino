#include <LiquidCrystal_I2C.h>
#include <GyverButton.h>
#include <GyverMAX6675.h>
#include <microDS18B20.h>

MicroDS18B20<A0> sensor;

int blue_pin = A1; // Пин для СИНЕГО светодиода
int green_pin = A2; // Пин для ЗЕЛЕНОГО светодиода
int red_pin = A3; // Пин для КРАСНОГО светодиода

GButton btnReset(5);  // RESET
GButton btnSet(6);  // SET
GButton btnOk(7);  // Выбор программы или ОК
GButton btnRight(8);  // ВПРАВО
GButton btnLeft(9);  // ВЛЕВО
GButton btnDown(10); // ВНИЗ
GButton btnUp(11); // ВВЕРХ

// digitalWrite(12, LOW); // Включение реле
// digitalWrite(12, HIGH); // Выключение реле

// Пины модуля MAX6675K
#define CLK_PIN   2  // Пин SCK
#define DATA_PIN  4  // Пин SO
#define CS_PIN    3  // Пин CS

// указываем пины в порядке SCK SO CS
GyverMAX6675<CLK_PIN, DATA_PIN, CS_PIN> sens;

LiquidCrystal_I2C lcd(0x27, 16, 2); // Задаем адрес и размерность дисплея
char lcdLine[16];

byte degree[8] = // кодируем символ градуса
{
  B00111,
  B00101,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000,
};

int temp; // начальное значение температуры при загрузке
float time; // начальное значение времени в миллисекундах

int menu = 2;

bool play = false;
bool ok = false;

unsigned long previousMillis = 0;
const long interval = 1000; // интервал для измерения температуры (1 секунда)

unsigned long startTime;
int dangerTime = 3600;

bool colling = false;

void setup() {
  Serial.begin(9600);

  startTime = millis(); // сохраняем время старта

  pinMode(12, OUTPUT); // Реле
  pinMode(13, OUTPUT); // Пищалка
  
  digitalWrite(12, HIGH); // Выключение реле

  lcd.init(); // Инициализация lcd             
  lcd.backlight(); // Включаем подсветку
  lcd.createChar(1, degree); // Создаем символ под номером 

  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT);

  setDefaultVlue();
  digitalWrite(blue_pin, HIGH); // Включение СИНЕГО светодиода
}

void loop() {
  unsigned long currentMillis = millis();

  // unsigned long elapsedTime = millis() - startTime;
  // unsigned long remainingTime = countdownDuration - elapsedTime;

  btnReset.tick();
  btnSet.tick();
  btnOk.tick();
  btnRight.tick();
  btnLeft.tick();
  btnDown.tick();
  btnUp.tick();

  if (btnReset.isHolded() && !colling) {
    buzOn(); // включение пищалки на один писк
    Serial.println("reset");
    resetDevice();
    // ok = false;
  }

  if (btnOk.isClick() && !colling) {
    Serial.println("ok");
    if (menu == 1 || menu == 0) {
      ok = true;
    }    
    buzOn(); // включение пищалки на один писк
  }

  if (btnSet.isClick() && !colling) { // ЗАПУСК программы и ОСТАНОВКА при повторном нажатии
    if (ok && (menu == 0 || menu == 1)) {
      Serial.println("set");
      if (play) {
        offDeviceBlinkLed();
        play = false;
        Serial.println("set play FALSE");
        // ok = false;
      } else {
        play = true;
        Serial.println("set play TRUE");
        onDevice();
      }
    }
    buzOn(); // включение пищалки на один писк
  }

  // Serial.println(play);

  if (btnUp.isClick() && !colling) { // выбор меню ВРЕМЯ
    buzOn(); // включение пищалки на один писк
    menu = 0;
    MenuCheckTime(time);   
  }

  if (btnDown.isClick() && !colling) { // выбор меню ТЕМПЕРАТУРА
    buzOn(); // включение пищалки на один писк
    menu = 1;    
    MenuCheckTemp(temp);
  }

  if ((btnRight.isClick() || btnRight.isHold()) && !colling) { // УВЕЛИЧЕНИЕ веремни/температуры
    if (menu == 0) {
      time += 60;

      if (time > 900) {
        time = 900;
      }
    }

    if (menu == 1) {
      temp += 10;

      if (temp > 800) {
        temp = 800;
      }
    }    
  }

  if ((btnLeft.isClick() || btnLeft.isHold()) && !colling) { // УМЕНЬШЕНИЕ веремни/температуры
    if (menu == 0) {
      time -= 60;

      if (time < 60) {
        time = 60;
      }
    }
    if (menu == 1) {
      temp -= 10;
      if (temp < 10) {
        temp = 10;
      }
    }    
  }

  if ((menu == 1) && play) {
    if (currentMillis - previousMillis >= interval) { // 1 сек, работа в режиме ТЕМПЕРАТУРА
      previousMillis = currentMillis;
      
      if (sens.readTemp()) { // Читаем температуру с термопары

        readSensorCooling();

        if (sens.getTemp() >= temp) {
        Serial.print("Temp: "); // Если чтение прошло успешно - выводим в Serial
        Serial.print(sens.getTemp());
        Serial.println(" *C");
        offDevice();
        }
      }
    }
  }

  if ((menu == 0) && play) {
    if (currentMillis - previousMillis >= interval) { // 1 сек, работа в режиме ВРЕМЯ
      previousMillis = currentMillis;
      readSensorCooling();

      if (time >  0) {        
        MenuCheckTime(time);
        outPutTemp(temp);
        time = time - 60;
      } else {
        time = 0;
        MenuCheckTime(time);
        offDevice();
      }
    }
  }
  
  if (currentMillis - previousMillis >= interval) { // 1 сек измерение датчика охлаждения
      previousMillis = currentMillis;
      readSensorCooling();
    if (colling && !play) {
      if (dangerTime >  0) {        
        outPutTime(dangerTime);
        outPutTemp(temp);
        dangerTime = dangerTime - 60;
        offLeds(); // выключение всех светодиодов
        digitalWrite(red_pin, HIGH); // Включение КРАСНОГО светодиода
        ok = false;
      } else {
        offLeds(); // выключение всех светодиодов
        digitalWrite(blue_pin, HIGH); // Включение СИНЕГО светодиода
        colling = false;
        // Serial.println("time end");
      }
    }
  } 

  if (menu == 0 && !colling) {
    MenuCheckTime(time);
    outPutTemp(temp);
  }

  if (menu == 1 && !colling) {
    outPutTime(time);
    MenuCheckTemp(temp);
  } 

  if (menu == 2 && !colling) {
    outPutTime(time);
    outPutTemp(temp);
  }
}

void readSensorCooling() { // функция чтения датчика охлаждения  
  sensor.requestTemp();
  if (sensor.readTemp()) {
    if (sensor.getTemp() >=80) {
      play = false;
      colling = true;
      digitalWrite(12, HIGH); // Выключение реле
    } else {
      // colling = false;
    }
    Serial.println(sensor.getTemp());
  }
  else {
    // Serial.println("error");
  }
}

void buzOn() { // функция включения пищалки на один писк
  digitalWrite(13, HIGH); // Включение пищалки
  delay(100);
  digitalWrite(13, LOW); // Выключение пищалки
}

void buzOnThree() { // функция включения пищалки на один писк
  digitalWrite(13, HIGH); // Включение пищалки - 1
  delay(500);
  digitalWrite(13, LOW); // Выключение пищалки
  delay(500);
  digitalWrite(13, HIGH); // Включение пищалки - 2
  delay(500);
  digitalWrite(13, LOW); // Выключение пищалки
  delay(500);
  digitalWrite(13, HIGH); // Включение пищалки - 3
  delay(500);
  digitalWrite(13, LOW); // Выключение пищалки
}

void outPutTime(float time) {
  char buffer[16]; // буфер для хранения строки
  dtostrf((time / 60), 4, 1, buffer); // преобразование float в строку с двумя знаками после запятой
  sprintf(lcdLine, " Time: %ss", buffer);
  lcd.setCursor(0, 0);
  lcd.print(lcdLine);
} 

void outPutTemp(int temp) {
  sprintf(lcdLine, " Temp: %3d\1C", temp);
  lcd.setCursor(0, 1);
  lcd.print(lcdLine);
}

void MenuCheckTime(float time) {
  char buffer[16]; // буфер для хранения строки
  dtostrf((time / 60), 4, 1, buffer); // преобразование float в строку с двумя знаками после запятой
  sprintf(lcdLine, "-Time: %ss", buffer);
  lcd.setCursor(0, 0);
  lcd.print(lcdLine);
}

void MenuCheckTemp(int temp) {
  sprintf(lcdLine, "-Temp: %3d\1C", temp);
  lcd.setCursor(0, 1);
  lcd.print(lcdLine);
}

void setDefaultVlue() {
  temp = 500;
  time = 180;
}

void offDevice() { // выключение устройства
  digitalWrite(12, HIGH); // Выключение реле
  offLeds(); // выключение всех светодиодов
  digitalWrite(red_pin, HIGH); // Включение КРАСНОГО светодиода
  delay(3000); // задержка 3 секунды после выключения
  buzOnThree();

  play = false;
  ok = false;
  menu = 2;

  offLeds(); // Выключение ВСЕХ светодиодов
  digitalWrite(blue_pin, HIGH); // Включение СИНЕГО светодиода
  digitalWrite(13, LOW); // Выключение пищалки
  setDefaultVlue();
}

void offDeviceBlinkLed() { // выключение устройства
  digitalWrite(12, HIGH); // Выключение реле
  for (int i = 0; i <= 2; i++) {
    digitalWrite(red_pin, HIGH); // Включение КРАСНОГО светодиода
    delay(500);
    offLeds(); // выключение всех светодиодов
    delay(500);
  }

  play = false;
  ok = false;
  menu = 2;
 
  offLeds(); // выключение всех светодиодов
  delay(3000); // задержка 3 секунды после выключения
  digitalWrite(blue_pin, HIGH); // Включение СИНЕГО светодиода
  setDefaultVlue();
}

void resetDevice() {
  setDefaultVlue();
  digitalWrite(12, HIGH); // Выключение реле
  offLeds(); // выключение всех светодиодов
  digitalWrite(red_pin, HIGH); // Включение КРАСНОГО светодиода
  delay(3000);
  offLeds(); // выключение всех светодиодов
  digitalWrite(blue_pin, HIGH); // Включение СИНЕГО светодиода
  play = false;
  menu = 2;
}

void onDevice() {
  Serial.println("set");
  digitalWrite(12, LOW); // Включение реле
  offLeds(); // Выключение ВСЕХ светодиодов
  digitalWrite(green_pin, HIGH); // Включение ЗЕЛЕНОГО светодиода
}

// void readyDevice() {
//   offLeds(); // Выключение ВСЕХ светодиодов
//   digitalWrite(blue_pin, HIGH); // Включение СИНЕГО светодиода
//   digitalWrite(13, LOW); // Выключение пищалки
// }

void offLeds() {
  digitalWrite(red_pin, LOW); // Выключение КРАСНОГО светодиода
  digitalWrite(blue_pin, LOW); // Выключение СИНЕГО светодиода
  digitalWrite(green_pin, LOW); // Выключение ЗЕЛЕНОГО светодиода
}
