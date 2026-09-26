#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

struct TrackPoint {
    float x;
    float y;
};

// A reference-derived Monza centerline for the geometry milestone. The points
// are ordered around the lap so GL_LINE_LOOP can join and close the circuit.
// They remain deliberately hand-authored; telemetry will replace them later.
constexpr TrackPoint kMonzaCenterline[] = {
    // Start/finish straight and Rettifilo chicane (turns 1-2).
    { 1.16F, -0.68F}, { 0.82F, -0.69F}, { 0.45F, -0.69F},
    { 0.08F, -0.68F}, {-0.25F, -0.68F}, {-0.48F, -0.67F},
    {-0.50F, -0.64F}, {-0.50F, -0.58F}, {-0.53F, -0.55F},
    {-0.59F, -0.56F}, {-0.68F, -0.61F},

    // Curva Grande and the run toward the Roggia chicane (turns 3-5).
    {-0.80F, -0.65F}, {-0.94F, -0.63F}, {-1.07F, -0.57F},
    {-1.18F, -0.47F}, {-1.25F, -0.34F}, {-1.30F, -0.18F},
    {-1.33F,  0.00F}, {-1.35F,  0.18F}, {-1.39F,  0.23F},
    {-1.46F,  0.24F}, {-1.49F,  0.28F}, {-1.50F,  0.35F},

    // Lesmo curves (turns 6-7).
    {-1.54F,  0.49F}, {-1.58F,  0.63F}, {-1.58F,  0.70F},
    {-1.55F,  0.76F}, {-1.49F,  0.80F}, {-1.39F,  0.83F},
    {-1.25F,  0.85F}, {-1.10F,  0.88F}, {-0.95F,  0.90F},
    {-0.87F,  0.88F}, {-0.81F,  0.81F},

    // Long descending run from Lesmo 2 to Variante Ascari (turns 8-10).
    {-0.72F,  0.68F}, {-0.61F,  0.51F}, {-0.51F,  0.35F},
    {-0.39F,  0.20F}, {-0.25F,  0.06F}, {-0.10F, -0.08F},
    { 0.05F, -0.21F}, { 0.18F, -0.32F}, { 0.26F, -0.38F},
    { 0.33F, -0.39F}, { 0.40F, -0.36F}, { 0.48F, -0.35F},
    { 0.56F, -0.37F}, { 0.63F, -0.42F}, { 0.70F, -0.44F},

    // Back straight and Curva Alboreto/Parabolica (turn 11).
    { 0.91F, -0.45F}, { 1.14F, -0.46F}, { 1.36F, -0.46F},
    { 1.51F, -0.47F}, { 1.59F, -0.50F}, { 1.64F, -0.56F},
    { 1.66F, -0.63F}, { 1.65F, -0.69F}, { 1.61F, -0.75F},
    { 1.54F, -0.80F}, { 1.43F, -0.84F}, { 1.30F, -0.86F},
    { 1.17F, -0.87F},
};

constexpr GLsizei kTrackPointCount =
    static_cast<GLsizei>(sizeof(kMonzaCenterline) / sizeof(kMonzaCenterline[0]));

bool gShowControlPoints = true;

constexpr char kVertexShaderSource[] = R"(
    #version 330 core

    layout (location = 0) in vec2 position;
    uniform vec2 viewportScale;

    void main()
    {
        gl_Position = vec4(position * viewportScale, 0.0, 1.0);
    }
)";

constexpr char kFragmentShaderSource[] = R"(
    #version 330 core

    uniform vec3 drawColor;
    out vec4 fragmentColor;

    void main()
    {
        fragmentColor = vec4(drawColor, 1.0);
    }
)";

void framebufferSizeCallback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    static bool pWasPressed = false;
    const bool pIsPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    if (pIsPressed && !pWasPressed) {
        gShowControlPoints = !gShowControlPoints;
    }
    pWasPressed = pIsPressed;
}

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
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, kVertexShaderSource);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, kFragmentShaderSource);

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

} // namespace

int main()
{
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

    GLFWwindow* window = glfwCreateWindow(
        kWindowWidth,
        kWindowHeight,
        "Apex3D - Monza Centerline Milestone",
        nullptr,
        nullptr
    );

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

    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);

    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(kMonzaCenterline),
        kMonzaCenterline,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TrackPoint), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    const GLint viewportScaleLocation = glGetUniformLocation(shaderProgram, "viewportScale");
    const GLint drawColorLocation = glGetUniformLocation(shaderProgram, "drawColor");

    glEnable(GL_DEPTH_TEST);
    GLfloat supportedLineWidths[2]{};
    glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, supportedLineWidths);
    glLineWidth(std::min(2.0F, supportedLineWidths[1]));
    glPointSize(6.0F);

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        processInput(window);

        glClearColor(0.035F, 0.035F, 0.045F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(vertexArray);

        int framebufferWidth = 0;
        int framebufferHeight = 0;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        float xScale = 1.0F;
        float yScale = 1.0F;
        if (framebufferWidth > framebufferHeight && framebufferWidth > 0) {
            xScale = static_cast<float>(framebufferHeight) /
                static_cast<float>(framebufferWidth);
        } else if (framebufferHeight > 0) {
            yScale = static_cast<float>(framebufferWidth) /
                static_cast<float>(framebufferHeight);
        }
        glUniform2f(viewportScaleLocation, xScale, yScale);

        // LINE_LOOP joins every point and closes the final segment automatically.
        glUniform3f(drawColorLocation, 0.94F, 0.94F, 0.96F);
        glDrawArrays(GL_LINE_LOOP, 0, kTrackPointCount);

        // The red control points reveal the raw geometry used to build the line.
        if (gShowControlPoints) {
            glUniform3f(drawColorLocation, 0.95F, 0.05F, 0.03F);
            glDrawArrays(GL_POINTS, 0, kTrackPointCount);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vertexArray);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}
