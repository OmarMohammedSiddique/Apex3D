#pragma once

#include "cp3_scene.hpp"
#include "cp3_playback.hpp"
#include "driver_focus.hpp"
#include "tyre_state.hpp"
#include "../../checkpoint2/animation/wheels/include/wheel_animation.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace apex3d::cp3 {

[[nodiscard]] inline std::array<unsigned char, 7> glyph(char letter)
{
    switch (letter) {
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {15, 16, 16, 16, 16, 16, 15};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {15, 16, 16, 19, 17, 17, 15};
    case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'I': return {31, 4, 4, 4, 4, 4, 31};
    case 'J': return {7, 2, 2, 2, 18, 18, 12};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'Q': return {14, 17, 17, 17, 21, 18, 13};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10};
    case 'X': return {17, 17, 10, 4, 10, 17, 17};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4};
    case 'Z': return {31, 1, 2, 4, 8, 16, 31};
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case '/': return {1, 1, 2, 4, 8, 16, 16};
    case '+': return {0, 4, 4, 31, 4, 4, 0};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    case '.': return {0, 0, 0, 0, 0, 12, 12};
    case ':': return {0, 12, 12, 0, 12, 12, 0};
    case '<': return {1, 2, 4, 8, 4, 2, 1};
    case '>': return {16, 8, 4, 2, 4, 8, 16};
    case '%': return {17, 2, 4, 4, 8, 16, 17};
    default: return {0, 0, 0, 0, 0, 0, 0};
    }
}

inline void hudText(std::vector<Vertex>& mesh, double x, double top,
                    double pixel, const std::string& text, Color color)
{
    for (char letter : text) {
        const auto rows = glyph(letter);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((rows[static_cast<std::size_t>(row)] & (1 << (4 - column))) == 0) {
                    continue;
                }
                const double left = x + column * pixel;
                const double high = top - row * pixel;
                quad(mesh, {left, high}, {left + pixel, high},
                     {left + pixel, high - pixel}, {left, high - pixel}, color);
            }
        }
        x += 6 * pixel;
    }
}

inline void hudRectangle(std::vector<Vertex>& mesh, double left, double bottom,
                         double right, double top, Color color)
{
    quad(mesh, {left, bottom}, {right, bottom},
         {right, top}, {left, top}, color);
}

// Use the same model-to-scene scale as the track car. These are simulated
// rolling speeds, not measured telemetry or an assumed real-world lap length.
inline constexpr double wheelRadius = wheels::kTyreRadius * carModelScale;
inline constexpr double wheelCircumference = 2.0 * std::numbers::pi * wheelRadius;

[[nodiscard]] inline double wheelTurnsForDistance(double travelled)
{
    return travelled / wheelCircumference;
}

[[nodiscard]] inline double wheelRpm(double sceneUnitsPerSecond)
{
    return sceneUnitsPerSecond / wheelCircumference * 60.0;
}

