// Apex3D – Checkpoint 2: Animation Timeline Contact Sheet
//
// Reads the PPM frames produced by timeline_demo and composites five of them
// (first, quarter, middle, three-quarter, final) into one PNG contact sheet.
//
// Build:
//   g++ -std=c++20 -Wall -Wextra -Wpedantic contact_sheet.cpp -o contact_sheet
//
// Run:
//   ./contact_sheet [frames_dir [output_dir]]

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Color + image
// ─────────────────────────────────────────────────────────────────────────────
struct Color { std::uint8_t r{}, g{}, b{}; };

struct Image {
    int width{};
    int height{};
    std::vector<Color> pixels;
};

[[nodiscard]] Image loadPpm(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open: " + path.string());
    std::string magic; in >> magic;
    if (magic != "P6") throw std::runtime_error("Not P6: " + path.string());
    int w{}, h{}, maxVal{};
    in >> w >> h >> maxVal; in.get();
    if (maxVal != 255) throw std::runtime_error("Unsupported maxVal");
    Image img; img.width = w; img.height = h;
    img.pixels.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    for (auto& px : img.pixels) {
        std::array<char,3> b{};
        in.read(b.data(), 3);
        px.r = static_cast<std::uint8_t>(b[0]);
        px.g = static_cast<std::uint8_t>(b[1]);
        px.b = static_cast<std::uint8_t>(b[2]);
    }
    return img;
}

// ─────────────────────────────────────────────────────────────────────────────
// Minimal uncompressed PNG writer (no external libs)
// ─────────────────────────────────────────────────────────────────────────────
namespace png {

std::uint32_t crc32Step(std::uint32_t crc, const std::uint8_t* data, std::size_t len)
{
    static const auto table = [](){
        std::array<std::uint32_t,256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            t[i] = c;
        }
        return t;
    }();
    for (std::size_t i = 0; i < len; ++i)
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}

void u32be(std::vector<std::uint8_t>& v, std::uint32_t x)
{
    v.push_back(static_cast<std::uint8_t>((x>>24)&0xFF));
    v.push_back(static_cast<std::uint8_t>((x>>16)&0xFF));
    v.push_back(static_cast<std::uint8_t>((x>>8)&0xFF));
    v.push_back(static_cast<std::uint8_t>(x&0xFF));
}

void chunk(std::vector<std::uint8_t>& out, const char type[4],
           const std::vector<std::uint8_t>& data)
{
    u32be(out, static_cast<std::uint32_t>(data.size()));
    auto ts = out.size();
    for (int i=0;i<4;++i) out.push_back(static_cast<std::uint8_t>(type[i]));
    out.insert(out.end(), data.begin(), data.end());
    std::uint32_t crc = 0xFFFFFFFFu;
    crc = crc32Step(crc, out.data()+ts, 4+data.size());
    u32be(out, crc ^ 0xFFFFFFFFu);
}

bool save(const std::filesystem::path& path, const Image& img)
{
    const int W = img.width, H = img.height;
    // Raw pixel data with filter byte
    std::vector<std::uint8_t> raw;
    raw.reserve(static_cast<std::size_t>(H) * (1+static_cast<std::size_t>(W)*3));
    for (int y=0;y<H;++y) {
        raw.push_back(0);
        for (int x=0;x<W;++x) {
            const Color& c = img.pixels[static_cast<std::size_t>(y*W+x)];
            raw.push_back(c.r); raw.push_back(c.g); raw.push_back(c.b);
        }
    }
    // zlib uncompressed wrapper
    std::vector<std::uint8_t> zlib;
    zlib.push_back(0x78); zlib.push_back(0x01);
    const std::size_t rawSz = raw.size();
    constexpr std::size_t kBlk = 65535;
    std::uint32_t s1=1, s2=0;
    for (std::size_t off=0; off<rawSz; ) {
        std::size_t blen = std::min(kBlk, rawSz-off);
        bool fin = (off+blen >= rawSz);
        zlib.push_back(fin ? 0x01 : 0x00);
        auto lo = static_cast<std::uint16_t>(blen&0xFFFF);
        zlib.push_back(lo&0xFF); zlib.push_back((lo>>8)&0xFF);
        zlib.push_back((~lo)&0xFF); zlib.push_back((~lo>>8)&0xFF);
        for (std::size_t i=0;i<blen;++i) {
            std::uint8_t b=raw[off+i]; zlib.push_back(b);
            s1=(s1+b)%65521; s2=(s2+s1)%65521;
        }
        off+=blen;
    }
    u32be(zlib,(s2<<16)|s1);

    std::vector<std::uint8_t> buf;
    const std::array<std::uint8_t,8> sig{137,80,78,71,13,10,26,10};
    buf.insert(buf.end(),sig.begin(),sig.end());
    std::vector<std::uint8_t> ihdr(13,0);
    ihdr[0]=(W>>24)&0xFF;ihdr[1]=(W>>16)&0xFF;ihdr[2]=(W>>8)&0xFF;ihdr[3]=W&0xFF;
    ihdr[4]=(H>>24)&0xFF;ihdr[5]=(H>>16)&0xFF;ihdr[6]=(H>>8)&0xFF;ihdr[7]=H&0xFF;
    ihdr[8]=8; ihdr[9]=2;
    chunk(buf,"IHDR",ihdr);
    chunk(buf,"IDAT",zlib);
    chunk(buf,"IEND",{});
    std::ofstream out(path,std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(buf.data()),
              static_cast<std::streamsize>(buf.size()));
    return static_cast<bool>(out);
}
} // namespace png

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] std::string frameFilename(int index)
{
    std::ostringstream oss;
    oss << "frame_" << std::setfill('0') << std::setw(3) << index << ".ppm";
    return oss.str();
}

