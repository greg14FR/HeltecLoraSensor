#include <Arduino.h>
#include <SPI.h>
#include <lmic.h>
#include <hal/hal.h>
#include <heltec-eink-modules.h>
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold18pt7b.h"

// ===== A REMPLIR avec les cles du SenseCAP Portal (OTAA) =====
static const u1_t PROGMEM APPEUI[8]  = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; // JoinEUI
static const u1_t PROGMEM DEVEUI[8]  = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; // DevEUI
static const u1_t PROGMEM APPKEY[16] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }; // AppKey

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

// ===== Pinmap SX1262 du Heltec Wireless Paper =====
// NSS=8, SCK=9, MOSI=10, MISO=11, RST=12, BUSY=13, DIO1=14
class cHalConfig : public Arduino_LMIC::HalConfiguration_t {
public:
    virtual bool queryUsingDcdc(void) override { return true; }
    virtual bool queryUsingDIO2AsRfSwitch(void) override { return true; }
    virtual bool queryUsingDIO3AsTCXOSwitch(void) override { return true; }
    virtual uint8_t queryBusyPin(void) override { return 13; }
};
static cHalConfig myConfig;

const lmic_pinmap lmic_pins = {
    .nss = 8, .rxtx = LMIC_UNUSED_PIN, .rst = 12,
    .dio = { 14, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN },
    .rxtx_rx_active = 0, .rssi_cal = 10,
    .spi_freq = 8000000, .pConfig = &myConfig,
};

// ===== Ecran E-Ink (Wireless Paper : 250x122) =====
// V1.0   -> EInkDisplay_WirelessPaperV1
// V1.1.1 / V1.2 -> EInkDisplay_WirelessPaperV1_2
EInkDisplay_WirelessPaperV1_2 display;

static const unsigned TX_INTERVAL = 60; // secondes entre deux envois
static unsigned long sendCount = 0;

void do_send(osjob_t* j);

void showStatus(const char* line1, const char* line2) {
    display.clearMemory();
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(10, 30);
    display.println(line1);
    display.setFont(&FreeSans9pt7b);
    display.setCursor(10, 55);
    display.println(line2);
    display.update();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(45, OUTPUT);          // Vext ON (actif a LOW) : alim ecran E-Ink
    digitalWrite(45, LOW);
    delay(100);

    display.landscape();
    showStatus("SenseCAP M2", "Join LoRaWAN...");

    os_init();
    LMIC_reset();
    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
    LMIC_setLinkCheckMode(0);
    LMIC_startJoining();
}

void loop() {
    os_runloop_once();
}

void do_send(osjob_t* j) {
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("TX en cours, report..."));
    } else {
        // ===== Donnees SIMULEES (a remplacer par de vrais capteurs) =====
        sendCount++;
        float t = 22.0 + 5.0 * sin(sendCount / 5.0);         // temperature ~17-27 C
        float h = 50.0 + 10.0 * sin(sendCount / 3.0 + 1.0);  // humidite ~40-60 %
        int   m = random(0, 1024);                           // "moisture" 0-1023

        // ===== Payload 6 octets, identique au kit Wio-E5 =====
        // octets 0-1 : temperature x100 | 2-3 : humidite x100 | 4-5 : moisture
        uint8_t payload[6];
        uint16_t tv = (uint16_t)(t * 100);
        uint16_t hv = (uint16_t)(h * 100);
        payload[0] = tv >> 8;   payload[1] = tv & 0xFF;
        payload[2] = hv >> 8;   payload[3] = hv & 0xFF;
        payload[4] = m >> 8;    payload[5] = m & 0xFF;
        LMIC_setTxData2(8, payload, sizeof(payload), 0); // port 8 comme le kit

        Serial.print(F("Envoi #")); Serial.print(sendCount);
        Serial.print(F("  temp=")); Serial.print(t, 2);
        Serial.print(F("  humi=")); Serial.print(h, 2);
        Serial.print(F("  moisture=")); Serial.println(m);

        // ===== Mise a jour de l'ecran E-Ink =====
        char l1[32], l2[48], l3[48], l4[48];
        snprintf(l1, sizeof(l1), "Envoi #%lu", sendCount);
        snprintf(l2, sizeof(l2), "Temp  : %.1f C", t);
        snprintf(l3, sizeof(l3), "Humi  : %.1f %%", h);
        snprintf(l4, sizeof(l4), "Moist : %d", m);
        display.clearMemory();
        display.setFont(&FreeSansBold18pt7b);
        display.setCursor(10, 28);
        display.println(l1);
        display.setFont(&FreeSans9pt7b);
        display.setCursor(10, 52);  display.println(l2);
        display.setCursor(10, 70);  display.println(l3);
        display.setCursor(10, 88);  display.println(l4);
        display.update();
    }
    os_setTimedCallback(j, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
}

void onEvent (ev_t ev) {
    switch (ev) {
    case EV_JOINED:
        Serial.println(F("JOIN OK"));
        showStatus("JOIN OK", "Pret a envoyer");
        LMIC_setLinkCheckMode(0);
        do_send(os_getCallback());
        break;
    case EV_TXCOMPLETE:
        Serial.println(F("TX OK"));
        break;
    case EV_JOIN_FAILED:
        Serial.println(F("JOIN FAILED"));
        showStatus("JOIN FAILED", "verifier cles/EUI");
        break;
    default:
        break;
    }
}
