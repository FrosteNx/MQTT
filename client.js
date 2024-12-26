const mqtt = require('mqtt');
const fs = require('fs');

const jsonData = JSON.parse(fs.readFileSync('user_data.json', 'utf-8'));
const BROKER_URL = 'mqtt://192.168.43.6:1883';
const commandTopic = `/sensor/command`;
const macAddresses = jsonData.devices;

const topics = macAddresses.map((mac) => [
    `${mac}/bh1750/light`,     // Natężenie światła
    `${mac}/ky037/analog`,    // Mikrofon analogowy
    `${mac}/ky037/digital`,   // Mikrofon cyfrowy
    `${mac}/ambilight/state`, // Ambilight
    `${mac}/rgb/values`       // RGB
]).flat();

const client = mqtt.connect(BROKER_URL);
const DATA_FILE = 'sensor_data.json';

function saveDataToFile(mac, sensor, value) {
    let sensorData = {};

    if (fs.existsSync(DATA_FILE)) {
        try {
            const fileContent = fs.readFileSync(DATA_FILE, 'utf-8');
            sensorData = fileContent ? JSON.parse(fileContent) : {}; 
        } catch (err) {
            console.error(`Błąd podczas wczytywania pliku ${DATA_FILE}:`, err.message);
            sensorData = {}; 
        }
    }

    const key = `${mac}/${sensor}`;
    
    const timestamp = new Intl.DateTimeFormat('pl-PL', {
        timeZone: 'Europe/Warsaw',
        year: 'numeric',
        month: '2-digit',
        day: '2-digit',
        hour: '2-digit',
        minute: '2-digit',
        second: '2-digit',
        hour12: false
    }).format(new Date());

    sensorData[key] = { value, timestamp };

    try {
        fs.writeFileSync(DATA_FILE, JSON.stringify(sensorData, null, 2));
        console.log(`Zapisano: ${key} -> ${value} (${timestamp})`);
    } catch (err) {
        console.error(`Błąd podczas zapisywania do pliku ${DATA_FILE}:`, err.message);
    }
}

client.on('connect', () => {
    console.log('Połączono z brokerem MQTT');

    client.subscribe(topics, (err) => {
        if (err) {
            console.error('Błąd podczas subskrypcji:', err);
        } else {
            console.log(`Subskrybowano tematy: ${topics.join(', ')}`);
            client.publish(commandTopic, 'readData'); 
        }
    });
});

client.on('message', (topic, message) => {
    try {
        const value = message.toString();
        const mac = topic.split('/')[0];
        const sensor = `${topic.split('/')[1]}/${topic.split('/')[2]}`;

        console.log(`Odebrano dane z ${topic}: MAC=${mac}, Sensor=${sensor}, Value=${value}`);
        saveDataToFile(mac, sensor, value);

        if (sensor === 'ambilight/state') {
            console.log(`Ambilight State for ${mac}: ${value === '1' ? 'ON' : 'OFF'}`);
        }

        if (sensor === 'rgb/values') {
            const [r, g, b] = value.split(',').map(Number);
            console.log(`RGB Values for ${mac}: Red=${r}, Green=${g}, Blue=${b}`);
        }
    } catch (error) {
        console.error('Błąd obsługi wiadomości:', error.message);
    }
});

client.on('error', (err) => {
    console.error('Błąd połączenia MQTT:', err);
});