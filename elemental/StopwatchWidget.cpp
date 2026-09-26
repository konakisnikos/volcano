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
const int FINGER_FRAME_COUNT = 9;
const float PHOTO_CENTER_X = 519.0f;
const float PHOTO_CENTER_Y = 668.0f;
const float PHOTO_CASE_RADIUS = 285.0f;
GLuint handBack = 0;
GLuint fingerFrames[3][FINGER_FRAME_COUNT] = {};
double fingerPressStart[3] = {-1.0, -1.0, -1.0};

struct PhotoRegion {
    float left;
    float top;
    float right;
    float bottom;
};

const PhotoRegion fingerRegions[3] = {
    {65.0f, 75.0f, 480.0f, 545.0f},
    {335.0f, 0.0f, 745.0f, 510.0f},
    {620.0f, 105.0f, 1082.0f, 650.0f}
};

void drawPhotoRegion(ImDrawList* draw, GLuint texture, const PhotoRegion& region,
                     const ImVec2& watchCenter, float scale)
{
    if (!texture) return;
    const float pixelScale = 111.0f / PHOTO_CASE_RADIUS * scale;
    const ImVec2 topLeft(watchCenter.x + (region.left - PHOTO_CENTER_X) * pixelScale,
                         watchCenter.y + (region.top - PHOTO_CENTER_Y) * pixelScale);
    const ImVec2 bottomRight(watchCenter.x + (region.right - PHOTO_CENTER_X) * pixelScale,
                              watchCenter.y + (region.bottom - PHOTO_CENTER_Y) * pixelScale);
    draw->AddImage(static_cast<ImTextureID>(texture), topLeft, bottomRight);
}

int fingerFrame(int finger)
{
    if (fingerPressStart[finger] < 0.0) return 0;
    const double age = ImGui::GetTime() - fingerPressStart[finger];
    if (age < 0.0 || age >= FINGER_PRESS_SECONDS) return 0;
    return std::min(FINGER_FRAME_COUNT - 1,
                    static_cast<int>(age / FINGER_PRESS_SECONDS * FINGER_FRAME_COUNT));
}

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

bool buttonHitbox(const char* id, const ImVec2& center, float scale,
                  const char* tooltip)
{
    const ImVec2 hitSize(42.0f * scale, 30.0f * scale);
    ImGui::SetCursorScreenPos(ImVec2(center.x - hitSize.x * 0.5f,
                                     center.y - hitSize.y * 0.5f));
    const bool clicked = ImGui::InvisibleButton(id, hitSize);
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
    return clicked;
}

} // namespace

void initializeStopwatchWidget()
{
    if (!handBack) {
        handBack = loadSOILWithAlpha(ELEMENTAL_ASSET_DIR "/stopwatch_hand_frames/back.png");
        const char* names[3] = {"slower", "pause", "faster"};
        for (int finger = 0; finger < 3; ++finger) {
            for (int frame = 0; frame < FINGER_FRAME_COUNT; ++frame) {
                char path[512];
                std::snprintf(path, sizeof(path), "%s/stopwatch_hand_frames/%s_%d.png",
                              ELEMENTAL_ASSET_DIR, names[finger], frame);
                fingerFrames[finger][frame] = loadSOILWithAlpha(path);
            }
        }
    }
}

void shutdownStopwatchWidget()
{
    if (handBack) glDeleteTextures(1, &handBack);
    handBack = 0;
    for (int finger = 0; finger < 3; ++finger) {
        glDeleteTextures(FINGER_FRAME_COUNT, fingerFrames[finger]);
        for (int frame = 0; frame < FINGER_FRAME_COUNT; ++frame) {
            fingerFrames[finger][frame] = 0;
        }
    }
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
    const float scale = std::max(0.58f, std::min(0.75f, io.DisplaySize.y / 768.0f));
    const ImVec2 size(426.0f * scale, 500.0f * scale);
    const ImVec2 position(std::max(0.0f,
                                   io.DisplaySize.x - size.x + 42.0f * scale),
                          std::max(0.0f, io.DisplaySize.y - size.y));
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
    const ImVec2 faceOrigin(origin.x + 64.0f * scale,
                            origin.y + 100.0f * scale);
    const ImVec2 center(faceOrigin.x + 140.0f * scale,
                        faceOrigin.y + 185.0f * scale);

    drawPhotoRegion(draw, handBack, PhotoRegion{0.0f, 0.0f, 1082.0f, 1454.0f},
                    center, scale);
    for (int finger = 0; finger < 3; ++finger) {
        drawPhotoRegion(draw, fingerFrames[finger][fingerFrame(finger)],
                        fingerRegions[finger], center, scale);
    }

    const bool slower = buttonHitbox(
        "##slower", ImVec2(center.x - 82.0f * scale, faceOrigin.y + 55.0f * scale),
        scale, "Halve simulation speed ([)");
    const bool pause = buttonHitbox(
        "##pause", ImVec2(center.x, faceOrigin.y + 31.0f * scale),
        scale, paused ? "Resume simulation (P)" : "Pause simulation (P)");
    const bool faster = buttonHitbox(
        "##faster", ImVec2(center.x + 75.0f * scale, faceOrigin.y + 55.0f * scale),
        scale, "Double simulation speed (])");

    // Replace only the photo's fixed 60-second face. Its case and pushers stay visible.
    draw->AddCircleFilled(center, 104.0f * scale, IM_COL32(5, 8, 13, 255), 128);

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

    ImGui::End();

    if (slower) return StopwatchAction::Slower;
    if (pause) return StopwatchAction::TogglePause;
    if (faster) return StopwatchAction::Faster;
    return StopwatchAction::None;
}
