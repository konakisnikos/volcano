#include "StopwatchWidget.h"

#include <imgui.h>
#include <common/texture.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

const float PI = 3.14159265358979323846f;
const double SECOND_HAND_REVOLUTION_SECONDS = 30.0;
const double FINGER_PRESS_SECONDS = 0.28;
GLuint handAtlas = 0;
double fingerPressStart[3] = {-1.0, -1.0, -1.0};

struct AtlasRegion {
    float left;
    float top;
    float right;
    float bottom;
};

void drawHandPart(ImDrawList* draw, const AtlasRegion& source,
                  const ImVec2& topLeft, const ImVec2& size)
{
    if (!handAtlas) return;
    const float atlasWidth = 1672.0f;
    const float atlasHeight = 941.0f;
    draw->AddImage(static_cast<ImTextureID>(handAtlas), topLeft,
                   ImVec2(topLeft.x + size.x, topLeft.y + size.y),
                   ImVec2(source.left / atlasWidth, source.top / atlasHeight),
                   ImVec2(source.right / atlasWidth, source.bottom / atlasHeight));
}

float smoothStep(float value)
{
    value = std::max(0.0f, std::min(1.0f, value));
    return value * value * (3.0f - 2.0f * value);
}

float fingerPressAmount(int finger)
{
    if (fingerPressStart[finger] < 0.0) return 0.0f;
    const double age = ImGui::GetTime() - fingerPressStart[finger];
    if (age < 0.0 || age >= FINGER_PRESS_SECONDS) return 0.0f;
    if (age < 0.085) return smoothStep(static_cast<float>(age / 0.085));
    return 1.0f - smoothStep(static_cast<float>((age - 0.085) /
                                                  (FINGER_PRESS_SECONDS - 0.085)));
}

ImVec2 pointOnCircle(const ImVec2& center, float radius, float angle)
{
    return ImVec2(center.x + std::cos(angle) * radius,
                  center.y + std::sin(angle) * radius);
}

ImVec2 rotatedPoint(const ImVec2& center, float x, float y, float angle)
{
    return ImVec2(center.x + x * std::cos(angle) - y * std::sin(angle),
                  center.y + x * std::sin(angle) + y * std::cos(angle));
}

void drawArc(ImDrawList* draw, const ImVec2& center, float radius,
             float startAngle, float endAngle, ImU32 color, float thickness)
{
    const int segments = 36;
    ImVec2 previous = pointOnCircle(center, radius, startAngle);
    for (int i = 1; i <= segments; ++i) {
        const float angle = startAngle + (endAngle - startAngle) * i / segments;
        const ImVec2 next = pointOnCircle(center, radius, angle);
        draw->AddLine(previous, next, color, thickness);
        previous = next;
    }
}

void centeredText(ImDrawList* draw, const ImVec2& center, float size,
                  ImU32 color, const char* text)
{
    ImFont* font = ImGui::GetFont();
    const ImVec2 extent = font->CalcTextSizeA(size, 1000.0f, 0.0f, text);
    draw->AddText(font, size,
                  ImVec2(center.x - extent.x * 0.5f, center.y - extent.y * 0.5f),
                  color, text);
}

struct PushButton {
    ImVec2 center;
    bool hovered;
    bool held;
    bool clicked;
};

PushButton buttonHitbox(const char* id, const ImVec2& center, float scale,
                        const char* tooltip)
{
    const ImVec2 hitSize(42.0f * scale, 30.0f * scale);
    ImGui::SetCursorScreenPos(ImVec2(center.x - hitSize.x * 0.5f,
                                     center.y - hitSize.y * 0.5f));
    const bool clicked = ImGui::InvisibleButton(id, hitSize);
    const bool hovered = ImGui::IsItemHovered();
    if (hovered) ImGui::SetTooltip("%s", tooltip);
    return PushButton{center, hovered, ImGui::IsItemActive(), clicked};
}

