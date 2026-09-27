#ifndef STOPWATCH_WIDGET_H
#define STOPWATCH_WIDGET_H

enum class StopwatchAction {
    None,
    Slower,
    TogglePause,
    Faster
};

void initializeStopwatchWidget();
void shutdownStopwatchWidget();
void triggerStopwatchFinger(StopwatchAction action);

// Reveal ranges from 0 (off screen) to 1 (resting position).
StopwatchAction drawStopwatch(double elapsedSeconds, float timeScale, bool paused,
                             float reveal, bool interactive);

#endif
