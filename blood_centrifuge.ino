#include <Adafruit_SSD1306.h>
#include <arduino-timer.h>
#include <Wire.h>

#define ALTO 64
#define ANCHO 128
#define OLED_RESET 4

#define SENSOR_HALL 2
#define ENABLE 12
#define MOTOR 10

Adafruit_SSD1306 oled(ANCHO, ALTO, &Wire, OLED_RESET);

const unsigned char MINIMUM_OPERATION_TIME = 5, MAXIMUM_OPERATION_TIME = 30;
const unsigned int MINIMUM_RPM = 1300, MAXIMUM_RPM = 1400;

volatile bool process_start = false, countdown_enabled = false, ajustar_pwm = false;
volatile unsigned char s = 0, m = MINIMUM_OPERATION_TIME, menu_to_show = 0;
volatile unsigned int rpm = 0, turns_per_sencond = 0, total_turns = 0, pwm = 255;
char countdown_value[40];

auto timer = timer_create_default();

void setup() {
  Serial.begin(9600);

  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(ENABLE, OUTPUT);
  pinMode(MOTOR, OUTPUT);

  timer.every(1000, countdown);

  Wire.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  attachInterrupt(digitalPinToInterrupt(SENSOR_HALL), turns_ISR, FALLING);
}

void loop() {
  if (!process_start) {
    digitalWrite(ENABLE, LOW);
    digitalWrite(MOTOR, LOW);
    menu_to_show = 0;
    rpm = 0;
    turns_per_sencond = 0;
    total_turns = 0;
    ajustar_pwm = false;
    countdown_enabled = false;
    pwm = 255;
    s = 0;
    if (!digitalRead(3)) {
      delay(250);
      process_start = true;
      s = 1;
    }
    if (!digitalRead(4)) {
      delay(250);
      m += 5;
      if (m > MAXIMUM_OPERATION_TIME) {
        m = MINIMUM_OPERATION_TIME;
      }
    }
  }
  else {
    timer.tick();
    digitalWrite(ENABLE, HIGH);
    if (!countdown_enabled && rpm > MINIMUM_RPM && rpm < MAXIMUM_RPM) {
      countdown_enabled = true;
    }
    else if (ajustar_pwm) {
      pwm -= 5;
      if(pwm < 100) {
        pwm = 255;
      }
      analogWrite(MOTOR, pwm);
      ajustar_pwm = false;
    }
    if (!digitalRead(3)) {
      delay(250);
      process_start = false;
      m = 5;
      digitalWrite(ENABLE, LOW);
      digitalWrite(MOTOR, LOW);
    }
    if (!digitalRead(4)) {
      delay(250);
      menu_to_show++;
      if (menu_to_show > 2) {
        menu_to_show = 0;
      }
    }
  }

  print_menu();
}

bool countdown() {
  if (process_start) {
    rpm = turns_per_sencond * 60;
    total_turns = total_turns + turns_per_sencond;
    turns_per_sencond = 0;
    if (countdown_enabled) {
      s--;
    }
    else {
      ajustar_pwm = true;
    }
  }
  if (s == 0 && countdown_enabled) {
    if (m == 0) {
      m = MINIMUM_OPERATION_TIME;
      process_start = false;
    }
    if (process_start) {
      m--;
      s = 59;
    }
  }
  return true;
}

void turns_ISR() {
  turns_per_sencond++;
}

void print_menu() {
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  if (menu_to_show == 0) {
    sprintf(countdown_value, "%02d:%02d", m, s);
    oled.print("Tiempo: ");
    oled.setCursor(0, 20);
    oled.setTextSize(4);
    oled.print(countdown_value);
  } else if (menu_to_show == 1) {
    oled.print("RPM: ");
    oled.setCursor(0, 20);
    oled.setTextSize(4);
    oled.print(rpm);
  } else {
    oled.print("Vueltas: ");
    oled.setCursor(0, 20);
    oled.setTextSize(4);
    oled.print(total_turns);
  }
  oled.display();
}
