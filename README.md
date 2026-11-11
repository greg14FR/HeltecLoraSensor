# HeltecLoraSensor

Nœud LoRaWAN pour **Heltec Wireless Paper** (ESP32-S3 + SX1262) connecté à une passerelle **SenseCAP M2** (EU868) en mode cloud SenseCAP par défaut.

Projet **PlatformIO / Arduino** (VS Code) basé sur la librairie **MCCI LoRaWAN LMIC** (pas de licence requise, contrairement à la librairie LoRaWAN officielle Heltec).

## Fonctionnement

- Join **OTAA** en **EU868**, envoi toutes les 60 s sur le **port 8**.
- Payload **6 octets**, identique au nœud XIAO + Grove Wio-E5 du kit SenseCAP :
  `température×100 (2 octets, big-endian) | humidité×100 (2 octets) | moisture (2 octets)`.
  Le même décodeur JavaScript côté SenseCAP Portal fonctionne donc sans modification.
- Données **simulées** pour l'instant (sinusoïde + aléatoire) ; remplacer par de vrais capteurs dans `do_send()`.
- Affichage des valeurs sur l'écran **E-Ink** (250×122) à chaque envoi.

## Brochage SX1262 (Wireless Paper)

| Signal | GPIO |
|---|---|
| NSS (CS) | 8 |
| SCK | 9 |
| MOSI | 10 |
| MISO | 11 |
| RST | 12 |
| BUSY | 13 |
| DIO1 | 14 |
| Vext (alim écran, actif à LOW) | 45 |

## Mise en route

1. Sur le **SenseCAP Portal**, ajouter l'appareil manuellement (DevEUI / JoinEUI / AppKey, profil EU868) et lui attacher le décodeur JavaScript du kit.
2. Reporter ces clés dans `src/main.cpp` (`APPEUI`, `DEVEUI`, `APPKEY`).
3. Dans VS Code + PlatformIO : sélectionner la carte `heltec_wireless_paper`, build, upload.

> **Prérequis PlatformIO** : la carte `heltec_wireless_paper` nécessite `platform = espressif32@^8.0.0` (Arduino core 3.x). Avec une plateforme 6.x/7.x, utiliser `board = esp32-s3-devkitc-1` à la place — la pinmap du SX1262 est définie explicitement dans `src/main.cpp`, donc la variante de carte n'a pas d'impact.

> Wireless Paper **V1.2** : utiliser `EInkDisplay_WirelessPaperV1_2` dans `src/main.cpp` (déjà en place). Le brochage SX1262 est identique sur toutes les versions (NSS=8, RST=12, BUSY=13, DIO1=14, Vext=45).
