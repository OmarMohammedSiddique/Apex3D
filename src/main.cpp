#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "../include/apex3d/cp3_scene.hpp"
#include "../include/apex3d/cp3_hud.hpp"
#include "../include/apex3d/cp3_wheels.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int windowWidth = 1280;
constexpr int windowHeight = 720;
constexpr double lapSeconds = 18.0;
// The close-up camera follows the car so the CP2 wheel motion is readable.
constexpr float closeUpZoom = 7.0F;

constexpr char vertexShaderSource[] = R"(
    #version 330 core
    layout (location = 0) in vec2 position;
    layout (location = 1) in vec3 vertexColor;
    uniform vec2 viewportScale;
    uniform vec2 viewCentre;
    out vec3 color;
    void main()
    {
        gl_Position = vec4((position - viewCentre) * viewportScale, 0.0, 1.0);
        color = vertexColor;
    }
)";

constexpr char fragmentShaderSource[] = R"(
    #version 330 core
    in vec3 color;
    out vec4 fragmentColor;
    void main()
    {
        fragmentColor = vec4(color, 1.0);
    }
)";

GLuint compileShader(GLenum type, const char* source)
{
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint succeeded = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &succeeded);
    if (succeeded == GL_TRUE) {
        return shader;
    }
    char infoLog[1024]{};
    glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
    std::cerr << "Shader compilation failed:\n" << infoLog << '\n';
    glDeleteShader(shader);
    return 0;
}

GLuint createShaderProgram()
{
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (vertexShader == 0 || fragmentShader == 0) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }
    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    GLint succeeded = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &succeeded);
    if (succeeded == GL_TRUE) {
        return program;
    }
    char infoLog[1024]{};
    glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
    std::cerr << "Shader linking failed:\n" << infoLog << '\n';
    glDeleteProgram(program);
    return 0;
}

struct GlMesh {
    GLuint vertexArray{};
    GLuint vertexBuffer{};
    GLsizei vertexCount{};

    void create(const std::vector<apex3d::cp3::Vertex>& vertices, GLenum usage)
    {
        glGenVertexArrays(1, &vertexArray);
        glGenBuffers(1, &vertexBuffer);
        glBindVertexArray(vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                              sizeof(apex3d::cp3::Vertex), nullptr);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                              sizeof(apex3d::cp3::Vertex),
                              reinterpret_cast<const void*>(offsetof(apex3d::cp3::Vertex, r)));
        glEnableVertexAttribArray(1);
        update(vertices, usage);
        glBindVertexArray(0);
    }

    void update(const std::vector<apex3d::cp3::Vertex>& vertices, GLenum usage)
    {
        vertexCount = static_cast<GLsizei>(vertices.size());
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(vertices.size() * sizeof(vertices.front())),
                     vertices.data(), usage);
    }

    void draw() const
    {
        glBindVertexArray(vertexArray);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void destroy()
    {
        glDeleteBuffers(1, &vertexBuffer);
        glDeleteVertexArrays(1, &vertexArray);
    }
};

struct Playback {
    double progress{};
    double speed{1.0};
    bool playing{true};
    bool showControls{false};
    bool scaleDemo{false};
    bool closeUp{false};
    apex3d::cp3::RouteMode routeMode{apex3d::cp3::RouteMode::CatmullRom};
};

bool pressedOnce(GLFWwindow* window, int key)
{
    static bool previous[GLFW_KEY_LAST + 1]{};
    const bool down = glfwGetKey(window, key) == GLFW_PRESS;
    const bool pressed = down && !previous[key];
    previous[key] = down;
    return pressed;
}

void processInput(GLFWwindow* window, Playback& playback)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (pressedOnce(window, GLFW_KEY_SPACE)) {
        playback.playing = !playback.playing;
    }
    if (pressedOnce(window, GLFW_KEY_R)) {
        playback.progress = 0.0;
        playback.playing = true;
    }
    if (pressedOnce(window, GLFW_KEY_P)) {
        playback.showControls = !playback.showControls;
    }
    if (pressedOnce(window, GLFW_KEY_T)) {
        playback.scaleDemo = !playback.scaleDemo;
    }
    if (pressedOnce(window, GLFW_KEY_C)) {
        playback.closeUp = !playback.closeUp;
    }
    if (pressedOnce(window, GLFW_KEY_B)) {
        playback.routeMode = (playback.routeMode == apex3d::cp3::RouteMode::CatmullRom)
            ? apex3d::cp3::RouteMode::Bezier
            : apex3d::cp3::RouteMode::CatmullRom;
    }
    if (pressedOnce(window, GLFW_KEY_EQUAL) ||
        pressedOnce(window, GLFW_KEY_KP_ADD)) {
        playback.speed = std::min(4.0, playback.speed * 1.25);
    }
    if (pressedOnce(window, GLFW_KEY_MINUS) ||
        pressedOnce(window, GLFW_KEY_KP_SUBTRACT)) {
        playback.speed = std::max(0.25, playback.speed / 1.25);
    }
}

