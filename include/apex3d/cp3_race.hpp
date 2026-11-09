#pragma once

#include "cp3_scene.hpp"
#include "physical_scale.hpp"
#include "recorded_motion.hpp"
#include "generated/monza_2025.hpp"

namespace apex3d::cp3 {

struct RaceClock {
    int lap;
    int completed;
    double lapElapsed;
    double lastLap;
    bool finished;
};

[[nodiscard]] inline RaceClock raceClock(double seconds)
{
    const double ms = std::clamp(seconds, 0.0, raceTotalSeconds) * 1000.0;
    const auto upper = std::upper_bound(raceBoundariesMs.begin(), raceBoundariesMs.end(), ms + 1e-6);
    const int completed = std::min(53, static_cast<int>(upper - raceBoundariesMs.begin()) - 1);
    const int index = std::min(completed, 52);
    return {index + 1, completed, (ms - raceBoundariesMs[index]) / 1000.0,
        completed > 0 ? raceLaps[static_cast<std::size_t>(completed - 1)].milliseconds() / 1000.0 : 0.0,
        completed == 53};
}

// OpenF1 pit completion minus its 24.545 s duration, aligned to FIA lap
// boundaries. Approximate UTC feed timestamps limit sub-second accuracy.
inline constexpr double pitEntrySeconds = 3077.895;
inline constexpr double pitExitSeconds = pitEntrySeconds + pitLaneSeconds;
inline constexpr double pitBoxStartSeconds = 3086.597; // first stationary sample bracket; +/- 0.2 s
inline constexpr double pitBoxEndSeconds = pitBoxStartSeconds + pitServiceSeconds;
inline constexpr double pitEntryProgress = 36.0 + 5707.0 / circuitMetres;
inline constexpr double pitExitProgress = 37.0 + 332.0 / circuitMetres;
inline constexpr double pitBoxProgress = 37.0 + 75.0 / circuitMetres;

// Nominal 2025 slick diameter is 720 mm (FIA tyre technical targets).
// The 3D tyres and race wheel spin use the same physical dimensions.
inline constexpr double raceTyreRadiusMetres = 0.360;
inline constexpr double raceTyreCircumference = 2.0 * std::numbers::pi * raceTyreRadiusMetres;
[[nodiscard]] inline double raceWheelTurns(double progress)
{
    return (progress - gridOffset) * circuitMetres / raceTyreCircumference;
}
[[nodiscard]] inline double raceWheelRpm(double sceneVelocity, double scenePathLength)
{
    return sceneVelocity / scenePathLength * circuitMetres / raceTyreCircumference * 60.0;
}

struct RaceSample {
    Pose pose;
    double progress; // unwrapped progress: grid offset through 53 completed crossings
    double velocity; // scene distance / race second, zero at the pit box
    bool inPit;
    bool servicing;
    double travelledMetres;
    bool visible{true},retired{false},dns{false};
};

class RaceMotion {
public:
    explicit RaceMotion(const std::vector<PathSample>& path) : path_(path)
    {
        // Integrate the reconstructed pit path once so the visible wheels roll
        // over its actual length, including the lateral service-bay excursion.
        pitArc_.push_back({pitEntryProgress,0.0});
        auto previous=pitPose(pitEntryProgress).position;
        double distance=0;
        for(int i=1;i<=1024;++i) {
            const double progress=pitEntryProgress+(pitExitProgress-pitEntryProgress)*i/1024.0;
            const auto p=pitPose(progress).position;
            distance+=std::hypot(p.x-previous.x,p.y-previous.y)*physical::metresPerSceneUnit();
            pitArc_.push_back({progress,distance});previous=p;
        }
        // Sector lines are located schematically on the existing hand-authored
        // circuit using the FIA map. All sector and lap crossing times are exact.
        const double s1 = nearestProgress({-1.33, -0.12});
        const double s2 = nearestProgress({-0.02, -0.14});
        keys_.push_back({0.0, gridOffset});
        for (std::size_t i = 0; i < raceLaps.size(); ++i) {
            const auto& lap = raceLaps[i];
            const double start = raceBoundariesMs[i] / 1000.0;
            keys_.push_back({start + lap.s1 / 1000.0, i + s1});
            keys_.push_back({start + (lap.s1 + lap.s2) / 1000.0, i + s2});
            keys_.push_back({raceBoundariesMs[i + 1] / 1000.0, static_cast<double>(i + 1)});
        }
        keys_.push_back({pitEntrySeconds, pitEntryProgress});
        keys_.push_back({pitBoxStartSeconds, pitBoxProgress});
        keys_.push_back({pitBoxEndSeconds, pitBoxProgress});
        keys_.push_back({pitExitSeconds, pitExitProgress});
        std::sort(keys_.begin(), keys_.end(), [](const Key& a, const Key& b) { return a.time < b.time; });
    }

