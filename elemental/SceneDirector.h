#ifndef SCENE_DIRECTOR_H
#define SCENE_DIRECTOR_H

#include <glm/glm.hpp>

class Camera;
class Light;
class Volcano;
class Skybox;

// Stages drive the sequence of scene effects.
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

    // A zero time scale freezes scene effects while the camera remains movable.
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

    // Effects use this clock so pause and time scale apply consistently.
    double m_simElapsedSeconds;
    float m_timeScale;
    SimulationStage m_stage;
    double m_stageStartSeconds;
};

#endif
