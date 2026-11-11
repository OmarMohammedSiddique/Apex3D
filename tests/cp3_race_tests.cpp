#include "../include/apex3d/cp3_hud.hpp"
#include "../include/apex3d/cp3_start_audio.hpp"
#include "../include/apex3d/race_field.hpp"
#include "../include/apex3d/driver_focus.hpp"
#include "../include/apex3d/race_telemetry.hpp"
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
    // The recorded visit spans the timing lines; the measured lane starts before the
    // entry line (after Parabolica) and rejoins after the exit line.
    CHECK(!race.sample(pitEntrySeconds - pitLaneMarginSeconds).inPit);
    CHECK(race.sample(pitEntrySeconds - 0.001).inPit && race.sample(pitEntrySeconds).inPit);
    CHECK(race.sample(pitExitSeconds - 0.001).inPit && race.sample(pitExitSeconds).inPit);
    CHECK(!race.sample(pitExitSeconds + pitLaneMarginSeconds).inPit);
    {
        // Continuous through the whole visit: lane start, timing lines, box, lane end.
        auto previous = race.sample(pitEntrySeconds - pitLaneMarginSeconds);
        bool entered = false, left = false;
        for (double t = pitEntrySeconds - pitLaneMarginSeconds; t < pitExitSeconds + pitLaneMarginSeconds; t += 0.02) {
            const auto s = race.sample(t);
            const double step = std::hypot(s.pose.position.x - previous.pose.position.x, s.pose.position.y - previous.pose.position.y) * apex3d::physical::metresPerSceneUnit();
            CHECK(step < 2.5);
            if (s.inPit && !previous.inPit) { entered = true; CHECK(std::abs(pitLaneRelative(s.progress) - pitLaneFirst) < 0.002); }
            if (!s.inPit && previous.inPit) { left = true; CHECK(std::abs(pitLaneRelative(s.progress) - pitLaneLast) < 0.002); }
            previous = s;
        }
        CHECK(entered && left);
    }
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
        CHECK(sample.travelledMetres>=previous.travelledMetres);
        if(sample.servicing && previous.servicing)CHECK(near(sample.travelledMetres,previous.travelledMetres));
        CHECK(std::isfinite(sample.pose.position.x) && std::isfinite(sample.pose.position.y));
        CHECK(std::hypot(sample.pose.position.x-previous.pose.position.x,
                         sample.pose.position.y-previous.pose.position.y) < 0.25);
        previous = sample;
    }

    const OpponentMotion norris(path,race,norrisSpec),piastri(path,race,piastriSpec);
    for(const auto* driver:{&norris,&piastri}) {
        double elapsed=0;
        for(int i=0;i<53;++i){elapsed+=(*driver->spec().laps)[i].milliseconds()/1000.0;
            CHECK(std::abs(driver->sample(elapsed).progress-(i+1))<1e-6);}
        CHECK(std::abs(elapsed-driver->spec().finish)<1e-6);
        auto previous=driver->sample(0);
        for(double t=0.5;t<=driver->spec().finish;t+=0.5){
            const auto sample=driver->sample(t);CHECK(sample.progress>=previous.progress);
            CHECK(std::hypot(sample.pose.position.x-previous.pose.position.x,sample.pose.position.y-previous.pose.position.y)<0.25);
            previous=sample;
        }
        CHECK(driver->sample(driver->spec().entry).inPit);
        CHECK(driver->sample(driver->spec().boxStart+0.5).servicing);
        CHECK(driver->sample(driver->spec().exit).inPit); // still in the lane until it rejoins
        CHECK(!driver->sample(driver->spec().exit + pitLaneMarginSeconds).inPit);
        CHECK(driver->sample(driver->spec().finish).finished);
    }
    CHECK(race.sample(1).travelledMetres<10); // observed launch, not a sector-average jump
    CHECK(race.sample(2).travelledMetres>race.sample(1).travelledMetres);
    CHECK(near(norrisSpec.finish-raceTotalSeconds,19.207));
    CHECK(near(piastriSpec.finish-raceTotalSeconds,21.351));
    CHECK(norris.sample(0).progress<race.sample(0).progress);
    CHECK(piastri.sample(0).progress<norris.sample(0).progress);
    for(const auto& point:verstappenTelemetry){
        const auto read=telemetryAt(point.raceMs/1000.0+0.000001);
        CHECK(read.available&&read.speedKmh==point.speedKmh&&read.gear==point.gear&&read.engineRpm==point.engineRpm);
    }
    for(std::size_t i=1;i<verstappenTelemetry.size();++i) {
        const auto& a=verstappenTelemetry[i-1];const auto& b=verstappenTelemetry[i];
        if(b.raceMs-a.raceMs>1100)CHECK(!telemetryAt(a.raceMs/1000.0+1.01).available);
    }
    CHECK(!telemetryAt(-100).available&&!telemetryAt(5000).available);
    CHECK(!telemetryAt(std::numeric_limits<double>::quiet_NaN()).available);
    CHECK(telemetryAt(3088).available&&telemetryAt(3088).speedKmh==0&&telemetryAt(3088).engineRpm>0);

    for(const auto& info:podiumDrivers) {
        for(const auto& point:recordedSeries(static_cast<int>(info.focus))) {
            const auto read=telemetryAt(point.raceMs/1000.0+0.000001,static_cast<int>(info.focus));
            CHECK(read.available&&read.speedKmh==point.speedKmh&&read.gear==point.gear&&read.engineRpm==point.engineRpm);
        }
        int boundary=0;
        for(int lap=0;lap<53;++lap){boundary+=(*info.laps)[lap].milliseconds();
            CHECK(driverClock(info.focus,boundary/1000.0).completed==lap+1);}
        CHECK(near(nextDriverLap(info.focus,0),(*info.laps)[0].milliseconds()/1000.0));
    }
    CHECK(driverClock(DriverFocus::Norris,87.5).lap==2);
    CHECK(driverClock(DriverFocus::Piastri,87.5).lap==1);
    const auto selected=focusedSample(DriverFocus::Norris,race.sample(120),norris.sample(120),piastri.sample(120));
    CHECK(near(selected.pose.position.x,norris.sample(120).pose.position.x));
    CHECK(selected.velocity>0);
    CHECK(focusedSample(DriverFocus::Piastri,race.sample(piastriSpec.boxStart+0.5),norris.sample(piastriSpec.boxStart+0.5),
        piastri.sample(piastriSpec.boxStart+0.5)).velocity==0);

    for(const auto& info:podiumDrivers) {
        const auto start=tyreState(info.focus,0);
        CHECK(start.stint->compound=="MEDIUM"&&start.completedLaps==0&&start.priorLaps==0);
        const auto before=tyreState(info.focus,info.boxStart+info.service-0.001);
        CHECK(before.stint->number==1&&before.changing&&before.completedLaps==info.pitLap);
        const auto fitted=tyreState(info.focus,info.boxStart+info.service);
        CHECK(fitted.stint->number==2&&!fitted.changing&&fitted.raceLaps==0);
        CHECK(fitted.stint->compound==(info.focus==DriverFocus::Verstappen?"HARD":"SOFT"));
        CHECK(fitted.priorLaps==(info.focus==DriverFocus::Verstappen?0:info.focus==DriverFocus::Norris?2:3));
        CHECK(fitted.completedLaps==fitted.priorLaps);
        int lapEnd=0;for(int i=0;i<info.pitLap+1;++i)lapEnd+=(*info.laps)[i].milliseconds();
        const auto next=tyreState(info.focus,lapEnd/1000.0);
        CHECK(next.raceLaps==1&&next.completedLaps==fitted.priorLaps+1);
        CHECK(tyreState(info.focus,info.finish).completedLaps==53-info.pitLap+fitted.priorLaps);
        CHECK(tyreState(info.focus,0).stint->number==1); // seek/restart restores the original set
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