int main(int argc, char* argv[])
{
    const std::filesystem::path framesDir = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::path(__FILE__).parent_path() / "output";
    const std::filesystem::path outputDir = argc > 2
        ? std::filesystem::path(argv[2])
        : framesDir;
    std::filesystem::create_directories(outputDir);

    // Count PPM frames
    int totalFrames = 0;
    for (const auto& entry : std::filesystem::directory_iterator(framesDir))
        if (entry.path().extension() == ".ppm" &&
            entry.path().stem().string().rfind("frame_", 0) == 0)
            ++totalFrames;

    if (totalFrames < 2) {
        std::cerr << "Need at least 2 PPM frames in " << framesDir << '\n';
        return EXIT_FAILURE;
    }

    // 5 representative picks
    const std::array<int,5> picks = {
        0,
        (totalFrames-1)/4,
        (totalFrames-1)/2,
        3*(totalFrames-1)/4,
        totalFrames-1,
    };
    const std::array<const char*,5> labels = {"First","Quarter","Middle","3/4","Final"};

    const Image ref = loadPpm(framesDir / frameFilename(picks[0]));
    const int W = ref.width, H = ref.height;
    constexpr int kPad = 8;

    Image sheet;
    sheet.width  = W * 5 + kPad * 6;
    sheet.height = H + kPad * 2;
    sheet.pixels.assign(
        static_cast<std::size_t>(sheet.width) * static_cast<std::size_t>(sheet.height),
        Color{9,12,20});

    std::cout << "Contact sheet: " << totalFrames << " frames found; using 5.\n";

    for (int i = 0; i < 5; ++i) {
        const Image frame = loadPpm(framesDir / frameFilename(picks[i]));
        const int ox = kPad + i * (W + kPad);
        const int oy = kPad;
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                sheet.pixels[static_cast<std::size_t>((oy+y)*sheet.width+(ox+x))]
                    = frame.pixels[static_cast<std::size_t>(y*W+x)];
        std::cout << "  [" << i << "] " << frameFilename(picks[i])
                  << "  (" << labels[i] << ")\n";
    }

    const std::filesystem::path pngPath = outputDir / "contact_sheet.png";
    const std::filesystem::path ppmPath = outputDir / "contact_sheet.ppm";

    // Write PPM
    {
        std::ofstream out(ppmPath, std::ios::binary);
        out << "P6\n" << sheet.width << ' ' << sheet.height << "\n255\n";
        for (const Color& c : sheet.pixels) {
            std::array<char,3> b{static_cast<char>(c.r),
                                  static_cast<char>(c.g),
                                  static_cast<char>(c.b)};
            out.write(b.data(), 3);
        }
    }

    if (!png::save(pngPath, sheet)) {
        std::cerr << "ERROR: failed to write PNG\n";
        return EXIT_FAILURE;
    }

    std::cout << "Output:\n"
              << "  PPM : " << ppmPath << "\n"
              << "  PNG : " << pngPath << "\n";
    return EXIT_SUCCESS;
}