    [[nodiscard]] RaceSample sample(double seconds) const
    {
        const double time = std::clamp(seconds, 0.0, raceTotalSeconds);
        const auto upper = std::upper_bound(keys_.begin(), keys_.end(), time,
            [](double t, const Key& k) { return t < k.time; });
        if (upper == keys_.end()) {
            return {poseAtProgress(path_, 0.0), 53.0, 0.0, false, false, travelledMetres(53.0)};
        }
        const auto& before = *(upper - 1);
        const auto& after = *upper;
        const double delta = after.progress-before.progress;
        const double rate = delta*profile_.rate(time,before.time,after.time);
        const double progress = before.progress+delta*profile_.fraction(time,before.time,after.time);
        const bool inPit = time >= pitEntrySeconds && time < pitExitSeconds;
        return {inPit ? pitPose(progress) : poseAtProgress(path_, progress), progress,
            rate * distancePerProgress(progress) / physical::metresPerSceneUnit(), inPit,
            time >= pitBoxStartSeconds && time < pitBoxEndSeconds, travelledMetres(progress)};
    }

    [[nodiscard]] double travelledMetres(double progress) const
    {
        const double lapMetres=path_.back().distance*physical::metresPerSceneUnit();
        if(progress<=pitEntryProgress)return (progress-gridOffset)*lapMetres;
        const double after=std::max(0.0,progress-pitExitProgress)*lapMetres;
        return (pitEntryProgress-gridOffset)*lapMetres+pitDistance(progress)+after;
    }

    [[nodiscard]] Pose pitPose(double progress, bool serviceBay = true, double bayProgress = pitBoxProgress) const
    {
        const auto pointAt = [&](double p) {
            auto pose = poseAtProgress(path_, p);
            const double blend = std::clamp(std::min(
                (p - pitEntryProgress) / 0.021,
                (pitExitProgress - p) / 0.006), 0.0, 1.0);
            // Raised inside the main straight; smooth joins at both gates.
            const double bayBlend = std::clamp(1.0-std::abs(p-bayProgress)/0.008,0.0,1.0);
            const double bay = bayBlend*bayBlend*(3.0-2.0*bayBlend);
            pose.position.y += physical::sceneUnits(physical::pitOffset * blend * blend *
                (3.0 - 2.0 * blend) + (serviceBay ? physical::pitBayOffset * bay : 0.0));
            return pose.position;
        };
        const auto a = pointAt(progress - 1e-6);
        const auto b = pointAt(progress + 1e-6);
        return {pointAt(progress), std::atan2(b.y - a.y, b.x - a.x),
            progress * path_.back().distance};
    }

