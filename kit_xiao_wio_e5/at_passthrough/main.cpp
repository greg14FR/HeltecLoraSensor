// Pont serie temporaire : USB (Serial) <-> Wio-E5 (Serial1, broches 44/43)
// Permet de taper des commandes AT a la main dans le moniteur serie.
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, 44, 43); // Wio-E5 : RX=44, TX=43
    Serial.println("AT passthrough pret. Tape par ex. : AT+ID");
}

void loop() {
    while (Serial.available())  Serial1.write(Serial.read());
    while (Serial1.available()) Serial.write(Serial1.read());
}
