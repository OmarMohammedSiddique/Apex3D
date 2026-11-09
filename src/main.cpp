#include "../include/apex3d/traffic_replay.hpp"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "../include/apex3d/cp3_scene.hpp"
#include "../include/apex3d/cp3_hud.hpp"
#include "../include/apex3d/cp3_playback.hpp"
#include "../include/apex3d/cp3_start_audio.hpp"
#include "../include/apex3d/renderer3d.hpp"
#include "../include/apex3d/hud3d.hpp"
#include "../include/apex3d/replay_controls.hpp"

#include <algorithm>
#include <cstddef>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int windowWidth = 1280;
constexpr int windowHeight = 720;
using apex3d::cp3::Playback;

constexpr char vertexShaderSource[] = R"(
    #version 330 core
    layout (location = 0) in vec2 position;
    layout (location = 1) in vec3 vertexColor;
    uniform vec2 viewportScale;
    uniform vec2 viewportOffset;
    out vec3 color;
    void main()
    {
        gl_Position = vec4(position * viewportScale + viewportOffset, 0.0, 1.0);
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

bool pressedOnce(GLFWwindow* window, int key)
{
    static bool previous[GLFW_KEY_LAST + 1]{};
    const bool down = glfwGetKey(window, key) == GLFW_PRESS;
    const bool pressed = down && !previous[key];
    previous[key] = down;
    return pressed;
}

struct WindowInput {
    double x{},y{};
    bool dragging{false},hudVisible{true};
    bool scrubbing{false},raceMode{true};
    int fieldPage{};
    apex3d::cp3::HeatLayer heat{apex3d::cp3::HeatLayer::Speed};
    apex3d::three::BarTarget pendingBar{};
    apex3d::three::CameraRig camera;
    apex3d::cp3::DriverFocus focus{apex3d::cp3::DriverFocus::Verstappen};
    apex3d::cp3::HudAction pendingAction{apex3d::cp3::HudAction::None};
};

apex3d::cp3::HudAction hoveredAction(GLFWwindow* window)
{
    double x = 0.0, y = 0.0;
    int width = 0, height = 0;
    if (const auto* input = static_cast<WindowInput*>(glfwGetWindowUserPointer(window))) {
        x = input->x; y = input->y;
    } else glfwGetCursorPos(window, &x, &y);
    glfwGetWindowSize(window, &width, &height);
    if (width <= 0 || height <= 0) return apex3d::cp3::HudAction::None;
    const auto* state=static_cast<WindowInput*>(glfwGetWindowUserPointer(window));
    return apex3d::three::overlayAction(2.0*x/width-1.0, 1.0-2.0*y/height, state->hudVisible,state->fieldPage);
}

void processInput(GLFWwindow* window, Playback& playback, bool audioAvailable)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (pressedOnce(window, GLFW_KEY_SPACE)) {
        playback.togglePlaying();
    }
    if (pressedOnce(window, GLFW_KEY_R)) {
        playback.restart();
    }
    if (pressedOnce(window, GLFW_KEY_P)) {
        playback.showControls = !playback.showControls;
    }
    if (pressedOnce(window, GLFW_KEY_B)) {
        playback.route = playback.route == apex3d::cp3::Route::Monza
            ? apex3d::cp3::Route::BezierDemo : apex3d::cp3::Route::Monza;
        playback.restart();
        playback.scaleDemo = false;
    }
    if (pressedOnce(window, GLFW_KEY_L)) {
        if (!playback.isRace()) playback.looping = !playback.looping;
    }
    if (pressedOnce(window, GLFW_KEY_W)) {
        playback.showWheels = !playback.showWheels;
    }
    if (pressedOnce(window, GLFW_KEY_T)) {
        if (!playback.isRace()) playback.scaleDemo = !playback.scaleDemo;
    }
    if (pressedOnce(window, GLFW_KEY_EQUAL) ||
        pressedOnce(window, GLFW_KEY_KP_ADD)) {
        playback.speed = std::min(64.0, playback.speed * 2.0);
    }
    if (pressedOnce(window, GLFW_KEY_MINUS) ||
        pressedOnce(window, GLFW_KEY_KP_SUBTRACT)) {
        playback.speed = std::max(0.25, playback.speed / 2.0);
    }
    auto* input=static_cast<WindowInput*>(glfwGetWindowUserPointer(window));
    if(pressedOnce(window,GLFW_KEY_C))input->camera.cycle();
    if(pressedOnce(window,GLFW_KEY_F))input->camera.reset();
    if(pressedOnce(window,GLFW_KEY_H))input->hudVisible=!input->hudVisible;
    if(pressedOnce(window,GLFW_KEY_V))input->heat=apex3d::cp3::nextHeatLayer(input->heat);
    if(pressedOnce(window,GLFW_KEY_E)&&playback.isRace()){
        playback.seek(apex3d::cp3::raceTotalSeconds);playback.postRaceElapsed=apex3d::cp3::postRaceDuration;
        input->camera.initialized=false;
    }
    if (pressedOnce(window, GLFW_KEY_1)) playback.speed = 1.0;
    if (pressedOnce(window, GLFW_KEY_M) && audioAvailable) playback.muted = !playback.muted;
    if (pressedOnce(window, GLFW_KEY_RIGHT)) playback.seek(playback.elapsed + 10.0);
    if (pressedOnce(window, GLFW_KEY_LEFT)) playback.seek(playback.elapsed - 10.0);
    if (pressedOnce(window, GLFW_KEY_J)&&apex3d::cp3::driverInfo(input->focus).pitEntry>=0) playback.seek(apex3d::cp3::driverInfo(input->focus).pitEntry - 5.0);
    if (pressedOnce(window, GLFW_KEY_N) && playback.isRace()) {
        playback.seek(apex3d::cp3::nextDriverLap(input->focus,playback.elapsed));
    }
    const auto action=input->pendingAction;
    input->pendingAction=apex3d::cp3::HudAction::None;
    const auto bar=input->pendingBar;
    input->pendingBar={};
    if(playback.isRace()) {
        using apex3d::three::BarHit;
        if(bar.hit==BarHit::Play)playback.togglePlaying();
        if(bar.hit==BarHit::Timeline)playback.seek(bar.fraction*apex3d::cp3::raceTotalSeconds);
        if(bar.hit==BarHit::Speed)playback.speed=apex3d::three::speedForFraction(bar.fraction);
        if(action==apex3d::cp3::HudAction::Play)playback.togglePlaying();
        if(action==apex3d::cp3::HudAction::Mute&&audioAvailable)playback.muted=!playback.muted;
        const auto before=input->focus;
        if(action==apex3d::cp3::HudAction::FieldPrevious)input->fieldPage=(input->fieldPage+6)%7;
        if(action==apex3d::cp3::HudAction::FieldNext)input->fieldPage=(input->fieldPage+1)%7;
        for(const auto& driver:apex3d::cp3::raceDrivers)
            if(action==apex3d::three::focusAction(driver.focus))input->focus=driver.focus;
        if(before!=input->focus)input->camera.initialized=false;
    }

}

