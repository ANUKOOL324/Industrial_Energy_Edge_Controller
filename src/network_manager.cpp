#include "network_manager.h"
#include "config.h"

#ifndef UNIT_TEST
#include <WiFi.h>
#include "secrets.h"
#if IEEC_ENABLE_BLYNK
#include <BlynkSimpleEsp32.h>
#endif
#endif

void NetworkManager::begin() {
#ifndef UNIT_TEST
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
#if IEEC_ENABLE_BLYNK
    Blynk.config(BLYNK_AUTH_TOKEN);
#endif
    status_.connected = false;
#endif
}

void NetworkManager::tick(uint32_t nowMs) {
#ifndef UNIT_TEST
    status_.connected = WiFi.status() == WL_CONNECTED;
    if (!status_.connected && nowMs - status_.lastAttemptMs >= config::wifiRetryIntervalMs) {
        if (status_.lastAttemptMs != 0) ++status_.failureCount;
        status_.lastAttemptMs = nowMs;
        ++status_.reconnectCount;
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    if (status_.connected) {
#if IEEC_ENABLE_BLYNK
        if (!Blynk.connected() && nowMs - status_.lastAttemptMs >= config::wifiRetryIntervalMs) {
            status_.lastAttemptMs = nowMs;
            Blynk.connect(50);
        }
        Blynk.run();
#endif
    }
#else
    (void)nowMs;
#endif
}

void NetworkManager::publish(const EnergyData &data) {
#if !defined(UNIT_TEST) && IEEC_ENABLE_BLYNK
    if (Blynk.connected()) {
        Blynk.virtualWrite(V0, data.voltage);
        Blynk.virtualWrite(V1, data.current);
        Blynk.virtualWrite(V2, data.power);
        Blynk.virtualWrite(V3, data.energyKWh);
        Blynk.virtualWrite(V4, data.cost);
    }
#else
    (void)data;
#endif
}
