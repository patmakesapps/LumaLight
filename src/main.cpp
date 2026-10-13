
#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <ArduinoOTA.h>

// Wi-Fi flashing needs include/secrets.h (copy secrets.example.h).
// Without it, or with NO_WIFI (the gift env), the light builds and works, USB flashing only.
#if !defined(NO_WIFI) && __has_include("secrets.h")
#include "secrets.h"
#define HAS_WIFI 1
#else
#define HAS_WIFI 0
#ifndef NO_WIFI
#warning "include/secrets.h missing: building without Wi-Fi flashing"
#endif
#endif

// LumaLight V1.2
// Proximity lighting + pixel sweep
// Off: snapshot the range, any change turns it on

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
constexpr int LED_PIN = 18;
constexpr int LED_COUNT = 12;

// Distances in millimeters
constexpr int ACTIVATION_MM = 1524; // 5 feet
// Readings past this (or no target) all count as "far"
constexpr int FAR_MM = 1800;
constexpr int CLOSE_MM = 300;
constexpr int MID_MM = 900;

// Off: a change this big from the snapshot counts as motion
constexpr int CHANGE_MM = 150;
constexpr int SNAPSHOT_SAMPLES = 5;

// Timing
constexpr uint32_t ACTIVE_MS = 120000;
constexpr uint32_t SWEEP_MS = 1100;
constexpr uint32_t FADE_OUT_MS = 1000;
constexpr uint32_t FRAME_MS = 20;
constexpr uint32_t WIFI_RETRY_MS = 30000;

// USB-friendly maximum electrical brightness
constexpr float MAX_BRIGHTNESS = 110.0f;

VL53L1X sensor;
Adafruit_NeoPixel ring(
    LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800
);

enum class State {
    SNAPSHOT,
    WATCHING,
    ACTIVE,
    FADING
};

State state = State::SNAPSHOT;

uint32_t activatedAt = 0;
uint32_t fadeAt = 0;
uint32_t lastFrame = 0;
uint32_t lastLog = 0;
uint32_t lastWifiTry = 0;

bool otaStarted = false;

int activationSamples = 0;
int lastDistance = -1;

// Range snapshot taken when the light goes off
float snapshotSum = 0;
int snapshotCount = 0;
float snapshot = FAR_MM;

float filteredDistance = ACTIVATION_MM;
float brightness = 0;
float displayedBrightness = 0;
float fadeStartBrightness = 0;

struct RGB {
    float r, g, b;
};

const RGB PURPLE = {170, 15, 255};
const RGB LAVENDER = {205, 115, 245};
const RGB WHITE = {255, 235, 220};

RGB currentColor = PURPLE;

float clamp01(float v) {
    return fmaxf(0, fminf(1, v));
}

float interpolate(float a, float b, float t) {
    return a + (b - a) * clamp01(t);
}

RGB mix(RGB a, RGB b, float t) {
    return {
        interpolate(a.r, b.r, t),
        interpolate(a.g, b.g, t),
        interpolate(a.b, b.b, t)
    };
}

RGB colorForDistance(float distance) {
    if (distance >= MID_MM) {
        float t = (ACTIVATION_MM - distance) /
                  float(ACTIVATION_MM - MID_MM);
        return mix(PURPLE, LAVENDER, t);
    }

    float t = (MID_MM - distance) /
              float(MID_MM - CLOSE_MM);
    return mix(LAVENDER, WHITE, t);
}

float brightnessForDistance(float distance) {
    float proximity = clamp01(
        (ACTIVATION_MM - distance) /
        float(ACTIVATION_MM - CLOSE_MM)
    );

    // 50% to 100% of configured max brightness
    return MAX_BRIGHTNESS * (0.5f + 0.5f * proximity);
}

void activate(int distance) {
    state = State::ACTIVE;
    activatedAt = millis();

    filteredDistance = distance;
    brightness = brightnessForDistance(distance);
    displayedBrightness = 0;
    currentColor = PURPLE;

    activationSamples = 0;

    Serial.println("[LIGHT] ACTIVATED");
    Serial.println("[ANIMATION] Purple pixel sweep");
}

void beginFade() {
    state = State::FADING;
    fadeAt = millis();
    fadeStartBrightness = displayedBrightness;

    Serial.println("[LIGHT] Timeout - fading out");
}

void updateState() {
    uint32_t now = millis();

    if (state == State::ACTIVE &&
        now - activatedAt >= ACTIVE_MS) {
        beginFade();
    }

    if (state == State::FADING &&
        now - fadeAt >= FADE_OUT_MS) {

        ring.clear();
        ring.show();

        displayedBrightness = 0;
        snapshotSum = 0;
        snapshotCount = 0;
        state = State::SNAPSHOT;

        Serial.println("[LIGHT] OFF");
    }
}

void updateSensor() {
    if (!sensor.dataReady()) return;

    int distance = sensor.read(false);
    if (sensor.timeoutOccurred()) return;

    auto status = sensor.ranging_data.range_status;

    bool valid =
        (status == VL53L1X::RangeValid ||
         status == VL53L1X::RangeValidNoWrapCheckFail)
        && distance >= 40 && distance <= 4000;

    bool noTarget =
        status == VL53L1X::OutOfBoundsFail ||
        status == VL53L1X::SignalFail;

    lastDistance = valid ? distance : -1;

    // Other statuses are glitches; skip them
    if (!valid && !noTarget) return;

    int sample = noTarget ? FAR_MM : min(distance, FAR_MM);

    switch (state) {

        case State::SNAPSHOT:
            snapshotSum += sample;

            if (++snapshotCount >= SNAPSHOT_SAMPLES) {
                snapshot = snapshotSum / snapshotCount;
                activationSamples = 0;
                state = State::WATCHING;

                Serial.printf(
                    "[LIGHT] Watching, range %.0fmm\n",
                    snapshot
                );
            }
            break;

        case State::WATCHING:
            // Any change from the snapshot, either way
            if (fabsf(sample - snapshot) > CHANGE_MM) {
                if (++activationSamples >= 2) {
                    activate(min(sample, ACTIVATION_MM));
                }
            } else {
                activationSamples = 0;
            }
            break;

        case State::ACTIVE: {
            // Distance adjusts lighting only.
            // It never resets the 120s countdown.
            float target = valid
                ? fminf(float(distance),
                        float(ACTIVATION_MM))
                : float(ACTIVATION_MM);

            filteredDistance = interpolate(
                filteredDistance, target, 0.25f
            );
            break;
        }

        case State::FADING:
            // Ignore activation while fading out.
            break;
    }
}

