#include <Arduino.h>
#include "config.h"
#include "energy_meter.h"
#include "energy_store.h"

#if IEEC_ENABLE_BLYNK
#include "secrets.h"
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#endif

EnergyMeter meter;
EnergyStore store;
EnergySample latest;
float tariff = config::costPerKWh;
uint32_t lastSampleMs = 0;
uint32_t lastStoreMs = 0;

void sampleAndPublish() {
    const uint32_t now = millis();
    const uint32_t elapsed = lastSampleMs == 0 ? 0 : now - lastSampleMs;
    lastSampleMs = now;
    latest = meter.read(elapsed, latest.energyKWh, tariff);

#if IEEC_ENABLE_BLYNK
    if (Blynk.connected()) {
        Blynk.virtualWrite(V0, latest.voltage);
        Blynk.virtualWrite(V1, latest.current);
        Blynk.virtualWrite(V2, latest.power);
        Blynk.virtualWrite(V3, latest.energyKWh);
        Blynk.virtualWrite(V4, latest.cost);
    }
#endif

    Serial.printf("V=%.2f V I=%.3f A P=%.2f W E=%.5f kWh Cost=%.2f\n",
                  latest.voltage, latest.current, latest.power, latest.energyKWh, latest.cost);

    if (now - lastStoreMs >= config::storageCheckpointMs) {
        store.save(latest, tariff);
        lastStoreMs = now;
    }
}

void setup() {
    Serial.begin(115200);
    delay(250);
    store.begin();
    store.load(latest.energyKWh, latest.cost, tariff);
    meter.begin();

#if IEEC_ENABLE_BLYNK
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Blynk.config(BLYNK_AUTH_TOKEN);
#endif

    Serial.println("Industrial Energy Edge Controller started");
}

void loop() {
#if IEEC_ENABLE_BLYNK
    if (WiFi.status() == WL_CONNECTED) {
        Blynk.run();
    }
#endif

    static uint32_t nextSample = 0;
    const uint32_t now = millis();
    if (static_cast<int32_t>(now - nextSample) >= 0) {
        nextSample = now + config::samplePeriodMs;
        sampleAndPublish();
    }
    delay(10);
}