void updateTitle(GLFWwindow* window, const Playback& playback,apex3d::cp3::DriverFocus focus)
{
    std::ostringstream title;
    if (playback.isRace()) {
        const auto clock = apex3d::cp3::driverClock(focus,playback.simulationSeconds());
        title << "Apex3D | Monza 2025 - " << apex3d::cp3::driverInfo(focus).name << " | Lap " << clock.lap
              << "/53 | Race " << apex3d::cp3::clockText(playback.elapsed, true)
              << " | " << playback.speed << "x | "
              << (playback.finished() ? (playback.postRaceElapsed>=apex3d::cp3::postRaceDuration?"Parked":"Post race") : !playback.started ? "Ready - Space to start" :
                  !playback.playing ? "Paused" : playback.starting() ? "Starting lights" : "Racing");
    } else {
        title << "Apex3D | Bezier teaching demo | " << std::fixed << std::setprecision(0)
              << playback.progress * 100.0 << "% | " << playback.speed << "x | "
              << (playback.playing ? "Playing" : "Paused");
    }
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
    bool initialBezier = false, showWheels = false, capture = false;
    std::string outputPath;
    double initialProgress = -1.0, initialRaceTime = -1.0, initialStartTime = -1.0, initialPostTime=-1.0;
    double initialSpeed = 1.0,initialZoom=1.0;
    apex3d::cp3::DriverFocus initialFocus=apex3d::cp3::DriverFocus::Verstappen;
    apex3d::three::CameraMode initialCamera=apex3d::three::CameraMode::Chase;
    apex3d::cp3::HeatLayer initialHeat=apex3d::cp3::HeatLayer::Speed;
    try {
        const auto number = [](const std::string& value) {
            std::size_t consumed = 0;
            const double result = std::stod(value, &consumed);
            if (consumed != value.size() || !std::isfinite(result)) throw std::invalid_argument("Invalid number");
            return result;
        };
        const auto bounded = [&](const std::string& value, double low, double high) {
            const double result = number(value);
            if (result < low || result > high) throw std::invalid_argument("Number out of range");
            return result;
        };
        int timingOptions = 0;
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            const auto next = [&]() -> std::string {
                if (++i >= argc) throw std::invalid_argument("Missing argument for " + arg);
                return argv[i];
            };
            if(arg=="--camera") {
                const auto value=next();
                if(value=="overview")initialCamera=apex3d::three::CameraMode::Overview;
                else if(value=="chase")initialCamera=apex3d::three::CameraMode::Chase;
                else if(value=="cockpit")initialCamera=apex3d::three::CameraMode::Cockpit;
                else throw std::invalid_argument("Camera must be overview, chase or cockpit");
            }
            else if(arg=="--driver") {
                const auto value=next();
                bool found=false;
                for(const auto& driver:apex3d::cp3::raceDrivers) {
                    std::string tag=driver.tag;std::transform(tag.begin(),tag.end(),tag.begin(),[](unsigned char c){return std::tolower(c);});
                    if(value==driver.archive->slug||value==std::to_string(static_cast<int>(driver.focus))||value==tag.substr(0,3)){
                        initialFocus=driver.focus;found=true;break;
                    }
                }
                if(!found)throw std::invalid_argument("Driver must be an official 2025 Monza surname, abbreviation or number");
            }
            else if(arg=="--heatmap") {
                const auto value=next();
                if(value=="speed")initialHeat=apex3d::cp3::HeatLayer::Speed;
                else if(value=="off")initialHeat=apex3d::cp3::HeatLayer::Off;
                else throw std::invalid_argument("Heatmap must be speed or off");
            }
            else if (arg == "--bezier") initialBezier = true;
            else if (arg == "--wheels") showWheels = true;
            else if (arg == "--zoom") initialZoom=bounded(next(),0.008,3.0);
            else if (arg == "--speed") initialSpeed = bounded(next(), 0.25, 64.0);
            else if (arg == "--race-time") { initialRaceTime = bounded(next(), 0.0, apex3d::cp3::raceTotalSeconds); ++timingOptions; }
            else if (arg == "--screenshot" || arg == "--screenshot-at" ||
                     arg == "--screenshot-time" || arg == "--screenshot-start" || arg == "--screenshot-post") {
                if (capture) throw std::invalid_argument("Choose one screenshot mode");
                capture = true;
                if (arg == "--screenshot-at") { initialProgress = bounded(next(), 0.0, 1.0); ++timingOptions; }
                if(arg=="--screenshot-post"){initialPostTime=std::min(bounded(next(),0.0,600.0),apex3d::cp3::postRaceDuration);initialRaceTime=apex3d::cp3::raceTotalSeconds;++timingOptions;}
                else if (arg == "--screenshot-time") { initialRaceTime = bounded(next(), 0.0, apex3d::cp3::raceTotalSeconds); ++timingOptions; }
                if (arg == "--screenshot-start") { initialStartTime = bounded(next(), 0.0, apex3d::cp3::startSequenceSeconds); ++timingOptions; }
                outputPath = next();
            } else throw std::invalid_argument("Unknown option: " + arg);
        }
        if (timingOptions > 1) throw std::invalid_argument("Choose one initial time or progress option");
        if (initialProgress < -1.0 || initialProgress > 1.0 ||
            initialRaceTime < -1.0 || initialRaceTime > apex3d::cp3::raceTotalSeconds ||
            initialStartTime < -1.0 || initialStartTime > apex3d::cp3::startSequenceSeconds ||
            initialSpeed < 0.25 || initialSpeed > 64.0)
            throw std::invalid_argument("Time, progress or speed out of range");
        if (initialBezier && (initialRaceTime >= 0 || initialStartTime >= 0))
            throw std::invalid_argument("Race timing options require Monza");
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\nUsage: Apex3D [--bezier] [--wheels] [--speed 0.25..64]\n"
                     "  [--driver surname|number] [--camera overview|chase|cockpit] [--zoom 0.008..3] [--race-time seconds]\n"
                     "  [--heatmap speed|off]\n"
                     "  [--screenshot file.ppm |\n"
                     "   --screenshot-at 0..1 file.ppm | --screenshot-time seconds file.ppm |\n"
                     "   --screenshot-start 0..6.5 file.ppm | --screenshot-post 0..600 file.ppm]\n";
        return EXIT_FAILURE;
    }
