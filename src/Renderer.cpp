#include "Renderer.h"

void Renderer::begin() {
    M5.Display.setRotation(1); // landscape: 240 wide, 135 tall
    M5.Display.fillScreen(TFT_BLACK);

    canvas_.setColorDepth(16);
    // 240*135*2 = 65 KB in internal RAM: every pixel is written one by one,
    // column by column, and through the PSRAM cache that is much slower.
    canvas_.setPsram(false);
    if (canvas_.createSprite(M5.Display.width(), M5.Display.height()) == nullptr) {
        canvas_.setPsram(true);
        canvas_.createSprite(M5.Display.width(), M5.Display.height());
    }
}

void Renderer::draw(const Terrain &terrain, const Camera &camera, const char *caption) {
    const uint32_t t0 = micros();
    voxel_.render(terrain, camera, static_cast<uint16_t *>(canvas_.getBuffer()), canvas_.width(),
                  canvas_.height());
    if (caption != nullptr) {
        canvas_.setFont(&fonts::Font2);
        canvas_.setTextDatum(top_center);
        canvas_.setTextColor(TFT_BLACK);
        canvas_.drawString(caption, canvas_.width() / 2 + 1, 5); // a shadow for contrast
        canvas_.setTextColor(0xFFE0C0u);
        canvas_.drawString(caption, canvas_.width() / 2, 4);
    }
    const uint32_t t1 = micros();
    canvas_.pushSprite(0, 0);
    pushUs_ = micros() - t1;
    paintUs_ = t1 - t0;
}

void Renderer::writeSnapshot(Print &out) {
    out.printf("SNAP %d %d\n", canvas_.width(), canvas_.height());
    // A 16-bit LovyanGFX sprite already keeps its pixels high byte first,
    // so the buffer goes out as it is.
    out.write(static_cast<const uint8_t *>(canvas_.getBuffer()),
              canvas_.width() * canvas_.height() * 2);
    out.flush();
}
