#include "include/wheel_animation.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>

int main(int argc, char* argv[])
{
    namespace wheels = apex3d::wheels;

    try {
        const std::filesystem::path outputDirectory =
            argc > 1 ? std::filesystem::path(argv[1])
                     : std::filesystem::path(__FILE__).parent_path() / "output";
        const std::filesystem::path frameDirectory = outputDirectory / "frames";

        const wheels::CarModel car;
        for (int frame = 0; frame < wheels::kFrameCount; ++frame) {
            std::array<char, 32> name{};
            std::snprintf(name.data(), name.size(), "wheel_frame_%02d.ppm", frame);
            wheels::renderFrame(car, frame).writePpm(frameDirectory / name.data());
        }

        const std::filesystem::path sheetPath = outputDirectory / "wheel_contact_sheet.ppm";
        wheels::renderContactSheet(car).writePpm(sheetPath);

        std::cout << "Apex3D Checkpoint 2 - Formula 1 wheel animation\n"
                  << "  Frames        : " << wheels::kFrameCount << " x "
                  << wheels::kFrameWidth << " x " << wheels::kFrameHeight << " RGB24 PPM\n"
                  << "  Step          : " << wheels::kDegreesPerFrame
                  << " degrees per frame (one full revolution)\n"
                  << "  Travel        : " << wheels::rollingDistance(360.0)
                  << " px per revolution (2 * pi * " << wheels::kTyreRadius << ")\n"
                  << "  Frames        : " << frameDirectory.string() << '\n'
                  << "  Contact sheet : " << sheetPath.string() << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Apex3D wheel animation failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
