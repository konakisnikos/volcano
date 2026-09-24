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

// Draws the simulation stopwatch and returns the button pressed this frame.
StopwatchAction drawStopwatch(double elapsedSeconds, float timeScale, bool paused);

#endif