void drawPushButton(ImDrawList* draw, const PushButton& button,
                    float scale, int symbol, bool paused, float pressAmount)
{
    const float halfWidth = 17.0f * scale;
    const float halfHeight = 7.0f * scale;
    const float pressedOffset = std::max(button.held ? 2.0f : 0.0f,
                                         3.0f * pressAmount) * scale;
    const ImVec2 center(button.center.x, button.center.y + pressedOffset);
    const float angle = symbol < 0 ? -0.40f : symbol > 0 ? 0.40f : 0.0f;
    const ImVec2 a = rotatedPoint(center, -halfWidth, -halfHeight, angle);
    const ImVec2 b = rotatedPoint(center, halfWidth, -halfHeight, angle);
    const ImVec2 c = rotatedPoint(center, halfWidth, halfHeight, angle);
    const ImVec2 d = rotatedPoint(center, -halfWidth, halfHeight, angle);
    const ImVec2 shadow(0.0f, 3.0f * scale);
    draw->AddQuadFilled(ImVec2(a.x + shadow.x, a.y + shadow.y),
                        ImVec2(b.x + shadow.x, b.y + shadow.y),
                        ImVec2(c.x + shadow.x, c.y + shadow.y),
                        ImVec2(d.x + shadow.x, d.y + shadow.y),
                        IM_COL32(1, 3, 7, 175));
    draw->AddQuadFilled(a, b, c, d,
                        button.hovered ? IM_COL32(144, 168, 185, 255)
                                       : IM_COL32(91, 110, 128, 255));
    draw->AddQuad(a, b, c, d, IM_COL32(210, 223, 228, 230), 1.3f * scale);
    draw->AddLine(rotatedPoint(center, -14.0f * scale, -4.5f * scale, angle),
                  rotatedPoint(center, 14.0f * scale, -4.5f * scale, angle),
                  IM_COL32(231, 239, 241, 215), 1.6f * scale);
    draw->AddLine(rotatedPoint(center, -14.0f * scale, 4.0f * scale, angle),
                  rotatedPoint(center, 14.0f * scale, 4.0f * scale, angle),
                  IM_COL32(28, 40, 53, 215), 1.5f * scale);

    const ImU32 ink = IM_COL32(238, 243, 239, 245);
    if (symbol < 0 || symbol > 0) {
        draw->AddLine(rotatedPoint(center, -4.5f * scale, 0.0f, angle),
                      rotatedPoint(center, 4.5f * scale, 0.0f, angle),
                      ink, 1.6f * scale);
        if (symbol > 0) {
            draw->AddLine(rotatedPoint(center, 0.0f, -4.5f * scale, angle),
                          rotatedPoint(center, 0.0f, 4.5f * scale, angle),
                          ink, 1.6f * scale);
        }
    } else if (paused) {
        draw->AddTriangleFilled(ImVec2(center.x - 3.0f * scale, center.y - 5.0f * scale),
                                ImVec2(center.x - 3.0f * scale, center.y + 5.0f * scale),
                                ImVec2(center.x + 5.0f * scale, center.y), ink);
    } else {
        draw->AddRectFilled(ImVec2(center.x - 4.5f * scale, center.y - 5.0f * scale),
                            ImVec2(center.x - 1.5f * scale, center.y + 5.0f * scale), ink);
        draw->AddRectFilled(ImVec2(center.x + 1.5f * scale, center.y - 5.0f * scale),
                            ImVec2(center.x + 4.5f * scale, center.y + 5.0f * scale), ink);
    }
}

} // namespace

void initializeStopwatchWidget()
{
    if (!handAtlas) {
        handAtlas = loadSOILWithAlpha(ELEMENTAL_ASSET_DIR "/stopwatch_hand_atlas.png");
    }
}

void shutdownStopwatchWidget()
{
    if (handAtlas) glDeleteTextures(1, &handAtlas);
    handAtlas = 0;
}

void triggerStopwatchFinger(StopwatchAction action)
{
    int finger = -1;
    if (action == StopwatchAction::Slower) finger = 0;
    if (action == StopwatchAction::TogglePause) finger = 1;
    if (action == StopwatchAction::Faster) finger = 2;
    if (finger >= 0) fingerPressStart[finger] = ImGui::GetTime();
}

