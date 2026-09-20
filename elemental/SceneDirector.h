#ifndef SCENE_DIRECTOR_H
#define SCENE_DIRECTOR_H

#include <glm/glm.hpp>

class Camera;
class Light;
class Volcano;
class Skybox;

// The domino sequence is deliberately explicit: the UI can display it and the
// render loop can trigger each effect from a state transition instead of from
// unrelated wall-clock animations.
enum class SimulationStage {
    Awakening,
    LavaFlowing,
    SmokeAndAsh,
    CloudFormation,
    Raining,
    RiverFilling,
    VegetationGrowing,
    LightningStorm,
    ElectrifiedRiver,
    CalmNight,
    Complete
};

class SceneDirector {
public:
    SceneDirector(Camera* camera, Light* moonlight, Volcano* volcano, Skybox* skybox);

    void update(double timeSeconds, float deltaSeconds);
    void reset(double timeSeconds);
    void setPaused(bool paused);
    bool isPaused() const;
    float getLavaTimeSeconds() const;

    // Time controller (Part B.5): scales how fast simulated time advances.
    // 0 effectively freezes the domino chain while still allowing free camera look.
    void setTimeScale(float scale);
    float getTimeScale() const;
    double getSimSeconds() const;
    float getScaledDeltaSeconds(float realDeltaSeconds) const;

    void transitionTo(SimulationStage stage);
    SimulationStage getStage() const;
    const char* getStageName() const;
    float getStageElapsedSeconds() const;

private:
    Camera* m_camera;
    Light* m_moonlight;
    Volcano* m_volcano;
    Skybox* m_skybox;
    double m_startTime;
    double m_lastTime;
    bool m_paused;
    double m_shakeStartSeconds;
    double m_lavaStartSeconds;
    float m_lavaTimeSeconds;
    glm::vec3 m_lastShakeOffset;
    bool m_hasShakeOffset;

    // Simulated clock: advances by deltaSeconds * m_timeScale each frame (frozen when
    // paused), independent of wall-clock time. Everything in the domino chain reads
    // from this instead of glfwGetTime() so the time controller can speed it up/slow
    // it down/pause it.
    double m_simElapsedSeconds;
    float m_timeScale;
    SimulationStage m_stage;
    double m_stageStartSeconds;
};

#endif