    [[nodiscard]] std::vector<Vertex> pitMesh(bool includeGarage = true) const
    {
        std::vector<Vertex> mesh;
        constexpr Color asphalt{0.17F, 0.195F, 0.225F};
        constexpr Color edge{0.46F, 0.50F, 0.50F};
        constexpr Color concrete{0.46F, 0.48F, 0.46F};
        constexpr Color boxPaint{0.74F, 0.64F, 0.35F};
        std::vector<PathSample> lane;
        for (int i = 0; i <= 192; ++i) {
            const double p = pitEntryProgress + (pitExitProgress - pitEntryProgress) * i / 192.0;
            const auto pose = pitPose(p);
            lane.push_back({pose.position,
                {-std::sin(pose.headingRadians), std::cos(pose.headingRadians)}, 0.0, p});
        }
        // A continuous road ribbon shares the main circuit's material. Thin
        // paint and a short concrete wall distinguish it without a bright halo.
        for (std::size_t i = 0; i + 1 < lane.size(); ++i)
            ribbonSegment(mesh, lane[i], lane[i+1], -0.035, 0.035, edge);
        for (std::size_t i = 0; i + 1 < lane.size(); ++i)
            ribbonSegment(mesh, lane[i], lane[i+1], -0.032, 0.032, asphalt);
        for (std::size_t i = 0; i + 1 < lane.size(); ++i) {
            if (lane[i].u > pitEntryProgress + 0.021 && lane[i+1].u < pitExitProgress - 0.024)
                ribbonSegment(mesh, lane[i], lane[i+1], 0.039, 0.044, concrete);
        }
        const auto box = pitPose(pitBoxProgress).position;
        const auto rectangle = [&](double left, double bottom, double right, double top, Color color) {
            quad(mesh, {left, bottom}, {right, bottom}, {right, top}, {left, top}, color);
        };
        // Painted service-bay edges leave the asphalt visible below the car.
        rectangle(box.x-0.058, box.y+0.030, box.x+0.058, box.y+0.033, boxPaint);
        rectangle(box.x-0.058, box.y-0.033, box.x+0.058, box.y-0.030, boxPaint);
        rectangle(box.x-0.058, box.y-0.033, box.x-0.055, box.y+0.033, boxPaint);
        rectangle(box.x+0.055, box.y-0.033, box.x+0.058, box.y+0.033, boxPaint);
        if (!includeGarage) return mesh;
        // One low garage building with a concrete apron and recessed doors.
        rectangle(box.x-0.17, box.y+0.035, box.x+0.17, box.y+0.054, {0.25F, 0.27F, 0.27F});
        rectangle(box.x-0.17, box.y+0.054, box.x+0.17, box.y+0.091, {0.22F, 0.25F, 0.26F});
        for (int i = 0; i < 4; ++i) {
            const double x = box.x-0.156+i*0.082;
            rectangle(x, box.y+0.054, x+0.062, box.y+0.063, {0.10F, 0.13F, 0.15F});
        }
        return mesh;
    }

private:
    struct Key { double time; double progress; };
    std::vector<PathSample> path_;
    std::vector<Key> keys_;
    RecordedDistance profile_;
    struct ArcPoint {double progress,distance;};
    std::vector<ArcPoint> pitArc_;
    double pitDistance(double progress) const {
        if(progress>=pitExitProgress)return pitArc_.back().distance;
        const auto upper=std::upper_bound(pitArc_.begin(),pitArc_.end(),progress,
            [](double p,const ArcPoint& a){return p<a.progress;});
        if(upper==pitArc_.begin())return 0;
        const auto& a=*(upper-1);const auto& b=*upper;
        return a.distance+(progress-a.progress)*(b.distance-a.distance)/(b.progress-a.progress);
    }
    double distancePerProgress(double progress) const {
        if(progress<pitEntryProgress||progress>=pitExitProgress)
            return path_.back().distance*physical::metresPerSceneUnit();
        const auto upper=std::upper_bound(pitArc_.begin(),pitArc_.end(),progress,
            [](double p,const ArcPoint& a){return p<a.progress;});
        const auto& a=*(upper-1);const auto& b=*upper;
        return (b.distance-a.distance)/(b.progress-a.progress);
    }

    double nearestProgress(Vec2 target) const
    {
        const auto closest = std::min_element(path_.begin(), path_.end(),
            [&](const PathSample& a, const PathSample& b) {
                return std::hypot(a.point.x-target.x, a.point.y-target.y) <
                       std::hypot(b.point.x-target.x, b.point.y-target.y);
            });
        return closest->distance / path_.back().distance;
    }
};

} // namespace apex3d::cp3