void updateTitle(GLFWwindow* window, const Playback& playback,
                 double steerRadians)
{
    std::ostringstream title;
    title << "Apex3D | Monza OpenGL prototype | "
          << (playback.routeMode == apex3d::cp3::RouteMode::Bezier
                  ? "Bezier" : "Catmull-Rom")
          << " | Lap "
          << std::fixed << std::setprecision(0) << playback.progress * 100.0
          << "% | " << std::setprecision(2) << playback.speed << "x | "
          << (playback.playing ? "Playing" : "Paused") << " | Steer "
          << std::setprecision(0) << steerRadians * 180.0 / std::numbers::pi << " deg"
          << " | Space pause  R restart  B route  +/- speed  T scale  P points  C close-up  Esc quit";
    glfwSetWindowTitle(window, title.str().c_str());
}

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

bool saveScreenshot(const std::string& path, int width, int height)
{
    std::vector<unsigned char> pixels(
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }
    output << "P6\n" << width << ' ' << height << "\n255\n";
    const std::size_t stride = static_cast<std::size_t>(width) * 3;
    for (int row = height - 1; row >= 0; --row) {
        output.write(reinterpret_cast<const char*>(
            pixels.data() + static_cast<std::size_t>(row) * stride),
            static_cast<std::streamsize>(stride));
    }
    return static_cast<bool>(output);
}

} // namespace

