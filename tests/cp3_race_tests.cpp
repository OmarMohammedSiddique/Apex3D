#include "../include/apex3d/cp3_hud.hpp"
#include "../include/apex3d/cp3_start_audio.hpp"
#include <iostream>
#include <stdexcept>

#define CHECK(expression) do { if (!(expression)) throw std::runtime_error("Check failed: " #expression); } while (false)

int main()
{
    using namespace apex3d::cp3;
    const auto near = [](double a, double b) { return std::abs(a-b) < 1e-7; };
    const auto path = makePath();
    const RaceMotion race(path);
    CHECK(raceLaps.size() == 53);
    CHECK(raceBoundariesMs.back() == 4404325);
    CHECK(raceLaps[0].milliseconds() == 87159);
    CHECK(raceLaps[36].milliseconds() == 87432);
    CHECK(raceLaps[37].milliseconds() == 102349);
    CHECK(clockText(87.159) == "01:27.159");
    CHECK(clockText(raceTotalSeconds, true) == "01:13:24.325");
    CHECK(clockText(0.0, true) == "00:00:00.000");
    for (std::size_t i = 0; i < raceLaps.size(); ++i) {
        const double boundary = raceBoundariesMs[i+1] / 1000.0;
        CHECK(raceClock(boundary - 0.0001).lap == static_cast<int>(i+1));
        CHECK(raceClock(boundary).completed == static_cast<int>(i+1));
        CHECK(raceClock(boundary).lap <= 53);
        CHECK(near(race.sample(boundary).progress, static_cast<double>(i+1)));
        CHECK(near(raceClock(boundary).lastLap, raceLaps[i].milliseconds()/1000.0));
        const double s1Time = (raceBoundariesMs[i] + raceLaps[i].s1) / 1000.0;
        const double s2Time = s1Time + raceLaps[i].s2 / 1000.0;
        CHECK(race.sample(s1Time).progress < race.sample(s2Time).progress);
    }
    CHECK(near(race.sample(0.0).progress, gridOffset));
    CHECK(near(raceWheelTurns(gridOffset), 0.0));
    CHECK(near(raceWheelTurns(gridOffset + raceTyreCircumference/circuitMetres), 1.0));
    CHECK(near(raceWheelRpm(path.back().distance * raceTyreCircumference/circuitMetres,
        path.back().distance), 60.0));
    CHECK(raceWheelRpm(0.0, path.back().distance) == 0.0);
    CHECK(near(pitExitSeconds - pitEntrySeconds, 24.545));
    CHECK(near(pitBoxEndSeconds - pitBoxStartSeconds, 2.3));
    CHECK(!race.sample(pitEntrySeconds - 0.001).inPit);
    CHECK(race.sample(pitEntrySeconds).inPit);
    CHECK(race.sample(pitExitSeconds - 0.001).inPit);
    CHECK(!race.sample(pitExitSeconds).inPit);
    CHECK(race.sample(pitBoxStartSeconds + 1.0).servicing);
    CHECK(race.sample(pitBoxStartSeconds + 1.0).velocity == 0.0);
    CHECK(near(race.sample(pitBoxStartSeconds).progress, race.sample(pitBoxEndSeconds - 1e-6).progress));
    for (double gate : {pitEntrySeconds, pitExitSeconds, pitBoxStartSeconds, pitBoxEndSeconds}) {
        const auto before = race.sample(gate - 1e-5).pose.position;
        const auto after = race.sample(gate + 1e-5).pose.position;
        CHECK(std::hypot(after.x-before.x, after.y-before.y) < 0.001);
    }
    // Every half second through the complete race: no backwards travel or
    // position discontinuity, including both timing lines inside the pit visit.
    auto previous = race.sample(0.0);
    for (double time = 0.5; time < raceTotalSeconds; time += 0.5) {
        const auto sample = race.sample(time);
        CHECK(sample.progress >= previous.progress);
        CHECK(sample.velocity >= 0.0);
        CHECK(std::isfinite(sample.pose.position.x) && std::isfinite(sample.pose.position.y));
        CHECK(std::hypot(sample.pose.position.x-previous.pose.position.x,
                         sample.pose.position.y-previous.pose.position.y) < 0.25);
        previous = sample;
    }

    Playback playback;
    CHECK(!playback.playing && playback.muted && playback.elapsed == 0.0);
    playback.advance(100.0, path.back().distance);
    CHECK(playback.elapsed == 0.0 && playback.redLights() == 0);
    playback.togglePlaying();
    for (int i = 1; i <= 5; ++i) {
        playback.advance(1.0, path.back().distance);
        CHECK(playback.starting() && playback.redLights() == i);
        CHECK(playback.elapsed == 0.0);
    }
    playback.togglePlaying();
    playback.advance(10.0, path.back().distance);
    CHECK(playback.redLights() == 5 && playback.elapsed == 0.0);
    playback.togglePlaying();
    playback.advance(1.5, path.back().distance);
    CHECK(!playback.starting() && playback.redLights() == 0 && playback.elapsed == 0.0);
    playback.advance(87.159, path.back().distance);
    CHECK(raceClock(playback.elapsed).lap == 2 && near(raceClock(playback.elapsed).lapElapsed, 0.0));
    playback.togglePlaying();
    playback.advance(50.0, path.back().distance);
    CHECK(near(playback.elapsed, 87.159));
    playback.togglePlaying();
    playback.speed = 64.0;
    while (playback.playing) playback.advance(1.0/60.0, path.back().distance);
    CHECK(playback.finished() && playback.elapsed == raceTotalSeconds);
    CHECK(raceClock(playback.elapsed).completed == 53 && raceClock(playback.elapsed).lap == 53);
    playback.advance(1000.0, path.back().distance);
    CHECK(playback.elapsed == raceTotalSeconds);
    playback.togglePlaying();
    CHECK(playback.playing && playback.elapsed == 0.0 && playback.startElapsed == 0.0);
    CHECK(playback.muted);
    playback.advance(7.5, path.back().distance);
    CHECK(near(playback.elapsed, 64.0)); // countdown is real-time even at 64x
    playback.seek(pitBoxStartSeconds + 0.2);
    CHECK(race.sample(playback.elapsed).servicing);
    playback.seek(-50.0);
    CHECK(playback.elapsed == 0.0);
    playback.seek(1e6);
    CHECK(playback.finished() && !playback.playing);

    CHECK(hudActionAt(0.3, -0.3) == HudAction::Play);
    CHECK(hudActionAt(0.7, -0.3) == HudAction::Mute);
    CHECK(hudActionAt(0.7, 0.0) == HudAction::None);
    for (double time : {0.0, 87.159, pitEntrySeconds+1.0, pitBoxStartSeconds+0.1, pitExitSeconds, raceTotalSeconds}) {
        playback.seek(time);
        playback.showWheels = true;
        const auto mesh = makeRaceHudMesh(playback, race.sample(time), path.back().distance, 16.0/9.0, true);
        CHECK(!mesh.empty() && mesh.size()%3 == 0);
        for (const auto& vertex : mesh) {
            CHECK(std::isfinite(vertex.x) && std::isfinite(vertex.y));
            CHECK(vertex.x >= -1.0F && vertex.x <= 1.0F);
            CHECK(vertex.y >= -1.0F && vertex.y <= 1.0F);
        }
    }
    apex3d::audio::CueMixer mixer;
    std::vector<float> audio(24000);
    mixer.trigger(2);
    mixer.render(audio.data(), audio.size(), true);
    CHECK(std::all_of(audio.begin(), audio.end(), [](float x) { return x == 0.0F; }));
    for (int cue : {1, 2}) {
        mixer.trigger(cue);
        mixer.render(audio.data(), audio.size(), false);
        double energy = 0.0;
        for (float value : audio) { CHECK(std::isfinite(value) && std::abs(value) <= 0.22F); energy += value*value; }
        CHECK(energy > 0.01 && audio.back() == 0.0F);
        mixer.render(audio.data(), audio.size(), false);
        CHECK(std::all_of(audio.begin(), audio.end(), [](float x) { return x == 0.0F; }));
    }
    std::cout << "Monza: 53 exact laps, sectors, 2.3s service, 24.545s pit visit, lights, mute and finish: PASS\n";
}
