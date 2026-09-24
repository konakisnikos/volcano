#include "StopwatchWidget.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

const float PI = 3.14159265358979323846f;
const double SECOND_HAND_REVOLUTION_SECONDS = 30.0;

ImVec2 pointOnCircle(const ImVec2& center, float radius, float angle)
{
    return ImVec2(center.x + std::cos(angle) * radius,
                  center.y + std::sin(angle) * radius);
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
                    float scale, int symbol, bool paused)
{
    const float halfWidth = 18.0f * scale;
    const float halfHeight = 7.0f * scale;
    const float pressedOffset = button.held ? 2.0f * scale : 0.0f;
    const ImVec2 center(button.center.x, button.center.y + pressedOffset);
    const ImVec2 topLeft(center.x - halfWidth, center.y - halfHeight);
    const ImVec2 bottomRight(center.x + halfWidth, center.y + halfHeight);

    draw->AddRectFilled(ImVec2(topLeft.x + 2.0f * scale, topLeft.y + 4.0f * scale),
                        ImVec2(bottomRight.x + 2.0f * scale, bottomRight.y + 4.0f * scale),
                        IM_COL32(0, 0, 0, 130), 4.0f * scale);
    draw->AddRectFilled(topLeft, bottomRight,
                        button.hovered ? IM_COL32(112, 135, 155, 255)
                                       : IM_COL32(72, 86, 101, 255),
                        4.0f * scale);
    draw->AddRect(topLeft, bottomRight, IM_COL32(181, 195, 205, 230),
                  4.0f * scale, 0, 1.0f * scale);
    draw->AddLine(ImVec2(topLeft.x + 4.0f * scale, topLeft.y + 2.0f * scale),
                  ImVec2(bottomRight.x - 4.0f * scale, topLeft.y + 2.0f * scale),
                  IM_COL32(210, 222, 229, 180), 1.0f * scale);

    const ImU32 ink = IM_COL32(235, 238, 237, 255);
    if (symbol < 0 || symbol > 0) {
        draw->AddLine(ImVec2(center.x - 5.0f * scale, center.y),
                      ImVec2(center.x + 5.0f * scale, center.y), ink, 1.8f * scale);
        if (symbol > 0) {
            draw->AddLine(ImVec2(center.x, center.y - 5.0f * scale),
                          ImVec2(center.x, center.y + 5.0f * scale), ink, 1.8f * scale);
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

StopwatchAction drawStopwatch(double elapsedSeconds, float timeScale, bool paused)
{
    const ImGuiIO& io = ImGui::GetIO();
    const float scale = std::max(0.72f, std::min(1.0f, io.DisplaySize.y / 768.0f));
    const ImVec2 size(280.0f * scale, 326.0f * scale);
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
    const ImVec2 center(origin.x + 140.0f * scale, origin.y + 185.0f * scale);
    const float radius = 111.0f * scale;

    const PushButton slower = buttonHitbox(
        "##slower", ImVec2(center.x - 70.0f * scale, origin.y + 40.0f * scale),
        scale, "Halve simulation speed ([)");
    const PushButton pause = buttonHitbox(
        "##pause", ImVec2(center.x, origin.y + 21.0f * scale),
        scale, paused ? "Resume simulation (P)" : "Pause simulation (P)");
    const PushButton faster = buttonHitbox(
        "##faster", ImVec2(center.x + 70.0f * scale, origin.y + 40.0f * scale),
        scale, "Double simulation speed (])");

    // The stems sit behind the case; the three caps remain clickable above it.
    const PushButton buttons[3] = {slower, pause, faster};
    for (int i = 0; i < 3; ++i) {
        const float stemBottom = i == 1 ? origin.y + 76.0f * scale
                                         : origin.y + 93.0f * scale;
        draw->AddLine(ImVec2(buttons[i].center.x, buttons[i].center.y + 6.0f * scale),
                      ImVec2(buttons[i].center.x, stemBottom),
                      IM_COL32(52, 65, 77, 255), 12.0f * scale);
        draw->AddLine(ImVec2(buttons[i].center.x - 3.0f * scale,
                             buttons[i].center.y + 8.0f * scale),
                      ImVec2(buttons[i].center.x - 3.0f * scale, stemBottom),
                      IM_COL32(157, 173, 185, 165), 2.0f * scale);
    }

    draw->AddCircleFilled(ImVec2(center.x + 4.0f * scale, center.y + 7.0f * scale),
                          radius + 7.0f * scale, IM_COL32(0, 0, 0, 100), 96);
    draw->AddCircleFilled(center, radius + 7.0f * scale,
                          IM_COL32(24, 31, 42, 255), 96);
    draw->AddCircle(center, radius + 5.0f * scale,
                    IM_COL32(158, 183, 204, 255), 96, 2.0f * scale);
    draw->AddCircle(center, radius + 1.0f * scale,
                    IM_COL32(45, 59, 75, 255), 96, 4.0f * scale);
    draw->AddCircleFilled(center, radius - 4.0f * scale,
                          IM_COL32(13, 18, 27, 255), 96);
    draw->AddCircle(center, radius - 5.0f * scale,
                    IM_COL32(105, 126, 144, 170), 96, 1.0f * scale);

    for (int tick = 0; tick < 60; ++tick) {
        const float angle = -PI * 0.5f + tick * 2.0f * PI / 60.0f;
        const bool major = tick % 10 == 0;
        const bool medium = tick % 5 == 0;
        const float inner = (major ? 86.0f : medium ? 91.0f : 96.0f) * scale;
        draw->AddLine(pointOnCircle(center, inner, angle),
                      pointOnCircle(center, 101.0f * scale, angle),
                      major ? IM_COL32(232, 231, 220, 255)
                            : IM_COL32(170, 183, 190, 190),
                      (major ? 2.0f : 1.0f) * scale);
        if (major) {
            char label[4];
            std::snprintf(label, sizeof(label), "%d", tick == 0 ? 30 : tick / 2);
            centeredText(draw, pointOnCircle(center, 76.0f * scale, angle),
                         15.0f * scale, IM_COL32(238, 238, 230, 255), label);
        }
    }

    const ImVec2 minuteCenter(center.x, center.y - 43.0f * scale);
    draw->AddCircleFilled(minuteCenter, 26.0f * scale,
                          IM_COL32(22, 27, 36, 255), 48);
    draw->AddCircle(minuteCenter, 26.0f * scale,
                    IM_COL32(111, 129, 144, 220), 48, 1.0f * scale);
    for (int tick = 0; tick < 30; tick += 5) {
        const float angle = -PI * 0.5f + tick * 2.0f * PI / 30.0f;
        draw->AddLine(pointOnCircle(minuteCenter, 20.0f * scale, angle),
                      pointOnCircle(minuteCenter, 23.0f * scale, angle),
                      IM_COL32(205, 208, 205, 230), 1.0f * scale);
    }

    const double time = std::max(0.0, elapsedSeconds);
    const float minuteAngle = -PI * 0.5f +
        static_cast<float>(std::fmod(time / 60.0, 30.0) / 30.0 * 2.0 * PI);
    draw->AddLine(minuteCenter, pointOnCircle(minuteCenter, 18.0f * scale, minuteAngle),
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
                  IM_COL32(245, 92, 27, 65), 6.0f * scale);
    draw->AddLine(pointOnCircle(center, -14.0f * scale, secondAngle),
                  pointOnCircle(center, 89.0f * scale, secondAngle),
                  IM_COL32(255, 136, 52, 255), 2.2f * scale);
    draw->AddCircleFilled(center, 5.5f * scale, IM_COL32(92, 109, 121, 255), 24);
    draw->AddCircleFilled(center, 2.4f * scale, IM_COL32(249, 142, 67, 255), 16);

    char speedText[24];
    std::snprintf(speedText, sizeof(speedText), "%.2gx", timeScale);
    centeredText(draw, ImVec2(center.x, center.y + 52.0f * scale),
                 19.0f * scale, IM_COL32(236, 235, 227, 255), speedText);
    if (paused) {
        centeredText(draw, ImVec2(center.x, center.y + 19.0f * scale),
                     10.0f * scale, IM_COL32(246, 148, 80, 255), "PAUSED");
    }

    drawPushButton(draw, slower, scale, -1, paused);
    drawPushButton(draw, pause, scale, 0, paused);
    drawPushButton(draw, faster, scale, 1, paused);
    ImGui::End();

    if (slower.clicked) return StopwatchAction::Slower;
    if (pause.clicked) return StopwatchAction::TogglePause;
    if (faster.clicked) return StopwatchAction::Faster;
    return StopwatchAction::None;
}