int main(int argc, char** argv)
{
    // Accepted forms:
    //   Apex3D
    //   Apex3D --bezier
    //   Apex3D --screenshot output.ppm
    //   Apex3D --bezier --screenshot output.ppm
    //   Apex3D --screenshot-at progress output.ppm
    //   Apex3D --bezier --screenshot-at progress output.ppm
    //   Apex3D --screenshot-closeup-at progress output.ppm
    //   Apex3D --bezier --screenshot-closeup-at progress output.ppm
    bool startBezier = false;
    int  argOffset   = 0;
    if (argc >= 2 && std::string(argv[1]) == "--bezier") {
        startBezier = true;
        argOffset   = 1;
    }
    const int effectiveArgc = argc - argOffset;
    char** effectiveArgv    = argv + argOffset;
    const bool capture   = effectiveArgc == 3 &&
        std::string(effectiveArgv[1]) == "--screenshot";
    const bool captureCloseUp = effectiveArgc == 4 &&
        std::string(effectiveArgv[1]) == "--screenshot-closeup-at";
    const bool captureAt = captureCloseUp || (effectiveArgc == 4 &&
        std::string(effectiveArgv[1]) == "--screenshot-at");
    if (effectiveArgc != 1 && !capture && !captureAt) {
        std::cerr << "Usage: Apex3D [--bezier] "
                     "[--screenshot output.ppm | "
                     "--screenshot-at lap-progress output.ppm | "
                     "--screenshot-closeup-at lap-progress output.ppm]\n";
        return EXIT_FAILURE;
    }
    double initialProgress = 0.0;
    if (captureAt) {
        try {
            initialProgress = std::stod(effectiveArgv[2]);
        } catch (const std::exception&) {
            std::cerr << "Lap progress must be a number between 0 and 1.\n";
            return EXIT_FAILURE;
        }
        if (!std::isfinite(initialProgress) || initialProgress < 0.0 ||
            initialProgress > 1.0) {
            std::cerr << "Lap progress must be a number between 0 and 1.\n";
            return EXIT_FAILURE;
        }
    }
    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Failed to initialize GLFW.\n";
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight,
                                          "Apex3D - Monza OpenGL prototype",
                                          nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create the Apex3D window.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0) {
        std::cerr << "Failed to load OpenGL functions.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }
    std::cout << "OpenGL renderer: " << glGetString(GL_RENDERER) << '\n';
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
    const GLuint shaderProgram = createShaderProgram();
    if (shaderProgram == 0) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    const auto carModel = apex3d::car::carModel();
    const auto wheelMounts = apex3d::cp3::wheelMountsFrom(carModel);
    GlMesh track;
    GlMesh controlPoints;       // Catmull-Rom cyan dots
    GlMesh bezierAnchors;       // Bézier orange squares
    GlMesh car;
    GlMesh hud;

    // Initial build with Catmull-Rom route.
    auto activePath = apex3d::cp3::makePath();
    track.create(apex3d::cp3::makeTrackMesh(activePath), GL_DYNAMIC_DRAW);
    controlPoints.create(apex3d::cp3::makeControlMesh(), GL_STATIC_DRAW);
    bezierAnchors.create(apex3d::cp3::makeBezierAnchorMesh(), GL_STATIC_DRAW);
    car.create(apex3d::cp3::makeLiveCarMesh(carModel, wheelMounts,
        apex3d::cp3::poseAtProgress(activePath, 0.0),
        apex3d::cp3::wheelStateAt(activePath, wheelMounts, 0.0)), GL_DYNAMIC_DRAW);
    hud.create(apex3d::cp3::makeHudMesh(0.0, 1.0, true, false,
        apex3d::cp3::RouteMode::CatmullRom), GL_DYNAMIC_DRAW);
    std::cout << "Monza control points: " << apex3d::cp3::monzaControls.size()
              << " | spline samples: " << activePath.size()
              << " | track triangles: " << track.vertexCount / 3 << '\n';

    Playback playback;
    playback.progress  = initialProgress;
    if (startBezier) {
        playback.routeMode = apex3d::cp3::RouteMode::Bezier;
    }
    if (captureAt) {
        playback.playing = false;
        playback.closeUp = captureCloseUp;
    }
    // If starting in Bézier mode, rebuild path and track immediately.
    if (startBezier) {
        activePath = apex3d::cp3::makeBezierPath();
        track.update(apex3d::cp3::makeTrackMesh(activePath), GL_DYNAMIC_DRAW);
    }
    apex3d::cp3::RouteMode lastRoute = playback.routeMode;
    double previousTime = glfwGetTime();
    double lastTitleTime = -1.0;
    bool screenshotSaved = true;
    glDisable(GL_DEPTH_TEST);
    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        const double now = glfwGetTime();
        const double deltaSeconds = std::clamp(now - previousTime, 0.0, 0.1);
        previousTime = now;
        glfwPollEvents();
        processInput(window, playback);

        // Rebuild path and track mesh when the user switches route mode.
        if (playback.routeMode != lastRoute) {
            activePath = apex3d::cp3::makePathForMode(playback.routeMode);
            track.update(apex3d::cp3::makeTrackMesh(activePath), GL_DYNAMIC_DRAW);
            lastRoute = playback.routeMode;
            std::cout << "Route: "
                      << (playback.routeMode == apex3d::cp3::RouteMode::Bezier
                              ? "Bezier" : "Catmull-Rom")
                      << " | samples: " << activePath.size() << '\n';
        }

        if (playback.playing) {
            playback.progress += deltaSeconds * playback.speed / lapSeconds;
            playback.progress -= std::floor(playback.progress);
        }
        const auto pose = apex3d::cp3::poseAtProgress(activePath, playback.progress);
        const auto wheelState =
            apex3d::cp3::wheelStateAt(activePath, wheelMounts, playback.progress);
        if (now - lastTitleTime >= 0.2) {
            updateTitle(window, playback, wheelState.steerRadians);
            lastTitleTime = now;
        }

        const double scale = playback.scaleDemo ? 1.0 + 0.22 * std::sin(now * 3.0) : 1.0;
        car.update(apex3d::cp3::makeLiveCarMesh(carModel, wheelMounts, pose,
            wheelState, scale), GL_DYNAMIC_DRAW);
        hud.update(apex3d::cp3::makeHudMesh(playback.progress, playback.speed,
            playback.playing, playback.scaleDemo, playback.routeMode), GL_DYNAMIC_DRAW);

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            continue;
        }
        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        const float zoom = playback.closeUp ? closeUpZoom : 1.0F;
        const float yScale = std::min(0.94F / 1.1F, 0.94F * aspect / 1.85F) * zoom;
        const float xScale = yScale / aspect;
        const GLint viewCentre = glGetUniformLocation(shaderProgram, "viewCentre");

        glViewport(0, 0, width, height);
        glClearColor(0.045F, 0.11F, 0.085F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glUniform2f(glGetUniformLocation(shaderProgram, "viewportScale"),
                    xScale, yScale);
        if (playback.closeUp) {
            glUniform2f(viewCentre, static_cast<float>(pose.position.x),
                        static_cast<float>(pose.position.y));
        } else {
            glUniform2f(viewCentre, 0.0F, 0.0F);
        }
        track.draw();
        if (playback.showControls) {
            if (playback.routeMode == apex3d::cp3::RouteMode::Bezier) {
                bezierAnchors.draw();
            } else {
                controlPoints.draw();
            }
        }
        car.draw();
        glUniform2f(glGetUniformLocation(shaderProgram, "viewportScale"), 1.0F, 1.0F);
        glUniform2f(viewCentre, 0.0F, 0.0F);
        hud.draw();
        if (capture || captureAt) {
            const char* outputPath = captureAt ? effectiveArgv[3] : effectiveArgv[2];
            screenshotSaved = saveScreenshot(outputPath, width, height);
            if (!screenshotSaved) {
                std::cerr << "Could not save screenshot to " << outputPath << '\n';
            } else {
                std::cout << "Saved OpenGL screenshot to " << outputPath << '\n';
            }
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        glfwSwapBuffers(window);
    }

    hud.destroy();
    car.destroy();
    bezierAnchors.destroy();
    controlPoints.destroy();
    track.destroy();
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return screenshotSaved ? EXIT_SUCCESS : EXIT_FAILURE;
}
