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

int menu;

void setup() {
  Serial.begin(9600);

  pinMode(12, OUTPUT); // Реле
  pinMode(13, OUTPUT); // Пищалка
  
  digitalWrite(12, HIGH); // Выключение реле

  lcd.init(); // Инициализация lcd             
  lcd.backlight(); // Включаем подсветку
  lcd.createChar(1, degree); // Создаем символ под номером 

  // pinMode(red_pin, OUTPUT);
  // pinMode(green_pin, OUTPUT);
  //pinMode(blue_pin, OUTPUT);
  setDefaultVlue();
}

void loop() {
// digitalWrite(red_pin, HIGH); // Включение КРАСНОГО светодиода
// digitalWrite(green_pin, HIGH); // Включение ЗЕЛЕНОГО светодиода
// digitalWrite(blue_pin, HIGH); // Включение СИНЕГО светодиода

  btnReset.tick();
  btnSet.tick();
  btnOk.tick();
  btnRight.tick();
  btnLeft.tick();
  btnDown.tick();
  btnUp.tick();

  if (btnReset.isHolded()) {
    Serial.println("reset");
    digitalWrite(12, HIGH); // Выключение реле
    setDefaultVlue();
  }

  if (btnSet.isClick()) {
    Serial.println("set");
  }

  if (btnOk.isClick()) {
    Serial.println("ok");
    buzOn(); // включение пищалки на один писк
  }

  if (btnUp.isClick()) { // выбор меню ВРЕМЯ
    menu = 0;
    MenuCheckTime(time);   
  }

  if (btnDown.isClick()) { // выбор меню ТЕМПЕРАТУРА
    menu = 1;    
    MenuCheckTemp(temp);
  }

  if (btnRight.isClick() || btnRight.isHold()) { // УВЕЛИЧЕНИЕ веремни/температуры
    if (menu == 0) {
      time += 6;

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

  if (btnLeft.isClick() || btnLeft.isHold()) { // УМЕНЬШЕНИЕ веремни/температуры
    if (menu == 0) {
      time -= 6;

      if (time < 30) {
        time = 30;
      }
    }
    if (menu == 1) {
      temp -= 10;
      if (temp < 10) {
        temp = 10;
      }
    }    
  }

  // digitalWrite(12, HIGH); // Включение реле

  // if (sens.readTemp()) {            // Читаем температуру
  //   Serial.print("Temp: ");         // Если чтение прошло успешно - выводим в Serial
  //   Serial.print(sens.getTemp());   // Забираем температуру через getTemp
  //   //Serial.print(sens.getTempInt());   // или getTempInt - целые числа (без float)
  //   Serial.println(" *C");
  // } else Serial.println("Error");   // ошибка чтения или подключения - выводим лог
  // delay(1000);
  // запрос температуры

  sensor.requestTemp();
  
  // вместо delay используй таймер на millis(), пример async_read
  // delay(500);
  
  // проверяем успешность чтения и выводим
  // if (sensor.readTemp()) {
  //   Serial.println(sensor.getTemp());
  // }
  // else {
  //   Serial.println("error");
  // } 

  outPutTime(time);
  outPutTemp(temp);
}

void buzOn() { // функция включения пищалки на один писк
  digitalWrite(13, HIGH); // Включение пищалки
  delay(100);
  digitalWrite(13, LOW); // Выключение пищалки
}

void outPutTime(float time) {
  char buffer[16]; // буфер для хранения строки
  dtostrf((time / 60), 4, 1, buffer); // преобразование float в строку с двумя знаками после запятой
  sprintf(lcdLine, "Time: %ss", buffer);
  lcd.setCursor(1, 0);
  lcd.print(lcdLine);
} 

void outPutTemp(int temp) {
  sprintf(lcdLine, "Temp: %3d\1C", temp);
  lcd.setCursor(1, 1);
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
