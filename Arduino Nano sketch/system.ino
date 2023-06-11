#include <SoftwareSerial.h>
#include <TinyGPS.h>


// Оголошення пінів для підключення компонентів
#define BUTTON 2
#define Vibramotor 7
#define Led 8
#define WaterBuzzer 9
#define UltrasonicBuzzer 10
#define EchoPin 11
#define TrigPin 12
#define APin A1

int moisture = 0; // Змінна для збереження значення вологості
long distance = 0; // Змінна для збереження значення відстані

// GPS&GSM
int state = 0;
const int pin = 9;
float gpslat, gpslon;
TinyGPS gps;
SoftwareSerial sgps(6, 5);
SoftwareSerial sgsm(3, 4);

void setup() {
  sgsm.begin(9600);
  sgps.begin(9600);
  Serial.begin(9600); // Ініціалізація послідовного порту для виводу відладкової інформації
  pinMode(Led, OUTPUT); // Встановлюємо пін для світлодіода у режим виводу
  pinMode(UltrasonicBuzzer, OUTPUT); // Встановлюємо пін для динаміка UltrasonicBuzzer у режим виводу
  pinMode(WaterBuzzer, OUTPUT); // Встановлюємо пін для динаміка WaterBuzzer у режим виводу
  pinMode(EchoPin, INPUT); // Встановлюємо пін для ехо-сигналу ультразвукового датчика у режим вводу
  pinMode(TrigPin, OUTPUT); // Встановлюємо пін для тригера ультразвукового датчика у режим виводу
  pinMode(Vibramotor, OUTPUT); // Встановлюємо пін для вібромотора у режим виводу
}

// Функція для вимірювання відстані за допомогою ультразвукового датчика
long takeDistance() {
  long duration;
  digitalWrite(TrigPin, LOW); // Встановлення TrigPin у низький рівень
  delay(2);
  digitalWrite(TrigPin, HIGH); // Встановлення TrigPin у високий рівень на 10 мікросекунд
  delay(10);
  digitalWrite(TrigPin, LOW); // Повертаємо пін тригера в низький рівень
  duration = pulseIn(EchoPin, HIGH); // Вимірюємо тривалість ехо-сигналу
  distance = (duration / 2) / 29.1; // Розраховуємо відстань в сантиметрах
  return distance; // Повертаємо виміряну відстань
}

// Функція для виявлення перешкод на основі виміряної відстані
void obstacleDetection(long distance) {
  if (distance <= 40) { // Якщо відстань до перешкоди менше або дорівнює 40 см
    digitalWrite(Led, HIGH); // Увімкнути світлодіод
    digitalWrite(Vibramotor, HIGH); // Увімкнути вібромотор
    if (distance <= 20) { // Якщо відстань менша або дорівнює 20 см
      if (distance <= 10) { // Якщо відстань менша або дорівнює 10 см
        digitalWrite(UltrasonicBuzzer, HIGH); // Увімкнути динамік
      } else {
        digitalWrite(UltrasonicBuzzer, LOW); // Вимкнути динамік
        delay(1000);
        digitalWrite(UltrasonicBuzzer, HIGH); // Увімкнути динамік
        delay(1000);
      }
    } else {}
    // Якщо відстань до перешкоди більше 40, вимкнути всі компоненти
  } else {
    digitalWrite(Led, LOW); // Вимкнути світлодіод
    digitalWrite(UltrasonicBuzzer, LOW); // Вимкнути динамік
    digitalWrite(Vibramotor, LOW); // Вимкнути вібромотор
  }
}

// Функція для виявлення вологості на основі аналогового сигналу
void moistureDetection(int moisture) {
  if (moisture > 5) { // Якщо значення вологості більше 5
    digitalWrite(WaterBuzzer, HIGH); // Увімкнути динамік про виявлення вологи
  } else {
    digitalWrite(WaterBuzzer, LOW); // Вимкнути динамік про виявлення вологи
  }
}

void sendingGPS() {
  sgps.listen();
  while (sgps.available())
  {
    int c = sgps.read();
    if (gps.encode(c))
    {
      gps.f_get_position(&gpslat, &gpslon);
    }
  }
  if (digitalRead(pin) == HIGH && state == 0) {
      sgsm.listen();
      sgsm.print("\r");
      delay(1000);
      sgsm.print("AT+CMGF=1\r");
      delay(1000);
      /*Replace XXXXXXXXXX to 10 digit mobile number &
        ZZ to 2 digit country code*/
      sgsm.print("AT+CMGS=\"+ZZXXXXXXXXXX\"\r");
      delay(1000);
      //The text of the message to be sent.
      sgsm.print("https://www.google.com/maps/?q=");
      sgsm.print(gpslat, 6);
      sgsm.print(",");
      sgsm.print(gpslon, 6);
      delay(1000);
      sgsm.write(0x1A);
      delay(1000);
      state = 1;
    }
  if (digitalRead(pin) == LOW) {
      state = 0;
    }
      delay(100);
}

void loop() {
  // Якщо кнопка натиснута
  if (digitalRead (BUTTON) == LOW)
  {
    // Відправляємо повідомлення з місцезнаходженням
    sendingGPS();
  }
  else {}
  distance = takeDistance(); // Вимірюємо відстань
  obstacleDetection(distance); // Виконуємо виявлення перешкод на основі відстані
  
  moisture = analogRead(APin); // Зчитуємо аналоговий сигнал вологості
  moistureDetection(moisture); // Виконуємо виявлення вологості на основі зчитаного значення
}
