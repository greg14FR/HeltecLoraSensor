// Variante WiFi LoRa 32 V4 (ESP32-S3 + SX1262) - sans ecran E-Ink, avec capteurs accessibles.
// Meme stack LoRaWAN (MCCI LMIC, EU868, OTAA) que la variante Wireless Paper.
// Brochage SX1262 identique : NSS=8, SCK=9, MOSI=10, MISO=11, RST=12, BUSY=13, DIO1=14.
#include <Arduino.h>
#include <SPI.h>
#include <lmic.h>
#include <hal/hal.h>

// ===== Cles OTAA : copier src/secrets.h ici (src_v4/secrets.h, ignore par git) =====
#if __has_include("secrets.h")
#include "secrets.h"
#else
static const u1_t PROGMEM APPEUI[8]  = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const u1_t PROGMEM DEVEUI[8]  = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static const u1_t PROGMEM APPKEY[16] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                         0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
#endif

void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8); }
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8); }
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16); }

// ===== Pinmap SX1262 (identique V3/V4/Wireless Paper) =====
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

static const unsigned TX_INTERVAL = 60;
static unsigned long sendCount = 0;
static osjob_t sendjob;

// ===== Capteurs (V4 : broches libres sur la barrette) =====
#define PIN_SOIL A0      // GPIO1 : sonde humidite sol (analogique)
// Exemple I2C (DHT20/SHT40) sur SDA=GPIO4 / SCL=GPIO3 si besoin.

void do_send(osjob_t* j);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println(F("WiFi LoRa 32 V4 - LoRaWAN boot"));

    // Vext du V4 : GPIO36 (version R2) ou GPIO40 (version R8) -> LOW pour alimenter le PA/RF
    pinMode(36, OUTPUT); digitalWrite(36, LOW);
    pinMode(40, OUTPUT); digitalWrite(40, LOW);

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
        sendCount++;
        float t = temperatureRead();        // capteur interne ESP32-S3 (vrai mesure)
        float h = 50.0;                     // TODO: remplacer par capteur I2C (DHT20/SHT40)
        int   m = analogRead(PIN_SOIL);     // sonde sol sur A0

        // Payload 6 octets identique au kit Wio-E5 (port 8)
        uint8_t payload[6];
        uint16_t tv = (uint16_t)(t * 100);
        uint16_t hv = (uint16_t)(h * 100);
        payload[0] = tv >> 8;   payload[1] = tv & 0xFF;
        payload[2] = hv >> 8;   payload[3] = hv & 0xFF;
        payload[4] = m >> 8;    payload[5] = m & 0xFF;
        LMIC_setTxData2(8, payload, sizeof(payload), 0);

        Serial.print(F("Envoi #")); Serial.print(sendCount);
        Serial.print(F("  temp=")); Serial.print(t, 2);
        Serial.print(F("  humi=")); Serial.print(h, 2);
        Serial.print(F("  moisture=")); Serial.println(m);
    }
    os_setTimedCallback(j, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
}

void onEvent (ev_t ev) {
    switch (ev) {
    case EV_JOINED:
        Serial.println(F("JOIN OK"));
        LMIC_setLinkCheckMode(0);
        do_send(&sendjob);
        break;
    case EV_TXCOMPLETE:
        Serial.println(F("TX OK"));
        break;
    case EV_JOIN_FAILED:
        Serial.println(F("JOIN FAILED - verifier cles/EUI et Vext"));
        break;
    default:
        break;
    }
}