#ifdef __APPLE__
    if(capture)glfwInitHint(GLFW_COCOA_MENUBAR,GLFW_FALSE);
#endif
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
    if(capture)glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES,4);
    glfwWindowHint(GLFW_DEPTH_BITS,24);
    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight,
                                          "Apex3D - Monza OpenGL prototype",
                                          nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Failed to create the Apex3D window.\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    WindowInput input;
    input.focus=initialFocus;input.heat=initialHeat;input.fieldPage=apex3d::cp3::driverIndex(initialFocus)/3;
    input.camera.mode=initialCamera;input.camera.reset();
    input.camera.zoom=std::clamp(initialZoom,initialCamera==apex3d::three::CameraMode::Overview?0.008:0.18,3.0);
    glfwGetCursorPos(window, &input.x, &input.y);
    glfwSetWindowUserPointer(window, &input);
    glfwSetCursorPosCallback(window, [](GLFWwindow* target, double x, double y) {
        auto* state = static_cast<WindowInput*>(glfwGetWindowUserPointer(target));
        if(state->scrubbing){
            int w=0;glfwGetWindowSize(target,&w,nullptr);
            const auto layout=apex3d::three::controlBarLayout(state->hudVisible);
            if(w>0)state->pendingBar={apex3d::three::BarHit::Timeline,
                std::clamp((2.0*x/w-1.0-layout.lineLeft)/(layout.lineRight-layout.lineLeft),0.0,1.0)};
        }
        else if(state->dragging)state->camera.orbit(x-state->x,y-state->y);
        state->x = x; state->y = y;
    });
    glfwSetMouseButtonCallback(window,[](GLFWwindow* target,int button,int action,int) {
        auto* state=static_cast<WindowInput*>(glfwGetWindowUserPointer(target));
        if(button!=GLFW_MOUSE_BUTTON_LEFT)return;
        int w=0,h=0;glfwGetWindowSize(target,&w,&h);
        if(action==GLFW_RELEASE)state->scrubbing=false;
        if(action==GLFW_PRESS&&w>0&&h>0&&state->raceMode){
            const auto bar=apex3d::three::controlBarAt(2.0*state->x/w-1.0,1.0-2.0*state->y/h,state->hudVisible);
            if(bar.hit!=apex3d::three::BarHit::None){
                state->pendingBar=bar;state->scrubbing=bar.hit==apex3d::three::BarHit::Timeline;
                state->dragging=false;return;
            }
        }
        if(action==GLFW_PRESS&&w>0&&h>0)
            state->pendingAction=apex3d::three::overlayAction(2.0*state->x/w-1.0,1.0-2.0*state->y/h,state->hudVisible,state->fieldPage);
        state->dragging=action==GLFW_PRESS && w>0 &&
            (!state->hudVisible || state->x<0.715*w);
    });
    glfwSetScrollCallback(window,[](GLFWwindow* target,double,double steps) {
        auto* state=static_cast<WindowInput*>(glfwGetWindowUserPointer(target));
        int w=0,h=0;glfwGetWindowSize(target,&w,&h);
        if(w>0&&state->hudVisible&&state->x>=0.715*w&&h>0&&state->y>0.88*h)
            state->fieldPage=(state->fieldPage+(steps<0?1:6))%7;
        else if(w>0&&(!state->hudVisible||state->x<0.715*w))state->camera.dolly(steps);
    });
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
    glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
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

    const auto monzaPath = apex3d::cp3::makePath();
    const auto bezierPath = apex3d::cp3::makeBezierPath();
    const auto initialRoute = initialBezier ? apex3d::cp3::Route::BezierDemo
        : apex3d::cp3::Route::Monza;
    const auto& path = initialBezier ? bezierPath : monzaPath;
    const apex3d::cp3::RaceMotion race(monzaPath);
    apex3d::cp3::TrafficReplay traffic(monzaPath,race);
    apex3d::three::Renderer renderer;
    if(!renderer.initialize()) {
        std::cerr<<"Failed to initialize the 3D renderer.\n";
        renderer.destroy();glDeleteProgram(shaderProgram);
        glfwDestroyWindow(window);glfwTerminate();return EXIT_FAILURE;
    }
    renderer.setWorld(apex3d::three::makeWorld(path,initialBezier?nullptr:&race),
        apex3d::three::makeAnchors(initialRoute));
    GlMesh hud;
    hud.create({},GL_DYNAMIC_DRAW);
    std::cout << "3D route: " << apex3d::cp3::routeName(initialRoute) << " | " << path.size() << " path samples\n";

    auto renderedRoute = initialRoute;
    Playback playback;
    playback.route = initialRoute;
    playback.speed = initialSpeed;
    playback.showWheels = showWheels;
    if (initialBezier) playback.playing = true;
    if (initialProgress >= 0.0) {
        if (initialBezier) { playback.progress = initialProgress; playback.travelled = initialProgress * path.back().distance; }
        else playback.seek(initialProgress * apex3d::cp3::raceLaps.front().milliseconds() / 1000.0);
    }
    if (initialRaceTime >= 0.0) playback.seek(initialRaceTime);
    if (initialStartTime >= 0.0) {
        playback.started = true;
        playback.startElapsed = initialStartTime;
    }
    if(initialPostTime>=0)playback.postRaceElapsed=initialPostTime;
    if (capture) playback.playing = false;
    apex3d::audio::StartAudio audio;
    const bool audioAvailable = !capture && audio.start();
    double previousTime = glfwGetTime();
    double lastTitleTime = -1.0;
    bool screenshotSaved = true;
    double previousRaceTime=playback.elapsed;
    int heatLap=-1;bool heatHasData=false;
    auto heatLayer=apex3d::cp3::HeatLayer::Off;
    auto heatFocus=input.focus;
    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        const double now = glfwGetTime();
        const double deltaSeconds = std::max(now - previousTime, 0.0);
        previousTime = now;
        glfwPollEvents();
        const int previousLights = playback.redLights();
        processInput(window, playback, audioAvailable);
        const bool wasStarting = playback.starting();
        const auto& activePath = playback.route == apex3d::cp3::Route::Monza
            ? monzaPath : bezierPath;
        if (renderedRoute != playback.route) {
            renderer.setWorld(apex3d::three::makeWorld(activePath,playback.isRace()?&race:nullptr),
                apex3d::three::makeAnchors(playback.route));
            input.camera.initialized=false;
            renderedRoute = playback.route;
        }
        playback.advance(deltaSeconds, activePath.back().distance);
        input.raceMode=playback.isRace();
        {
            // Rebuild the coloured road only when the driver, lap or layer changes.
            const int lap=playback.isRace()?apex3d::cp3::heatmapLap(input.focus,playback.simulationSeconds()):0;
            const auto layer=playback.isRace()?input.heat:apex3d::cp3::HeatLayer::Off;
            if(lap!=heatLap||layer!=heatLayer||input.focus!=heatFocus){
                const auto values=apex3d::cp3::lapProfile(monzaPath,
                    traffic.motion(static_cast<std::size_t>(apex3d::cp3::driverIndex(input.focus))),input.focus,lap,layer);
                heatHasData=std::any_of(values.begin(),values.end(),[](double v){return std::isfinite(v);});
                renderer.setHeatmap(apex3d::three::heatmapMesh(monzaPath,values,layer));
                heatLap=lap;heatLayer=layer;heatFocus=input.focus;
            }
        }
        // Pre-compute replay spacing a few frames at a time so timeline jumps stay smooth.
        if(!capture&&playback.isRace())traffic.warm(24);
        audio.mute(playback.muted);
        if (playback.isRace() && playback.playing) {
            if (wasStarting && !playback.starting()) audio.trigger(2);
            else if (playback.redLights() > previousLights) audio.trigger(1);
        }
        auto samples=traffic.sample(playback.simulationSeconds());
        const auto& raceSample=samples.front();
        const auto& focusSample=samples[apex3d::cp3::driverIndex(input.focus)];
        if (now - lastTitleTime >= 0.2) {
            updateTitle(window, playback,input.focus);
            lastTitleTime = now;
        }

        const auto pose = playback.isRace() ? raceSample.pose :
            apex3d::cp3::poseAtProgress(activePath, playback.progress);
        const double wheelTurns=playback.isRace()?raceSample.travelledMetres/apex3d::cp3::raceTyreCircumference:
            playback.travelled*apex3d::physical::metresPerSceneUnit()/apex3d::cp3::raceTyreCircumference;
        auto carVertices=apex3d::three::makeCar(pose,wheelTurns,
            playback.isRace()&&playback.elapsed>=apex3d::cp3::pitBoxEndSeconds,
            playback.isRace()&&raceSample.inPit ? apex3d::three::pitSteering(race,raceSample.progress) :
                apex3d::three::metricSteering(activePath,playback.isRace()?raceSample.progress:playback.progress),
            {0.10F,0.23F,0.52F},{0.95F,0.74F,0.15F},apex3d::cp3::compoundColor(
                apex3d::cp3::tyreState(apex3d::cp3::DriverFocus::Verstappen,playback.elapsed).stint->compound));
        if(!playback.isRace()&&playback.scaleDemo) {
            const double scale=1.0+0.22*std::sin(playback.elapsed*3.0);
            const auto centre=apex3d::three::world(pose.position,apex3d::physical::roadHeight);
            for(auto& v:carVertices){v.x=static_cast<float>(centre.x+(v.x-centre.x)*scale);
                v.y=static_cast<float>(centre.y+(v.y-centre.y)*scale);v.z=static_cast<float>(centre.z+(v.z-centre.z)*scale);}
        }
        apex3d::three::Mesh crew;
        if(playback.isRace()) {
            carVertices.clear();
            for(std::size_t i=0;i<samples.size();++i){const auto& car=samples[i];if(!car.visible)continue;
                const auto& driver=apex3d::cp3::raceDrivers[i];
                const auto tyre=apex3d::cp3::tyreState(driver.focus,playback.simulationSeconds());
                double steering=apex3d::three::metricSteering(monzaPath,car.progress);
                if(car.inPit){
                    double shift=-37,box=37;
                    for(const auto& pit:apex3d::cp3::archivedPits)if(pit.driver==driver.archive->number&&
                        playback.elapsed>=pit.entry&&playback.elapsed<pit.exit){
                        shift=pit.lap-37;box=37+pit.boxMetres/apex3d::cp3::circuitMetres;
                    }
                    if(driver.archive->dnf&&playback.elapsed>=driver.archive->retireEntry){
                        shift=driver.totalLaps-37;box=37+driver.archive->retireBox/apex3d::cp3::circuitMetres;
                    }
                    steering=apex3d::three::pitSteering(race,car.progress-shift,box);
                }
                const auto vertices=apex3d::three::makeCar(car.pose,car.travelledMetres/apex3d::cp3::raceTyreCircumference,
                    false,steering,
                    driver.archive->color,i%2?apex3d::cp3::Color{0.95F,0.74F,0.15F}:apex3d::cp3::Color{0.90F,0.25F,0.30F},
                    apex3d::cp3::compoundColor(tyre.stint->compound),static_cast<int>(driver.focus),
                    car.cooldown||car.inPit?0:apex3d::cp3::drsOpening(static_cast<int>(driver.focus),playback.simulationSeconds()),
                    driver.focus==input.focus);
                carVertices.insert(carVertices.end(),vertices.begin(),vertices.end());
                if(car.servicing){const auto team=apex3d::three::makeCrew(car.pose,true,driver.archive->color);
                    crew.insert(crew.end(),team.begin(),team.end());}
            }
        }
        auto sceneEffects=playback.isRace()?apex3d::three::gantry(activePath,playback.redLights(),false):apex3d::three::Mesh{};
        if(playback.isRace()){const auto birds=apex3d::three::birds(monzaPath,playback.simulationSeconds()+playback.startElapsed);
            sceneEffects.insert(sceneEffects.end(),birds.begin(),birds.end());}
        renderer.update(carVertices,sceneEffects,crew);

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) {
            continue;
        }
        const float aspect = static_cast<float>(width) / static_cast<float>(height);
        const int sceneWidth=input.hudVisible?static_cast<int>(width*0.715):width;
        const bool snap=capture||std::abs(playback.elapsed-previousRaceTime)>2.0;
        const auto camera=input.camera.frame(playback.isRace()?focusSample.pose:pose,static_cast<double>(sceneWidth)/height,deltaSeconds,snap);
        previousRaceTime=playback.elapsed;
        renderer.draw(camera,width,height,sceneWidth,playback.showControls);
        auto overlay=apex3d::three::overlay(playback,focusSample,activePath.back().distance,aspect,audioAvailable,
            input.camera.mode,input.hudVisible,hoveredAction(window),input.focus,input.fieldPage);
        if(input.camera.mode==apex3d::three::CameraMode::Overview&&(!playback.isRace()||focusSample.visible)) {
            const double fraction=static_cast<double>(sceneWidth)/width;
            const auto& driver=apex3d::cp3::driverInfo(input.focus);
            apex3d::three::overviewMarker(overlay,camera,playback.isRace()?focusSample.pose:pose,fraction,aspect,
                playback.isRace()?apex3d::cp3::driverLabel(input.focus,playback.simulationSeconds()):"BEZIER",driver.color,true);
        }
        if(playback.isRace()){
            const double buffered=traffic.cachedFraction()*(apex3d::cp3::raceTotalSeconds+apex3d::cp3::postRaceDuration)/apex3d::cp3::raceTotalSeconds;
            apex3d::three::controlBar(overlay,playback,input.focus,input.hudVisible,buffered);
            apex3d::three::heatLegend(overlay,input.heat,input.focus,heatLap,heatHasData);
        }
        hud.update(overlay,GL_DYNAMIC_DRAW);
        glUseProgram(shaderProgram);
        glUniform2f(glGetUniformLocation(shaderProgram,"viewportScale"),1,1);
        glUniform2f(glGetUniformLocation(shaderProgram,"viewportOffset"),0,0);
        hud.draw();
        if (capture) {
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

    renderer.destroy();
    hud.destroy();
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    return screenshotSaved ? EXIT_SUCCESS : EXIT_FAILURE;
}
