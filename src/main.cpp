#include <M5Unified.h>
#include <esp_heap_caps.h>
#include <esp_random.h>

#include "DisplayTimeout.h"
#include "Flight.h"
#include "Renderer.h"
#include "Terrain.h"
#include "Tilt.h"

namespace {

constexpr uint8_t kBrightness = 120;
constexpr uint32_t kCaptionMs = 1500;
constexpr uint32_t kSleepingLoopMs = 20; // 50 Hz while the display is off

// The neutral grip is taken this long after start-up and after the display
// wakes: by then the stick is in the pilot's hands, not still being picked up.
constexpr uint32_t kSettleMs = 700;

const char *const kSpeedNames[Flight::kSpeedLevels] = {"SLOW", "CRUISE", "FAST"};

// 144 KB. As a static object it lands in internal RAM, which the renderer's
// random reads need: through the PSRAM cache they are much slower.
Terrain terrain;
Renderer renderer;
Flight flight;
Tilt tilt;
DisplayTimeout displayTimeout;

bool displayAwake = true;

bool recenterPending = true;
uint32_t recenterAtMs = kSettleMs;

const char *caption = nullptr;
uint32_t captionSinceMs = 0;

// Self-testing over serial.
String command;
bool perfOn = false;
uint32_t perfSinceMs = 0;
uint32_t perfFrames = 0;
uint64_t perfPaintUs = 0;
uint64_t perfPushUs = 0;
bool accelOn = false;
uint32_t accelSinceMs = 0;

void showCaption(const char *text, uint32_t now) {
    caption = text;
    captionSinceMs = now;
}

void scheduleRecenter(uint32_t now) {
    recenterPending = true;
    recenterAtMs = now + kSettleMs;
}

void newPlanet(uint32_t now) {
    terrain.generate(esp_random());
    flight.begin(now, terrain);
}

void setDisplayAwake(bool awake, uint32_t now) {
    if (awake == displayAwake) {
        return;
    }
    displayAwake = awake;

    if (awake) {
        M5.Display.wakeup();
        M5.Display.setBrightness(kBrightness);
        scheduleRecenter(now);
    } else {
        // The backlight is the main power consumer, so it is turned off
        // separately from putting the panel itself to sleep.
        M5.Display.setBrightness(0);
        M5.Display.sleep();
    }
}

void printStatus() {
    const Camera &c = flight.camera();
    Serial.printf("ST seed=%08lx speed=%s x=%.1f y=%.1f heading=%.2f alt=%.1f ground=%.1f "
                  "heap=%u internal=%u\n",
                  static_cast<unsigned long>(terrain.seed()), kSpeedNames[flight.speedLevel()], c.x,
                  c.y, c.heading, c.altitude, terrain.groundAt(c.x, c.y),
                  static_cast<unsigned>(ESP.getFreeHeap()),
                  static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
}

// Line commands: s (screenshot), k1 / k2 (press KEY1 / KEY2), perf (a
// performance line every second), acc (accelerometer lines at 10 Hz),
// st (status).
void readSerial(bool &key1, bool &key2, uint32_t now) {
    while (Serial.available() > 0) {
        const char ch = static_cast<char>(Serial.read());
        if (ch != '\n' && ch != '\r') {
            if (command.length() < 16) {
                command += ch;
            }
            continue;
        }
        if (command == "s") {
            renderer.writeSnapshot(Serial);
        } else if (command == "k1") {
            key1 = true;
        } else if (command == "k2") {
            key2 = true;
        } else if (command == "perf") {
            perfOn = !perfOn;
            perfSinceMs = now;
            perfFrames = 0;
            perfPaintUs = perfPushUs = 0;
        } else if (command == "acc") {
            accelOn = !accelOn;
        } else if (command == "st") {
            printStatus();
        }
        command = "";
    }
}

// Nobody may be reading the port any more: then the USB buffer fills up and
// every print would stall the frame, so the lines are skipped instead.
bool serialHasRoom() { return Serial.availableForWrite() >= 128; }

void report(uint32_t now, float ax, float ay, float az) {
    ++perfFrames;
    perfPaintUs += renderer.paintUs();
    perfPushUs += renderer.pushUs();
    if (perfOn && now - perfSinceMs >= 1000 && serialHasRoom()) {
        const float seconds = (now - perfSinceMs) / 1000.0f;
        Serial.printf("PERF fps=%.1f paint=%.1fms push=%.1fms internal=%u\n",
                      perfFrames / seconds, perfPaintUs / 1000.0f / perfFrames,
                      perfPushUs / 1000.0f / perfFrames,
                      static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_INTERNAL)));
        perfSinceMs = now;
        perfFrames = 0;
        perfPaintUs = perfPushUs = 0;
    }
    if (accelOn && now - accelSinceMs >= 100 && serialHasRoom()) {
        Serial.printf("ACC %.3f %.3f %.3f turn=%.2f climb=%.2f\n", ax, ay, az, tilt.turn(),
                      tilt.climb());
        accelSinceMs = now;
    }
}

} // namespace

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    // M5Unified leaves Serial alone (its serial_baudrate is 0 by default).
    Serial.begin(115200);

    M5.Display.setBrightness(kBrightness);
    renderer.begin();

    const uint32_t now = millis();
    newPlanet(now);
    displayTimeout.begin(now);
    scheduleRecenter(now);
    showCaption("MARS", now);
}

void loop() {
    M5.update();

    const uint32_t now = millis();

    // Powering off is not handled here: the side button does it by
    // itself on a double press, via the PMIC.
    bool key1 = M5.BtnA.wasPressed();
    bool key2 = M5.BtnB.wasPressed();
    readSerial(key1, key2, now);

    float ax = 0, ay = 0, az = 0;
    const bool haveAccel = M5.Imu.getAccel(&ax, &ay, &az);
    const bool tilted = haveAccel && tilt.moved(ax, ay, az);

    const bool wasAwake = displayAwake;
    const bool activity = M5.BtnA.isPressed() || M5.BtnB.isPressed() || key1 || key2 || tilted;
    setDisplayAwake(displayTimeout.shouldBeOn(now, activity), now);

    if (!displayAwake) {
        delay(kSleepingLoopMs);
        return;
    }

    // A press that wakes the display only wakes it.
    if (wasAwake && key1) {
        newPlanet(now);
        scheduleRecenter(now);
        showCaption("NEW PLANET", now);
    }
    if (wasAwake && key2) {
        flight.nextSpeed();
        showCaption(kSpeedNames[flight.speedLevel()], now);
    }

    Controls controls;
    if (haveAccel) {
        if (recenterPending && static_cast<int32_t>(now - recenterAtMs) >= 0) {
            tilt.recenter(ax, ay, az);
            recenterPending = false;
        }
        tilt.update(ax, ay, az);
        if (!recenterPending) {
            controls.turn = tilt.turn();
            controls.climb = tilt.climb();
        }
    }
    flight.update(now, controls, terrain);

    if (caption != nullptr && now - captionSinceMs >= kCaptionMs) {
        caption = nullptr;
    }
    renderer.draw(terrain, flight.camera(), caption);

    report(now, ax, ay, az);
}
