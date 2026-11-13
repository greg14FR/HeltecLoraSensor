// Pont TTN (MQTT TCP 8883) -> broker WebSocket local (pour la page HTML)
// Usage :
//   npm install mqtt aedes
//   node ttn-ws-bridge.js <APP_ID> <API_KEY> [DEV_EUI]
// Puis ouvrir tools/mqtt-dashboard.html et mettre ws://localhost:9001 comme broker.
const mqtt = require('mqtt');
const aedes = require('aedes')();
const http = require('http');
const { WebSocketServer } = require('ws');

const APP = process.argv[2];        // ex. sensecap-m2-greg
const KEY = process.argv[3];        // cle API TTN (View traffic)
const DEV = process.argv[4] || '+'; // DevEUI ou '+' pour tous
if (!APP || !KEY) {
  console.error('Usage: node ttn-ws-bridge.js <APP_ID> <API_KEY> [DEV_EUI]');
  process.exit(1);
}

// Broker WebSocket local pour le navigateur
const server = http.createServer();
const wss = new WebSocketServer({ server });
aedes.on('client', (c) => console.log('[ws] client navigateur connecte:', c.id));
server.listen(9001, () => console.log('[ws] broker WebSocket sur ws://localhost:9001'));
// aedes via websocket: utiliser aedes.handle(ws) - simplifie avec le module ws
wss.on('connection', (ws) => aedes.handle(ws));

// Connexion TTN
const ttnUrl = 'mqtts://eu1.cloud.thethings.network:8883';
const topic = `v3/${APP}@ttn/devices/${DEV}/up`;
console.log('[ttn] connexion a', ttnUrl, 'topic', topic);
const ttn = mqtt.connect(ttnUrl, { username: `${APP}@ttn`, password: KEY });

ttn.on('connect', () => {
  console.log('[ttn] connecte. Abonnement a', topic);
  ttn.subscribe(topic);
});
ttn.on('message', (t, payload) => {
  console.log('[ttn] uplink recu -> republication locale');
  aedes.publish({ topic: 'lorawan/up', payload }, () => {});
});
ttn.on('error', (e) => console.error('[ttn] erreur:', e.message));
