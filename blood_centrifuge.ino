#include <Adafruit_SSD1306.h>
#include <arduino-timer.h>
#include <Wire.h>

#define ALTO 64
#define ANCHO 128
#define OLED_RESET 4

#define START_STOP_BUTTON 0
#define ACTION_BUTTON 1
#define BUZZER 2
#define SENSOR_HALL 3
#define MOTOR 10
#define ENABLE 12

const unsigned char MINIMUM_OPERATION_TIME = 5, MAXIMUM_OPERATION_TIME = 30, MINIMUM_ELAPSED_SECONDS_TO_START = 15;
const unsigned int MINIMUM_RPM = 2500, MAXIMUM_RPM = 2550;

volatile bool process_start = false, process_end = false, countdown_enabled = false, adjust_pwm = false;
volatile unsigned char s = 0, m = MINIMUM_OPERATION_TIME, turns_per_sencond = 0;
volatile unsigned int rpm = 0, total_turns = 0;

unsigned char menu_to_show = 0;
unsigned int pwm = 255, elapsed_seconds = 0;

char rpm_buffer[10];
char time_buffer[10];
char total_turns_buffer[10];

Adafruit_SSD1306 oled(ANCHO, ALTO, &Wire, OLED_RESET);
auto timer = timer_create_default();

void setup() {
  //  Serial.begin(9600);

  pinMode(START_STOP_BUTTON, INPUT_PULLUP);
  pinMode(ACTION_BUTTON, INPUT_PULLUP);
  pinMode(ENABLE, OUTPUT);
  pinMode(MOTOR, OUTPUT);
  pinMode(BUZZER, OUTPUT);

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
    adjust_pwm = false;
    countdown_enabled = false;
    pwm = 255;
    s = 0;
    elapsed_seconds = 0;
    process_end = false;
    if (!digitalRead(START_STOP_BUTTON)) {
      delay(250);
      process_start = true;
    }
    if (!digitalRead(ACTION_BUTTON)) {
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
    if (elapsed_seconds < MINIMUM_ELAPSED_SECONDS_TO_START) {
      digitalWrite(MOTOR, HIGH);
    }
    else if (!countdown_enabled && rpm > MINIMUM_RPM && rpm < MAXIMUM_RPM) {
      countdown_enabled = true;
    }
    else if (adjust_pwm && (rpm < MINIMUM_RPM || rpm > MAXIMUM_RPM)) {
      if (!countdown_enabled) {
        if (rpm < MINIMUM_RPM) {
          pwm += 5;
        } else if (rpm > MAXIMUM_RPM) {
          pwm -= 5;
        }
      } else {
        if (rpm < MINIMUM_RPM) {
          pwm += 1;
        } else if (rpm > MAXIMUM_RPM) {
          pwm -= 1;
        }
      }
      if (pwm > 255) {
        pwm = 250;
      }
      else if (pwm < 100) {
        pwm = 150;
      }
      adjust_pwm = false;
    }
    analogWrite(MOTOR, pwm);
    if (!digitalRead(START_STOP_BUTTON)) {
      delay(250);
      process_start = false;
      m = 5;
      digitalWrite(ENABLE, LOW);
      digitalWrite(MOTOR, LOW);
    }
    if (!digitalRead(ACTION_BUTTON)) {
      delay(250);
      menu_to_show++;
      if (menu_to_show > 2) {
        menu_to_show = 0;
      }
    }
  }
  if (process_end) {
    digitalWrite(ENABLE, LOW);
    digitalWrite(MOTOR, LOW);
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setTextSize(3);
    oled.setCursor(0, 20);
    oled.print("Cuidado");
    oled.display();
    digitalWrite(BUZZER, HIGH);
    delay(3000);
    digitalWrite(BUZZER, LOW);
    process_end = false;
  }
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(2);
  oled.setCursor(0, 0);
  sprintf(rpm_buffer, "RPM: %0.4d", rpm);
  oled.print(rpm_buffer);
  oled.setCursor(0, 25);
  sprintf(time_buffer, "T: %02d:%02d", m, s);
  oled.print(time_buffer);
  oled.setCursor(0, 50);
  sprintf(total_turns_buffer, "Vt: %0.6d", total_turns);
  oled.print(total_turns_buffer);
  oled.display();
}

bool countdown() {
  if (process_start) {
    rpm = turns_per_sencond * 60;
    total_turns = total_turns + turns_per_sencond;
    turns_per_sencond = 0;
    elapsed_seconds++;
    adjust_pwm = true;
    if (countdown_enabled) {
      s--;
      if (s > 59) {
        s = 0;
      }
    }
  }
  if (s == 0 && countdown_enabled) {
    if (m == 0) {
      m = MINIMUM_OPERATION_TIME;
      process_start = false;
      process_end = true;
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
