#pragma once

class Timer
{
private:
    unsigned long lastChecked;
    unsigned long checkInterval;

public:
    Timer(unsigned long interval, bool runOnStart = true);
    bool isIntervalPassed();
};
