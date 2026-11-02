#include "include/render.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>

int main(int argc, char* argv[])
{
    namespace bezier = apex3d::bezier;

    try {
        const std::filesystem::path output = argc > 1
            ? std::filesystem::path(argv[1])
            : std::filesystem::path(__FILE__).parent_path() / "output";
        const std::filesystem::path frames = output / "frames";

        for (int frame = 0; frame < bezier::frameCount; ++frame) {
            std::ostringstream filename;
            filename << "bezier_frame_" << std::setw(2) << std::setfill('0')
                     << frame << ".ppm";
            bezier::render(frame).writePpm(frames / filename.str());
        }

        apex3d::raster::Canvas sheet(560 * 4, 360 * 3, bezier::background);
        std::cout << "Apex3D Checkpoint 2 - arc-length Bezier car motion\n"
                  << "  Circuit segments : " << bezier::segmentCount << '\n'
                  << "  Animation frames : " << bezier::frameCount << '\n'
                  << "  Approx. lap length: " << std::fixed << std::setprecision(2)
                  << bezier::arcLengthTable().totalLength() << " px\n";
        for (int tile = 0; tile < 12; ++tile) {
            const int frame = tile * (bezier::frameCount - 1) / 11;
            sheet.blit(bezier::render(frame), (tile % 4) * 560, (tile / 4) * 360);
            std::cout << "  Contact frame    : " << frame << '\n';
        }
        const std::filesystem::path sheetPath = output / "bezier_contact_sheet.ppm";
        sheet.writePpm(sheetPath);
        std::cout << "  Frames            : " << frames << '\n'
                  << "  Contact sheet     : " << sheetPath << '\n';
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "Bezier animation failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