inline void wheelInspector(std::vector<Vertex>& mesh, double travelled, double aspect, double rpm,
    Color bodyColor = {0.85F, 0.17F, 0.17F},const TyreState* tyre = nullptr)
{
    constexpr Color panel{0.035F, 0.075F, 0.11F};
    constexpr Color white{0.94F, 0.97F, 0.97F};
    constexpr Color secondary{0.55F, 0.68F, 0.72F};
    constexpr Color cyan{0.22F, 0.86F, 0.91F};
        hudRectangle(mesh, 0.17, -0.20, 0.95, tyre?0.34:0.20, panel);
        hudText(mesh, 0.21, tyre?0.30:0.17, 0.0042, "WHEEL INSPECTOR", white);
        if(tyre) {
            hudText(mesh,0.21,0.245,0.0038,"TYRE: "+tyreCompoundText(tyre->stint->compound)+" / SET "+std::to_string(tyre->stint->number),compoundColor(tyre->stint->compound));
            hudText(mesh,0.21,0.195,0.0033,"AGE: "+std::to_string(tyre->completedLaps)+" COMPLETED LAPS",white);
            hudText(mesh,0.21,0.15,0.0030,tyre->changing?"TYRE CHANGE IN PROGRESS":
                std::to_string(tyre->raceLaps)+" RACE + "+std::to_string(tyre->priorLaps)+" PRIOR LAPS",secondary);
        }
        hudText(mesh, 0.21, 0.12, 0.0033, "FIXED BODY - LOCAL WHEEL ROTATION", secondary);
        // Wrap only the spin angle. The fixed body never jumps when a tyre
        // completes a revolution; its rotation follows distance = radius * angle.
        const double degrees = std::fmod(wheelTurnsForDistance(travelled), 1.0) * 360.0;
        // Cap the inset aspect correction on very wide windows so the car
        // stays inside its panel; ordinary 16:9 wheels remain circular.
        const double wheelAspect = std::clamp(aspect, 0.5, 1.8);
        constexpr double scale = 0.0008;
        const Mat3 view = Mat3::translation(0.46, -0.07) *
            Mat3::scaling(scale, -scale * wheelAspect);
        const Mat3 body = view;
        const wheels::CarModel model;
        triangulate(mesh, transformPolygon(model.chassis, body), bodyColor);
        triangulate(mesh, transformPolygon(model.rearWingPillar, body), secondary);
        triangulate(mesh, transformPolygon(model.rearWing, body), secondary);
        triangulate(mesh, transformPolygon(model.frontWing, body), secondary);
        hudRectangle(mesh, 0.21, -0.125, 0.91, -0.121, secondary);
        const auto wheelDisk = [&](Vec2 centre, double radius, Color fill) {
            const std::size_t begin = mesh.size();
            disk(mesh, centre, radius, fill);
            for (std::size_t i = begin; i < mesh.size(); ++i) {
                mesh[i].y = static_cast<float>(centre.y + (mesh[i].y - centre.y) * wheelAspect);
            }
        };
        for (const auto& wheel : {model.rearWheel, model.frontWheel}) {
            const auto pose = wheels::poseWheel(wheel, body, degrees);
            wheelDisk(pose.centre, wheel.tyreRadius * scale, {0.012F, 0.018F, 0.025F});
            wheelDisk(pose.centre, wheel.rimRadius * scale, secondary);
            wheelDisk(pose.centre, 0.0056, white);
            wheelDisk(pose.marker, 0.0032, tyre?compoundColor(tyre->stint->compound):Color{1.0F, 0.8F, 0.25F});
            for (std::size_t i = 0; i < pose.spokeOuter.size(); ++i) {
                const Vec2 a = pose.spokeInner[i];
                const Vec2 b = pose.spokeOuter[i];
                const Vec2 normal{-(b.y - a.y) * 0.10, (b.x - a.x) * 0.10};
                quad(mesh, {a.x + normal.x, a.y + normal.y},
                     {b.x + normal.x, b.y + normal.y},
                     {b.x - normal.x, b.y - normal.y},
                     {a.x - normal.x, a.y - normal.y}, cyan);
            }
        }
        std::ostringstream wheelSpeed;
        wheelSpeed << "WHEEL SPEED " << std::fixed << std::setprecision(0) << rpm << " RPM";
        hudText(mesh, 0.21, -0.16, 0.0033, wheelSpeed.str(), secondary);
}

[[nodiscard]] inline std::vector<Vertex> makeHudMesh(double progress,
    double speed, bool playing, bool scaleDemo,
    Route route = Route::Monza, bool showWheels = false, double travelled = 0.0,
    double aspect = 16.0 / 9.0, bool looping = false, double rpm = 0.0)
{
    constexpr Color panel{0.035F, 0.075F, 0.11F};
    constexpr Color panelAccent{0.055F, 0.16F, 0.20F};
    constexpr Color cyan{0.22F, 0.86F, 0.91F};
    constexpr Color white{0.94F, 0.97F, 0.97F};
    constexpr Color secondary{0.55F, 0.68F, 0.72F};
    std::vector<Vertex> mesh;
    hudRectangle(mesh, 0.17, 0.28, 0.95, 0.94, panel);
    hudRectangle(mesh, 0.17, 0.927, 0.95, 0.94, cyan);
    hudText(mesh, 0.21, 0.88, 0.009, "APEX3D", white);
    hudText(mesh, 0.21, 0.78, 0.0044, routeName(route), secondary);

    std::ostringstream lap;
    lap << "LAP " << std::setfill('0') << std::setw(2)
        << static_cast<int>(std::clamp(progress, 0.0, 1.0) * 100.0)
        << "%  " << std::fixed << std::setprecision(2) << speed << "X";
    hudText(mesh, 0.21, 0.70, 0.0062, lap.str(), white);
    hudRectangle(mesh, 0.21, 0.605, 0.91, 0.638, panelAccent);
    hudRectangle(mesh, 0.21, 0.605,
                 0.21 + 0.70 * std::clamp(progress, 0.0, 1.0), 0.638, cyan);
    hudText(mesh, 0.21, 0.57, 0.0046,
            progress >= 1.0 ? "LAP COMPLETE" : playing ? "PLAYING" : "PAUSED", cyan);
    hudText(mesh, 0.21, 0.50, 0.0039, progress >= 1.0 ? "SPACE REPLAY  R RESTART" : "SPACE PAUSE  R RESTART", secondary);
    hudText(mesh, 0.21, 0.46, 0.0039,
            scaleDemo ? "+/- SPEED  T SCALE ON" : "+/- SPEED  T SCALE OFF", secondary);
    hudText(mesh, 0.21, 0.42, 0.0039, "P POINTS  B ROUTE  ESC QUIT", secondary);
    hudText(mesh, 0.21, 0.38, 0.0039,
            showWheels ? "W WHEEL VIEW ON" : "W WHEEL VIEW OFF", secondary);
    hudText(mesh, 0.21, 0.34, 0.0039,
            looping ? "L LOOP ON" : "L LOOP OFF - SINGLE LAP", secondary);
    if (showWheels) {
        wheelInspector(mesh, travelled, aspect, rpm);
    }
    return mesh;
}

