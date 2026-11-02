#include <iomanip>
#include <iostream>
#include <sstream>
#include "include/render.hpp"

int main(int argc, char** argv)
{
    try {
        const std::filesystem::path output = argc > 1 ? argv[1] :
            std::filesystem::path(__FILE__).parent_path() / "output";
        apex3d::raster::Canvas sheet(560 * 4, 360 * 3, apex3d::spline::background);
        for (int frame = 0; frame < apex3d::spline::frameCount; ++frame) {
            std::ostringstream name;
            name << "spline_frame_" << std::setw(2) << std::setfill('0') << frame << ".ppm";
            apex3d::spline::render(frame).writePpm(output / "frames" / name.str());
        }
        // Eleven evenly spaced lap views plus the exact closing frame.
        for (int tile = 0; tile < 12; ++tile) {
            const int frame = tile * (apex3d::spline::frameCount - 1) / 11;
            sheet.blit(apex3d::spline::render(frame), (tile % 4) * 560, (tile / 4) * 360);
        }
        sheet.writePpm(output / "spline_contact_sheet.ppm");
        std::cout << "Wrote 33 numbered frames and contact sheet to " << output << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
