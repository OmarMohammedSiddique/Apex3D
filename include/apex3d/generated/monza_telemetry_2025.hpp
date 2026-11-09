#pragma once
#include <array>
#include <span>
namespace apex3d::cp3 {
// Archived OpenF1 session 9912. Definitions compile once in the data library.
struct TelemetryPoint {int raceMs,speedKmh,gear,engineRpm;};
extern const std::array<TelemetryPoint,16394> verstappenTelemetry;
extern const std::array<TelemetryPoint,16464> norrisTelemetry;
extern const std::array<TelemetryPoint,16471> piastriTelemetry;
extern const std::array<TelemetryPoint,16485> leclercTelemetry;
extern const std::array<TelemetryPoint,16514> russellTelemetry;
extern const std::array<TelemetryPoint,16533> hamiltonTelemetry;
extern const std::array<TelemetryPoint,16578> albonTelemetry;
extern const std::array<TelemetryPoint,16608> bortoletoTelemetry;
extern const std::array<TelemetryPoint,16593> antonelliTelemetry;
extern const std::array<TelemetryPoint,16623> hadjarTelemetry;
extern const std::array<TelemetryPoint,16626> sainzTelemetry;
extern const std::array<TelemetryPoint,16643> bearmanTelemetry;
extern const std::array<TelemetryPoint,16684> tsunodaTelemetry;
extern const std::array<TelemetryPoint,16691> lawsonTelemetry;
extern const std::array<TelemetryPoint,16402> oconTelemetry;
extern const std::array<TelemetryPoint,16415> gaslyTelemetry;
extern const std::array<TelemetryPoint,16422> colapintoTelemetry;
extern const std::array<TelemetryPoint,16429> strollTelemetry;
extern const std::array<TelemetryPoint,7773> alonsoTelemetry;
extern const std::array<TelemetryPoint,0> hulkenbergTelemetry;
std::span<const TelemetryPoint> recordedSeries(int number);
}
