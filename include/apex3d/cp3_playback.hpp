#pragma once

#include "cp3_race.hpp"
#include "post_race.hpp"

namespace apex3d::cp3 {

inline constexpr double lapSeconds = 18.0; // preserved CP2 Bezier demonstration

struct Playback {
    double progress{};
    double speed{1.0};
    bool playing{false};
    bool looping{false}; // Bezier demo only; Monza always finishes at lap 53
    bool showControls{false};
    bool scaleDemo{false};
    bool showWheels{false};
    bool muted{true};
    bool started{false};
    Route route{Route::Monza};
    double travelled{};
    double elapsed{};
    double startElapsed{};
    double postRaceElapsed{};
    double simulationSeconds()const{return elapsed+postRaceElapsed;}

    [[nodiscard]] bool isRace() const { return route == Route::Monza; }
    [[nodiscard]] bool finished() const { return isRace() ? elapsed >= raceTotalSeconds : progress >= 1.0; }
    [[nodiscard]] bool starting() const { return isRace() && startElapsed < startSequenceSeconds; }
    [[nodiscard]] int redLights() const { return starting() ? std::clamp(static_cast<int>(startElapsed), 0, 5) : 0; }

    void restart()
    {
        progress = 0.0;
        travelled = 0.0;
        elapsed = 0.0;
        startElapsed = 0.0;
        postRaceElapsed=0;
        started = true;
        playing = true;
    }

    void togglePlaying()
    {
        if (finished()&&(!isRace()||postRaceElapsed>=postRaceDuration)) {
            restart();
        } else {
            started = true;
            playing = !playing;
        }
    }

    void seek(double seconds)
    {
        if (!isRace()) return;
        elapsed = std::clamp(seconds, 0.0, raceTotalSeconds);
        postRaceElapsed=0;
        startElapsed = startSequenceSeconds;
        started = true;
        const auto clock = raceClock(elapsed);
        progress = clock.lapElapsed / (raceLaps[static_cast<std::size_t>(clock.lap-1)].milliseconds() / 1000.0);
        if (finished()) playing = false;
    }

    bool advance(double seconds, double pathLength)
    {
        if (!playing) return false;
        double delta = std::max(0.0, seconds);
        if (isRace()) {
            if (starting()) {
                const double used = std::min(delta, startSequenceSeconds - startElapsed);
                startElapsed += used; // start sequence always takes real time
                delta -= used;
                if (starting()) return false;
            }
            if(finished()){
                postRaceElapsed=std::min(postRaceDuration,postRaceElapsed+delta*speed);
                if(postRaceElapsed>=postRaceDuration)playing=false;
                return false;
            }
            const double target=elapsed+delta*speed;
            seek(target);
            if(finished()){
                postRaceElapsed=std::min(postRaceDuration,std::max(0.0,target-raceTotalSeconds));
                playing=postRaceElapsed<postRaceDuration;
                return true;
            }
            return false;
        }
        double step = delta * speed / lapSeconds;
        if (!looping) step = std::min(step, 1.0 - progress);
        travelled += step * pathLength;
        elapsed += delta * speed;
        progress += step;
        if (progress < 1.0) return false;
        if (looping) progress -= std::floor(progress);
        else { progress = 1.0; playing = false; }
        return true;
    }
};

} // namespace apex3d::cp3
