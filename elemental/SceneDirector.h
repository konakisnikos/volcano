#ifndef SCENE_DIRECTOR_H
#define SCENE_DIRECTOR_H

#include <glm/glm.hpp>

class Camera;
class Light;
class Volcano;
class Skybox;

class SceneDirector {
public:
    SceneDirector(Camera* camera, Light* moonlight, Volcano* volcano, Skybox* skybox);

    void update(double timeSeconds, float deltaSeconds);
    void reset(double timeSeconds);
    void setPaused(bool paused);
    bool isPaused() const;
    float getLavaTimeSeconds() const;

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
};

#endif
