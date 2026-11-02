// Apex3D - Checkpoint 2: polygon Formula 1 car animation demo.
//
// Writes 25 numbered PPM frames (car_frame_00.ppm ... car_frame_24.ppm) and a
// contact sheet as both PPM and PNG.
//
// Build and run from the repository root:
//   c++ -std=c++20 -Wall -Wextra -Wpedantic checkpoint2/animation/car/main.cpp -o apex3d-car-demo
//   ./apex3d-car-demo [output_directory]

#include "include/car_animation.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[])
{
    namespace car = apex3d::car;

    const std::filesystem::path outputDirectory = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::path(__FILE__).parent_path() / "output";
    const std::filesystem::path frameDirectory = outputDirectory / "frames";
    std::filesystem::create_directories(frameDirectory);

    std::cout << "Apex3D Checkpoint 2 - Polygon Formula 1 car animation\n"
              << "  Canvas     : " << car::kCanvasWidth << " x " << car::kCanvasHeight << " RGB\n"
              << "  Frames     : " << car::kFrameCount << '\n'
              << "  Components : " << car::carModel().size()
              << " polygons, one shared matrix per frame\n\n"
              << "  frame  phase                     centre (x, y)      angle   scale (x, y)\n";

    for (int frame = 0; frame < car::kFrameCount; ++frame) {
        char name[32];
        std::snprintf(name, sizeof(name), "car_frame_%02d.ppm", frame);
        const std::filesystem::path path = frameDirectory / name;
        if (!car::renderFrame(frame).writePpm(path)) {
            std::cerr << "ERROR: failed to write " << path << '\n';
            return EXIT_FAILURE;
        }

        const car::Pose pose = car::poseAtFrame(frame);
        char line[160];
        std::snprintf(line, sizeof(line), "  %5d  %-24s  (%6.1f, %6.1f)  %6.1f  (%4.2f, %4.2f)\n",
                      frame, std::string(car::phaseName(car::phaseAtFrame(frame))).c_str(),
                      pose.position.x, pose.position.y, pose.rotationDegrees,
                      pose.scaleX, pose.scaleY);
        std::cout << line;
    }

    const car::Canvas sheet = car::renderContactSheet();
    const std::filesystem::path sheetPpm = outputDirectory / "car_contact_sheet.ppm";
    const std::filesystem::path sheetPng = outputDirectory / "car_contact_sheet.png";
    if (!sheet.writePpm(sheetPpm) || !car::png::write(sheetPng, sheet)) {
        std::cerr << "ERROR: failed to write the contact sheet\n";
        return EXIT_FAILURE;
    }

    std::cout << "\n  Frames        : " << frameDirectory.string() << '\n'
              << "  Contact sheet : " << sheetPng.string() << "\n  Sheet frames  :";
    for (int frame : car::kContactFrames) {
        std::cout << ' ' << frame;
    }
    std::cout << '\n';
    return EXIT_SUCCESS;
}
