# Restauration du nœud d'origine du kit (XIAO ESP32S3 + Grove Wio-E5)

Projet séparé à ouvrir **dans sa propre fenêtre VS Code** (dossier `kit_xiao_wio_e5/`),
car la racine du dépôt est le projet Heltec Wireless Paper.

## Pourquoi

Si la XIAO ESP32S3 du kit a été flashée par erreur avec le firmware du Heltec
Wireless Paper, ce sketch restaure le fonctionnement d'origine : la XIAO pilote
à nouveau le module Wio-E5 en commandes AT (JOIN OTAA + envoi toutes les 30 s).

Les clés OTAA du Wio-E5 vivent **dans le module Wio-E5 lui-même** (binding QR
SenseCAP Mate) : elles n'ont pas été perdues, aucune reconfiguration de clés n'est
nécessaire ici.

## Utilisation

1. Ouvrir le dossier `kit_xiao_wio_e5` dans VS Code (File > Open Folder).
2. Sélectionner la carte **Seeed XIAO ESP32S3** (env `seeed_xiao_esp32s3`).
3. Build puis Upload sur la XIAO du kit (USB-C).
4. Moniteur série 115200 bauds : attendre `JOIN OK` puis `Send OK`.
5. Les données réapparaissent dans le SenseCAP Portal / SenseCAP Mate.

## Capteurs

Données simulées par défaut. Pour remettre les vrais capteurs du kit :
température/humidité (DHT) dans `t`/`h`, et `analogRead(sensorPin)` (A1) pour
l'humidité de sol — le payload reste le même format 6 octets, port 8.
