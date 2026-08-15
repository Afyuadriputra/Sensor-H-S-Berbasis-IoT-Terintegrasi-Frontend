import {
  useEffect,
  useState,
} from "react";

import {
  mqttClient,
  MQTT_TOPIC,
} from "../lib/mqtt";


// =============================================
// INITIAL DATA
// =============================================

const initialData = {
  deviceId: "H2S-TPA-001",

  ppm: 0,

  adc: 0,

  filteredAdc: 0,

  level: 0,

  status: "WAITING",

  effect:
    "Menunggu data dari ESP32...",

  uptimeMs: 0,

  updatedAt: null,
};


// =============================================
// HOOK
// =============================================

export function useH2S() {

  const [
    connected,
    setConnected,
  ] = useState(false);


  const [
    data,
    setData,
  ] = useState(initialData);


  const [
    history,
    setHistory,
  ] = useState([]);


  useEffect(() => {

    // =========================================
    // SUBSCRIBE
    // =========================================

    const subscribe = () => {

      mqttClient.subscribe(
        MQTT_TOPIC,
        {
          qos: 0,
        },
        (error) => {

          if (error) {

            console.error(
              "MQTT subscribe error:",
              error
            );

            return;
          }


          console.log(
            "Subscribed:",
            MQTT_TOPIC
          );
        }
      );
    };


    // =========================================
    // CONNECT
    // =========================================

    const handleConnect = () => {

      console.log(
        "MQTT CONNECTED"
      );

      setConnected(true);

      subscribe();
    };


    // =========================================
    // MESSAGE
    // =========================================

    const handleMessage = (
      topic,
      message
    ) => {

      if (
        topic !== MQTT_TOPIC
      ) {
        return;
      }


      try {

        const payload =
          JSON.parse(
            message.toString()
          );


        const reading = {

          deviceId:
            payload.device_id,

          ppm:
            Number(
              payload.ppm
            ),

          adc:
            Number(
              payload.adc
            ),

          filteredAdc:
            Number(
              payload.filtered_adc
            ),

          level:
            Number(
              payload.level
            ),

          status:
            payload.status,

          effect:
            payload.effect,

          uptimeMs:
            Number(
              payload.uptime_ms
            ),

          updatedAt:
            new Date(),
        };


        // Latest reading
        setData(
          reading
        );


        // History maksimal 60 data
        setHistory(
          previous => {

            const updated = [
              ...previous,
              reading,
            ];

            return updated.slice(
              -60
            );
          }
        );


        console.log(
          "H2S DATA:",
          reading
        );

      }

      catch (error) {

        console.error(
          "Invalid MQTT payload:",
          error
        );
      }
    };


    // =========================================
    // DISCONNECTED
    // =========================================

    const handleDisconnect = () => {

      setConnected(false);
    };


    // =========================================
    // ERROR
    // =========================================

    const handleError = (
      error
    ) => {

      console.error(
        "MQTT Error:",
        error
      );
    };


    // =========================================
    // EVENTS
    // =========================================

    mqttClient.on(
      "connect",
      handleConnect
    );

    mqttClient.on(
      "message",
      handleMessage
    );

    mqttClient.on(
      "offline",
      handleDisconnect
    );

    mqttClient.on(
      "close",
      handleDisconnect
    );

    mqttClient.on(
      "error",
      handleError
    );


    // Client mungkin sudah connected
    // sebelum hook mount
    if (
      mqttClient.connected
    ) {

      setConnected(true);

      subscribe();
    }


    // =========================================
    // CLEANUP
    // =========================================

    return () => {

      mqttClient.off(
        "connect",
        handleConnect
      );

      mqttClient.off(
        "message",
        handleMessage
      );

      mqttClient.off(
        "offline",
        handleDisconnect
      );

      mqttClient.off(
        "close",
        handleDisconnect
      );

      mqttClient.off(
        "error",
        handleError
      );
    };

  }, []);


  return {
    connected,
    data,
    history,
  };
}