[[nodiscard]] inline std::string clockText(double seconds, bool hours = false)
{
    const auto ms = static_cast<long long>(std::max(0.0, seconds) * 1000.0 + 1e-5);
    std::ostringstream out;
    out << std::setfill('0');
    if (hours) out << std::setw(2) << ms / 3600000 << ':';
    out << std::setw(2) << (hours ? ms / 60000 % 60 : ms / 60000) << ':'
        << std::setw(2) << ms / 1000 % 60 << '.' << std::setw(3) << ms % 1000;
    return out.str();
}

enum class HudAction { None, Play, Mute, FocusVerstappen, FocusNorris, FocusPiastri };
[[nodiscard]] inline HudAction hudActionAt(double x, double y)
{
    if (y < -0.34 || y > -0.26) return HudAction::None;
    if (x >= 0.21 && x <= 0.54) return HudAction::Play;
    if (x >= 0.58 && x <= 0.91) return HudAction::Mute;
    return HudAction::None;
}

[[nodiscard]] inline std::vector<Vertex> makeRaceHudMesh(const Playback& playback,
    const RaceSample& sample, double pathLength, double aspect, bool audioAvailable,
    HudAction hover = HudAction::None, DriverFocus focus = DriverFocus::Verstappen)
{
    constexpr Color panel{0.035F, 0.075F, 0.11F};
    constexpr Color white{0.94F, 0.97F, 0.97F};
    constexpr Color secondary{0.55F, 0.68F, 0.72F};
    constexpr Color cyan{0.22F, 0.86F, 0.91F};
    constexpr Color gold{0.96F, 0.74F, 0.28F};
    const auto& info=driverInfo(focus);
    const auto clock = driverClock(focus,playback.elapsed);
    const auto tyres=tyreState(focus,playback.elapsed);
    std::vector<Vertex> mesh;
    hudRectangle(mesh, 0.17, -0.45, 0.95, 0.94, panel);
    hudText(mesh, 0.21, 0.88, 0.008, "APEX3D", white);
    hudText(mesh, 0.21, 0.77, 0.0038, std::string("MONZA 2025 / ")+info.name, secondary);
    hudText(mesh, 0.21, 0.71, 0.0036, "TIMING-BASED RACE REPLAY", secondary);
    hudText(mesh, 0.21, 0.63, 0.0065, "LAP " + std::to_string(clock.lap) + " OF 53", white);
    hudText(mesh, 0.21, 0.53, 0.0038, "RACE TIME", secondary);
    hudText(mesh, 0.21, 0.47, 0.006, clockText(playback.elapsed, true), white);
    hudText(mesh, 0.21, 0.37, 0.0042, "LAP  " + clockText(clock.lapElapsed), cyan);
    hudText(mesh, 0.21, 0.31, 0.0038,
        clock.completed ? "LAST " + clockText(clock.lastLap) : "LAST --:--.---", secondary);
    std::ostringstream rate;
    rate << std::fixed << std::setprecision(2) << playback.speed << "X  "
        << (playback.speed == 1.0 ? "REAL TIME" : "FAST/SLOW PLAYBACK");
    hudText(mesh, 0.21, 0.25, 0.0035, rate.str(), secondary);
    const char* state = playback.finished()&&!clock.finished ? "WINNER FINISHED / REPLAY ENDED" : clock.finished ? "CHEQUERED FLAG / FINISHED" : !playback.started ? "READY TO START" :
        playback.starting() ? (playback.playing ? "START SEQUENCE" : "START / PAUSED") :
        sample.servicing ? (playback.playing ? "PIT BOX / TYRE CHANGE" : "PIT BOX / PAUSED") :
        sample.inPit ? (playback.playing ? "IN PIT LANE" : "PIT LANE / PAUSED") :
        playback.playing ? "ON TRACK" : "PAUSED";
    hudText(mesh, 0.21, 0.19, 0.0040, state, sample.inPit ? gold : cyan);
    const auto durationText=[](double seconds){std::ostringstream out;out<<std::fixed<<std::setprecision(3)<<seconds<<"S";return out.str();};
    const std::string duration=durationText(info.pitExit-info.pitEntry),service=durationText(info.service);
    if (sample.inPit) {
        hudText(mesh, 0.21, 0.12, 0.0033, "PIT " + clockText(playback.elapsed-info.pitEntry) + " / "+duration, gold);
    } else {
        hudText(mesh, 0.21, 0.12, 0.0035,
            playback.elapsed >= info.pitExit ? "PIT STOP COMPLETE / "+duration :
                "PIT 1 OF 1 / LAP "+std::to_string(info.pitLap)+" / "+duration, secondary);
    }
    hudText(mesh, 0.21, 0.07, 0.0033, sample.servicing ?
        "BOX " + clockText(playback.elapsed-info.boxStart) + " / "+service :
        std::string(tyres.stint->compound)+" TYRES / BOX "+service, secondary);
    hudRectangle(mesh, 0.21, -0.025, 0.91, -0.005, {0.08F, 0.16F, 0.20F});
    hudRectangle(mesh, 0.21, -0.025, 0.21 + 0.70 * playback.elapsed / raceTotalSeconds, -0.005, cyan);
    hudText(mesh, 0.21, -0.07, 0.0035, "+/- SPEED  1 REAL TIME", secondary);
    hudText(mesh, 0.21, -0.12, 0.0035, "< > SEEK 10S  N NEXT LAP", secondary);
    hudText(mesh, 0.21, -0.17, 0.0035, "J PIT STOP  R RESTART  B DEMO", secondary);
    hudText(mesh, 0.21, -0.22, 0.0035, "W WHEELS  P POINTS  M MUTE", secondary);
    const auto button = [&](double left, double right, const std::string& label, HudAction action) {
        hudRectangle(mesh, left, -0.34, right, -0.26,
            hover == action ? Color{0.13F, 0.32F, 0.36F} : Color{0.08F, 0.20F, 0.24F});
        hudText(mesh, left + 0.022, -0.282, 0.0041, label, white);
    };
    button(0.21, 0.54, playback.finished() ? "REPLAY" : !playback.started ? "START" : playback.playing ? "PAUSE" : "RESUME", HudAction::Play);
    button(0.58, 0.91, !audioAvailable ? "NO AUDIO" : playback.muted ? "UNMUTE" : "MUTE", HudAction::Mute);
    hudText(mesh, 0.21, -0.40, 0.0032, "PATH / PIT ARRIVAL INTERPOLATED", secondary);

    // Five double red lamps; the start is signalled by all lamps going out.
    hudRectangle(mesh, -0.96, 0.60, -0.17, 0.95, panel);
    for (int i = 0; i < 5; ++i) {
        const Color color = i < playback.redLights() ? Color{0.96F, 0.08F, 0.10F} : Color{0.18F, 0.07F, 0.08F};
        for (double y : {0.85, 0.76}) {
            const Vec2 centre{-0.87 + i * 0.145, y};
            const auto first = mesh.size();
            disk(mesh, centre, 0.026, color);
            for (auto j = first; j < mesh.size(); ++j)
                mesh[j].y = static_cast<float>(centre.y + (mesh[j].y-centre.y) * aspect);
        }
    }
    hudText(mesh, -0.90, 0.66, 0.0040,
        !playback.started ? "SPACE OR START TO BEGIN" : playback.starting() ? "WAIT FOR LIGHTS OUT" : "LIGHTS OUT / RACE STARTED", white);
    if (playback.showWheels) {
        const auto first = mesh.size();
        const double travel = sample.travelledMetres / raceTyreCircumference * wheelCircumference;
        wheelInspector(mesh, travel, aspect,
            playback.playing && !playback.starting() ?
                raceWheelRpm(sample.velocity * playback.speed, pathLength) : 0.0, {0.16F, 0.28F, 0.56F});
        for (auto i = first; i < mesh.size(); ++i) mesh[i].y -= 0.72F;
    }
    return mesh;
}

} // namespace apex3d::cp3