uint8_t scaledChannel(float channel, float scale) {
    return uint8_t(
        fminf(255.0f,
              fmaxf(0.0f, channel * scale + 0.5f))
    );
}

void renderLights() {
    uint32_t now = millis();

    if (now - lastFrame < FRAME_MS) return;
    lastFrame = now;

    float sweepProgress = 1.0f;
    float lightLevel = 0;

    if (state == State::ACTIVE) {
        float target =
            brightnessForDistance(filteredDistance);

        brightness = interpolate(brightness, target, 0.15f);

        RGB targetColor =
            colorForDistance(filteredDistance);

        currentColor = mix(
            currentColor, targetColor, 0.12f
        );

        uint32_t elapsed = now - activatedAt;
        sweepProgress = clamp01(
            elapsed / float(SWEEP_MS)
        );

        // Smooth fade-in during the pixel sweep
        float fadeIn = sweepProgress *
                       sweepProgress *
                       (3.0f - 2.0f * sweepProgress);

        lightLevel = brightness * fadeIn;
    }

    else if (state == State::FADING) {
        float progress = clamp01(
            (now - fadeAt) / float(FADE_OUT_MS)
        );

        float fade = 1.0f - progress;
        lightLevel = fadeStartBrightness * fade;
    }

    displayedBrightness = lightLevel;

    // Per-pixel brightness avoids repeated
    // setBrightness() scaling artifacts.
    for (int i = 0; i < LED_COUNT; i++) {
        float pixelLevel = lightLevel;

        if (state == State::ACTIVE &&
            sweepProgress < 1.0f) {

            // Reveal LEDs sequentially clockwise
            float head = sweepProgress * LED_COUNT;

            float reveal = clamp01(
                head - float(i) + 0.15f
            );

            pixelLevel *= reveal;

            // Bright moving leading pixel
            if (int(head) == i) {
                pixelLevel *= 1.25f;
            }
        }

        float scale = pixelLevel / 255.0f;

        ring.setPixelColor(
            i,
            ring.Color(
                scaledChannel(currentColor.r, scale),
                scaledChannel(currentColor.g, scale),
                scaledChannel(currentColor.b, scale)
            )
        );
    }

    ring.show();
}

void logStatus() {
    uint32_t now = millis();
    if (now - lastLog < 1000) return;
    lastLog = now;

    const char* name = "WATCHING";

    switch (state) {
        case State::SNAPSHOT: name = "SNAPSHOT"; break;
        case State::WATCHING: name = "WATCHING"; break;
        case State::ACTIVE: name = "ACTIVE"; break;
        case State::FADING: name = "FADING"; break;
    }

    uint32_t remaining = 0;

    if (state == State::ACTIVE) {
        uint32_t elapsed = now - activatedAt;
        remaining = elapsed < ACTIVE_MS
            ? (ACTIVE_MS - elapsed) / 1000
            : 0;
    }

    Serial.printf(
        "[LumaLight] %s | Distance: %dmm "
        "| Brightness: %.0f | Timer: %lus "
        "| Snapshot: %.0fmm\n",
        name,
        lastDistance,
        displayedBrightness,
        (unsigned long)remaining,
        snapshot
    );
}

#if HAS_WIFI

// Wi-Fi is only for flashing. The light never waits on it.
void beginWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname("lumalight");
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    ArduinoOTA.setHostname("lumalight");
    ArduinoOTA.setPassword(OTA_PASSWORD);

    ArduinoOTA.onStart([]() {
        ring.clear();
        ring.show();
        Serial.println("[OTA] Updating");
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("[OTA] Failed (%u)\n", error);
    });
}

void updateWifi() {
    uint32_t now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastWifiTry >= WIFI_RETRY_MS) {
            lastWifiTry = now;
            WiFi.reconnect();
        }
        return;
    }

    if (!otaStarted) {
        ArduinoOTA.begin();
        otaStarted = true;

        Serial.printf(
            "[WIFI] lumalight.local at %s\n",
            WiFi.localIP().toString().c_str()
        );
    }

    ArduinoOTA.handle();
}

#else

void beginWifi() {}
void updateWifi() {}

#endif

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("=== LumaLight V1.2 ===");

    ring.begin();
    ring.setBrightness(255);
    ring.clear();
    ring.show();

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);

    sensor.setTimeout(500);

    if (!sensor.init()) {
        Serial.println("[ERROR] VL53L1X not found");

        // Leave LEDs off on sensor failure.
        while (true) delay(1000);
    }

    sensor.setDistanceMode(VL53L1X::Long);
    sensor.setMeasurementTimingBudget(50000);
    sensor.startContinuous(75);

    beginWifi();

    Serial.println("[SENSOR] Ready");
    Serial.println("[LIGHT] Turns on when the range changes");
    Serial.println("[LIGHT] Timeout: 120 seconds");
}

void loop() {
    updateState();
    updateSensor();
    renderLights();
    logStatus();
    updateWifi();
}

