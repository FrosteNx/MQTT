import paho.mqtt.client as mqtt

BROKER_URL = "192.168.43.6"
BROKER_PORT = 1883
SENSOR_COMMAND_TOPIC = "/sensor/comm"
BH1750_TOPIC = "/+/bh1750/data"
YL69_TOPIC = "/+/yl69/data"
DTT11_TOPIC = "/+/dtt11/data"

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected successfully to MQTT broker.")
        client.subscribe(SENSOR_COMMAND_TOPIC)
        client.subscribe(BH1750_TOPIC)
        client.subscribe(YL69_TOPIC)
        client.subscribe(DTT11_TOPIC)
        print("Subscribed to all topics.")
        client.publish(SENSOR_COMMAND_TOPIC, "read1")
        print("Sent 'read1' command to ESP32.")
    else:
        print(f"Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    print(f"Received message on {msg.topic}: {msg.payload.decode()}")

    if "/bh1750/data" in msg.topic:
        print(f"BH1750 sensor data: {msg.payload.decode()}")
    elif "/yl69/data" in msg.topic:
        print(f"YL69 sensor data: {msg.payload.decode()}")
    elif "/dtt11/data" in msg.topic:
        print(f"DTT11 sensor data: {msg.payload.decode()}")

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    client.connect(BROKER_URL, BROKER_PORT, 60)
    client.loop_forever()

if __name__ == "__main__":
    main()