StopwatchAction drawStopwatch(double elapsedSeconds, float timeScale, bool paused)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float scale = std::max(0.72f, std::min(1.0f, io.DisplaySize.y / 768.0f));
    const ImVec2 size(350.0f * scale, 426.0f * scale);
    const ImVec2 position(14.0f * scale,
                          std::max(8.0f * scale,
                                   io.DisplaySize.y - size.y - 14.0f * scale));
    ImGui::SetNextWindowPos(position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBackground |
                                   ImGuiWindowFlags_NoNavFocus;
    ImGui::Begin("Simulation stopwatch", nullptr, flags);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 origin = ImGui::GetWindowPos();
    const ImVec2 faceOrigin(origin.x + 25.0f * scale,
                            origin.y + 100.0f * scale);
    const ImVec2 center(faceOrigin.x + 140.0f * scale,
                        faceOrigin.y + 185.0f * scale);
    const float radius = 111.0f * scale;

    drawHandPart(draw, AtlasRegion{105.0f, 145.0f, 850.0f, 925.0f},
                 ImVec2(faceOrigin.x + 4.0f * scale,
                        faceOrigin.y + 54.0f * scale),
                 ImVec2(270.0f * scale, 280.0f * scale));

    const PushButton slower = buttonHitbox(
        "##slower", ImVec2(center.x - 82.0f * scale, faceOrigin.y + 55.0f * scale),
        scale, "Halve simulation speed ([)");
    const PushButton pause = buttonHitbox(
        "##pause", ImVec2(center.x, faceOrigin.y + 31.0f * scale),
        scale, paused ? "Resume simulation (P)" : "Pause simulation (P)");
    const PushButton faster = buttonHitbox(
        "##faster", ImVec2(center.x + 82.0f * scale, faceOrigin.y + 55.0f * scale),
        scale, "Double simulation speed (])");

    // The pushers emerge from behind the upper edge of the case.
    const PushButton buttons[3] = {slower, pause, faster};
    for (int i = 0; i < 3; ++i) {
        const float side = i == 0 ? -1.0f : i == 2 ? 1.0f : 0.0f;
        const ImVec2 stemBase(center.x + side * 67.0f * scale,
                              center.y - (side == 0.0f ? 116.0f : 94.0f) * scale);
        const ImVec2 stemTop(buttons[i].center.x,
                             buttons[i].center.y + 7.0f * scale);
        draw->AddLine(stemBase, stemTop, IM_COL32(7, 13, 21, 255), 13.0f * scale);
        draw->AddLine(stemBase, stemTop, IM_COL32(54, 76, 96, 255), 8.0f * scale);
        draw->AddLine(ImVec2(stemBase.x - 2.0f * scale, stemBase.y),
                      ImVec2(stemTop.x - 2.0f * scale, stemTop.y),
                      IM_COL32(160, 185, 202, 205), 1.8f * scale);
    }

    const ImVec2 crownMin(center.x + 108.0f * scale, center.y - 47.0f * scale);
    const ImVec2 crownMax(center.x + 132.0f * scale, center.y - 31.0f * scale);
    draw->AddRectFilled(crownMin, crownMax, IM_COL32(13, 21, 31, 255), 3.0f * scale);
    draw->AddRect(ImVec2(crownMin.x + 9.0f * scale, crownMin.y), crownMax,
                  IM_COL32(156, 175, 187, 220), 3.0f * scale, 0, 1.2f * scale);
    for (int groove = 0; groove < 4; ++groove) {
        const float x = crownMin.x + (13.0f + groove * 4.0f) * scale;
        draw->AddLine(ImVec2(x, crownMin.y + 3.0f * scale),
                      ImVec2(x, crownMax.y - 3.0f * scale),
                      IM_COL32(80, 103, 119, 195), 1.0f * scale);
    }

    draw->AddCircleFilled(ImVec2(center.x + 5.0f * scale, center.y + 7.0f * scale),
                          radius + 10.0f * scale, IM_COL32(0, 0, 0, 105), 128);
    draw->AddCircleFilled(center, radius + 9.0f * scale,
                          IM_COL32(9, 14, 22, 255), 128);
    draw->AddCircleFilled(center, radius + 6.5f * scale,
                          IM_COL32(75, 103, 125, 255), 128);
    draw->AddCircleFilled(center, radius + 3.0f * scale,
                          IM_COL32(24, 37, 50, 255), 128);
    draw->AddCircleFilled(center, radius - 0.5f * scale,
                          IM_COL32(177, 195, 205, 255), 128);
    draw->AddCircleFilled(center, radius - 2.0f * scale,
                          IM_COL32(19, 28, 39, 255), 128);
    draw->AddCircleFilled(center, radius - 5.0f * scale,
                          IM_COL32(5, 8, 13, 255), 128);
    drawArc(draw, center, radius + 7.0f * scale, 3.55f, 5.55f,
            IM_COL32(224, 236, 241, 220), 2.1f * scale);
    drawArc(draw, center, radius + 5.0f * scale, 0.20f, 1.65f,
            IM_COL32(80, 134, 171, 190), 1.8f * scale);
    drawArc(draw, center, radius - 2.0f * scale, 3.50f, 5.45f,
            IM_COL32(223, 232, 237, 180), 1.0f * scale);

    for (int tick = 0; tick < 120; ++tick) {
        const float angle = -PI * 0.5f + tick * 2.0f * PI / 120.0f;
        const bool major = tick % 20 == 0;
        const bool medium = tick % 10 == 0;
        const float inner = (major ? 90.0f : medium ? 95.0f : 98.0f) * scale;
        draw->AddLine(pointOnCircle(center, inner, angle),
                      pointOnCircle(center, 102.0f * scale, angle),
                      major ? IM_COL32(236, 238, 235, 255)
                            : medium ? IM_COL32(204, 214, 219, 220)
                                     : IM_COL32(171, 185, 194, 150),
                      (major ? 2.1f : medium ? 1.2f : 0.8f) * scale);
        if (major) {
            char label[4];
            std::snprintf(label, sizeof(label), "%d", tick == 0 ? 30 : tick / 4);
            centeredText(draw, pointOnCircle(center, 77.0f * scale, angle),
                         16.5f * scale, IM_COL32(241, 243, 240, 255), label);
        }
    }

    const ImVec2 minuteCenter(center.x, center.y - 41.0f * scale);
    draw->AddCircleFilled(minuteCenter, 24.0f * scale,
                          IM_COL32(11, 16, 23, 255), 64);
    draw->AddCircle(minuteCenter, 24.0f * scale,
                    IM_COL32(105, 129, 147, 200), 64, 1.0f * scale);
    for (int tick = 0; tick < 30; ++tick) {
        const float angle = -PI * 0.5f + tick * 2.0f * PI / 30.0f;
        draw->AddLine(pointOnCircle(minuteCenter, (tick % 5 == 0 ? 17.0f : 20.0f) * scale, angle),
                      pointOnCircle(minuteCenter, 21.0f * scale, angle),
                      tick % 5 == 0 ? IM_COL32(226, 230, 226, 235)
                                    : IM_COL32(165, 183, 193, 170),
                      1.0f * scale);
    }

    const double time = std::max(0.0, elapsedSeconds);
    const float minuteAngle = -PI * 0.5f +
        static_cast<float>(std::fmod(time / 60.0, 30.0) / 30.0 * 2.0 * PI);
    draw->AddLine(minuteCenter, pointOnCircle(minuteCenter, 16.0f * scale, minuteAngle),
                  IM_COL32(237, 142, 60, 255), 2.0f * scale);
    draw->AddCircleFilled(minuteCenter, 2.5f * scale,
                          IM_COL32(204, 211, 212, 255), 16);

    const float secondAngle = -PI * 0.5f +
        static_cast<float>(std::fmod(time, SECOND_HAND_REVOLUTION_SECONDS) /
                           SECOND_HAND_REVOLUTION_SECONDS * 2.0 * PI);
    const int ghostCount = paused ? 0 :
        timeScale >= 7.0f ? 7 : timeScale >= 3.5f ? 6 : timeScale >= 1.75f ? 4 : 0;
    for (int i = ghostCount; i >= 1; --i) {
        const double ghostAge = i * 0.1 * timeScale;
        if (time < ghostAge) continue;
        const float ghostAngle = -PI * 0.5f +
            static_cast<float>(std::fmod(time - ghostAge,
                                         SECOND_HAND_REVOLUTION_SECONDS) /
                               SECOND_HAND_REVOLUTION_SECONDS * 2.0 * PI);
        const int alpha = 12 + (ghostCount - i) * 9;
        draw->AddLine(pointOnCircle(center, -10.0f * scale, ghostAngle),
                      pointOnCircle(center, 87.0f * scale, ghostAngle),
                      IM_COL32(255, 103, 36, alpha), 2.5f * scale);
    }
    draw->AddLine(pointOnCircle(center, -14.0f * scale, secondAngle),
                  pointOnCircle(center, 89.0f * scale, secondAngle),
                  IM_COL32(245, 92, 27, 65), 5.0f * scale);
    draw->AddLine(pointOnCircle(center, -14.0f * scale, secondAngle),
                  pointOnCircle(center, 89.0f * scale, secondAngle),
                  IM_COL32(255, 145, 52, 255), 1.9f * scale);
    draw->AddCircleFilled(center, 6.4f * scale, IM_COL32(116, 139, 155, 255), 24);
    draw->AddCircleFilled(center, 4.3f * scale, IM_COL32(16, 22, 31, 255), 24);
    draw->AddCircleFilled(center, 2.5f * scale, IM_COL32(249, 142, 67, 255), 16);

    char speedText[24];
    std::snprintf(speedText, sizeof(speedText), "%.2gx", timeScale);
    centeredText(draw, ImVec2(center.x, center.y + 40.0f * scale),
                 18.0f * scale, IM_COL32(238, 240, 236, 255), speedText);
    if (paused) {
        centeredText(draw, ImVec2(center.x, center.y + 60.0f * scale),
                     10.0f * scale, IM_COL32(246, 148, 80, 255), "PAUSED");
    }

    const float leftPress = fingerPressAmount(0);
    const float middlePress = fingerPressAmount(1);
    const float rightPress = fingerPressAmount(2);
    drawPushButton(draw, slower, scale, -1, paused, leftPress);
    drawPushButton(draw, pause, scale, 0, paused, middlePress);
    drawPushButton(draw, faster, scale, 1, paused, rightPress);

    drawHandPart(draw, AtlasRegion{510.0f, 30.0f, 840.0f, 300.0f},
                 ImVec2(slower.center.x - 90.0f * scale,
                        slower.center.y - 74.0f * scale + leftPress * 9.0f * scale),
                 ImVec2(100.0f * scale, 82.0f * scale));
    drawHandPart(draw, AtlasRegion{970.0f, 25.0f, 1170.0f, 310.0f},
                 ImVec2(pause.center.x - 30.0f * scale,
                        pause.center.y - 87.0f * scale + middlePress * 9.0f * scale),
                 ImVec2(60.0f * scale, 91.0f * scale));
    drawHandPart(draw, AtlasRegion{1270.0f, 30.0f, 1605.0f, 300.0f},
                 ImVec2(faster.center.x - 9.0f * scale,
                        faster.center.y - 75.0f * scale + rightPress * 9.0f * scale),
                 ImVec2(106.0f * scale, 86.0f * scale));
    ImGui::End();

    if (slower.clicked) return StopwatchAction::Slower;
    if (pause.clicked) return StopwatchAction::TogglePause;
    if (faster.clicked) return StopwatchAction::Faster;
    return StopwatchAction::None;
}
