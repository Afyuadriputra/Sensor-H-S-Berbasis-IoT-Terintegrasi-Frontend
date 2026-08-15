import mqtt from "mqtt";

export const MQTT_TOPIC =
  "afyuadri/h2s-demo/a7c91f/device-001/telemetry";

export const mqttClient = mqtt.connect(
  "ws://broker.hivemq.com:8000/mqtt",
  {
    clientId:
      `h2s-dashboard-${Math.random()
        .toString(16)
        .slice(2, 10)}`,

    clean: true,

    reconnectPeriod: 2000,

    connectTimeout: 10000,

    keepalive: 30,
  }
);