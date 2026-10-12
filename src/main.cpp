
#include <Arduino.h>
#include <Wire.h>
#include <VL53L1X.h>
#include <Adafruit_NeoPixel.h>

// LumaLight V1.1
// Proximity lighting + pixel sweep + hand reactivation

constexpr int SDA_PIN = 21;
constexpr int SCL_PIN = 22;
constexpr int LED_PIN = 18;
constexpr int LED_COUNT = 12;

// Distances in millimeters
constexpr int ACTIVATION_MM = 1524; // 5 feet
constexpr int REARM_MM = 1700;
constexpr int CLOSE_MM = 300;
constexpr int MID_MM = 900;

// Hand gesture thresholds
constexpr int WAVE_READY_MM = 650;
constexpr int WAVE_TRIGGER_MM = 350;

// Timing
constexpr uint32_t ACTIVE_MS = 120000;
constexpr uint32_t SWEEP_MS = 1100;
constexpr uint32_t FADE_OUT_MS = 1000;
constexpr uint32_t COOLDOWN_MS = 2000;
constexpr uint32_t CLEAR_MS = 1000;
constexpr uint32_t FRAME_MS = 20;

// USB-friendly maximum electrical brightness
constexpr float MAX_BRIGHTNESS = 110.0f;

VL53L1X sensor;
Adafruit_NeoPixel ring(
    LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800
);

enum class State {
    IDLE,
    ACTIVE,
    FADING,
    WAIT_REARM
};

State state = State::IDLE;

uint32_t activatedAt = 0;
uint32_t fadeAt = 0;
uint32_t waitAt = 0;
uint32_t clearAt = 0;
uint32_t lastFrame = 0;
uint32_t lastLog = 0;

bool waveReady = false;
int activationSamples = 0;
int lastDistance = -1;

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

    waveReady = false;
    clearAt = 0;
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
        waitAt = now;
        clearAt = 0;
        waveReady = false;
        state = State::WAIT_REARM;

        Serial.println("[LIGHT] OFF");
        Serial.println("[LIGHT] 2 second cooldown");
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

    uint32_t now = millis();

    switch (state) {

        case State::IDLE:
            if (valid && distance <= ACTIVATION_MM) {
                if (++activationSamples >= 2) {
                    activate(distance);
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

        case State::WAIT_REARM: {

            // Minimum 2-second lock after lights go off
            if (now - waitAt < COOLDOWN_MS) {
                break;
            }

            // Automatic rearm if detection area clears
            bool clear =
                (valid && distance > REARM_MM) ||
                noTarget;

            if (clear) {
                if (clearAt == 0) clearAt = now;

                if (now - clearAt >= CLEAR_MS) {
                    state = State::IDLE;
                    waveReady = false;
                    clearAt = 0;

                    Serial.println("[LIGHT] Auto rearmed");
                    break;
                }
            } else {
                clearAt = 0;
            }

            // Manual reactivation:
            // First see a farther distance,
            // then detect a hand moving close.
            if (valid && distance >= WAVE_READY_MM) {
                waveReady = true;
            }

            if (waveReady && valid &&
                distance <= WAVE_TRIGGER_MM) {

                Serial.println("[GESTURE] Hand detected");
                activate(distance);
            }
            break;
        }
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

    const char* name = "IDLE";

    switch (state) {
        case State::IDLE: name = "IDLE"; break;
        case State::ACTIVE: name = "ACTIVE"; break;
        case State::FADING: name = "FADING"; break;
        case State::WAIT_REARM: name = "REARM"; break;
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
        "| WaveReady: %s\n",
        name,
        lastDistance,
        displayedBrightness,
        (unsigned long)remaining,
        waveReady ? "YES" : "NO"
    );
}

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("=== LumaLight V1.1 ===");

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

    Serial.println("[SENSOR] Ready");
    Serial.println("[LIGHT] Activation: 5 feet");
    Serial.println("[LIGHT] Timeout: 120 seconds");
    Serial.println("[LIGHT] Hand-wave reactivation enabled");
}

void loop() {
    updateState();
    updateSensor();
    renderLights();
    logStatus();
}

