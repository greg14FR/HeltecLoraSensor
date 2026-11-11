// Noeud d'origine du kit SenseCAP : XIAO ESP32S3 pilote le module Grove Wio-E5 (STM32WLE5)
// en commandes AT. Les cles OTAA du Wio-E5 sont deja enregistrees dans le module
// (binding QR fait via l'app SenseCAP Mate) : ici on ne fait que JOIN + envoi.
#include <Arduino.h>

static char recv_buf[512];
static bool is_exist = false;
static bool is_join = false;

int sensorPin = A1; // capteur analogique (ex. humidite de sol Grove)

static int at_send_check_response(const char *expected_ack, int timeout_ms, const char *command) {
    int character, index = 0;
    unsigned long startMillis;
    memset(recv_buf, 0, sizeof(recv_buf));
    Serial1.print(command);
    Serial.print(command);
    delay(200);
    startMillis = millis();
    if (expected_ack == NULL) return 0;
    do {
        while (Serial1.available() > 0) {
            character = Serial1.read();
            if (index < (int)sizeof(recv_buf) - 1) recv_buf[index++] = character;
            Serial.print((char)character);
            delay(2);
        }
        if (strstr(recv_buf, expected_ack) != NULL) return 1;
    } while (millis() - startMillis < (unsigned long)timeout_ms);
    return 0;
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, 44, 43); // Wio-E5 sur RX=44 / TX=43 de la XIAO

    if (at_send_check_response("+AT: OK", 1000, "AT\r\n")) {
        is_exist = true;
        // Le kit est deja bind via QR : on ne reconfigure PAS les cles ici.
        // (Si besoin : AT+KEY=APPKEY,"<32 hex>" et AT+ID=DevEui/AppEui,"...")
        at_send_check_response("+MODE: LWOTAA", 1000, "AT+MODE=LWOTAA\r\n");
        at_send_check_response("+DR: EU868", 1000, "AT+DR=EU868\r\n");
        at_send_check_response("+CH: NUM", 1000, "AT+CH=NUM,0-2\r\n");
        at_send_check_response("+CLASS: A", 1000, "AT+CLASS=A\r\n");
        at_send_check_response("+PORT: 8", 1000, "AT+PORT=8\r\n");
        delay(200);
        is_join = true;
    } else {
        Serial.println("No E5 module found.");
    }
}

void loop() {
    if (!is_exist) { delay(1000); return; }
    if (is_join) {
        if (at_send_check_response("+JOIN: Network joined", 12000, "AT+JOIN\r\n")) {
            is_join = false;
            Serial.println("JOIN OK");
        } else {
            Serial.println("JOIN failed!");
            delay(5000);
        }
    } else {
        // ===== Donnees SIMULEES (remplacer par DHT11 + capteur sol du kit) =====
        // Payload 6 octets identique au demo du kit : temp x100 | humi x100 | moisture
        float t = 22.0, h = 50.0;
        int m = analogRead(sensorPin);
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "AT+CMSGHEX=\"%04X%04X%04X\"\r\n",
                 (unsigned)(h * 100), (unsigned)(t * 100), m);
        if (at_send_check_response("Done", 5000, cmd)) {
            Serial.println("Send OK");
        } else {
            Serial.println("Send failed!");
        }
        delay(30000);
    }
}
