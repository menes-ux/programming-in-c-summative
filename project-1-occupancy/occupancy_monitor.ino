#include <Adafruit_LiquidCrystal.h>
#include <stdlib.h>

Adafruit_LiquidCrystal lcd(0);

const byte greenLight = A2;
const byte redLight = A3;
const byte capacity = 5;

unsigned long screenTime = 0;

struct Occupancy {
  byte inside;
  unsigned long entries;
  unsigned long exits;

  void report(const char *message) const {
    Serial.print(message);
    Serial.print(" | Inside: ");
    Serial.print(inside);
    Serial.print(" | Available: ");
    Serial.print(capacity - inside);
    Serial.print(" | Entries: ");
    Serial.print(entries);
    Serial.print(" | Exits: ");
    Serial.println(exits);
  }

  void enter() {
    if (inside >= capacity) {
      report("Entry rejected: full");
      return;
    }

    ++inside;
    ++entries;
    lights();
    display();
    report("Entry accepted");
  }

  void leave() {
    if (inside == 0) {
      report("Exit rejected: empty");
      return;
    }

    --inside;
    ++exits;
    lights();
    display();
    report("Exit accepted");
  }

  void lights() const {
    digitalWrite(greenLight, inside < capacity ? HIGH : LOW);
    digitalWrite(redLight, inside == capacity ? HIGH : LOW);
  }

  void display() const {
    lcd.setCursor(8, 0);
    lcd.print(inside);
    lcd.print("  ");
    lcd.setCursor(11, 1);
    lcd.print(capacity - inside);
    lcd.print("  ");
    screenTime = millis();
  }
};

struct Button {
  byte pin;
  byte reading;
  byte state;
  unsigned long changedAt;

  bool pressed(unsigned long now) {
    byte input = digitalRead(pin);

    if (input != reading) {
      reading = input;
      changedAt = now;
    }

    if (reading == state || now - changedAt < 20) {
      return false;
    }

    state = reading;
    return state == LOW;
  }
};

Button entryButton = {A0, HIGH, HIGH, 0};
Button exitButton = {A1, HIGH, HIGH, 0};
Occupancy *room = NULL;

void setup() {
  Serial.begin(9600);
  pinMode(entryButton.pin, INPUT_PULLUP);
  pinMode(exitButton.pin, INPUT_PULLUP);
  pinMode(greenLight, OUTPUT);
  pinMode(redLight, OUTPUT);

  lcd.begin(16, 2);
  lcd.setBacklight(1);

  room = (Occupancy *)calloc(1, sizeof(Occupancy));

  if (room == NULL) {
    digitalWrite(redLight, HIGH);
    lcd.print("Memory error");
    Serial.println("Error: occupancy allocation failed");
    return;
  }

  lcd.setCursor(0, 0);
  lcd.print("Inside: ");
  lcd.setCursor(0, 1);
  lcd.print("Available: ");

  room->lights();
  room->display();
  room->report("Ready");
}

void loop() {
  if (room == NULL) {
    return;
  }

  unsigned long now = millis();
  bool leaving = exitButton.pressed(now);
  bool entering = entryButton.pressed(now);

  if (leaving) {
    room->leave();
  }

  if (entering) {
    room->enter();
  }

  if (millis() - screenTime >= 1000) {
    room->display();
  }
}
