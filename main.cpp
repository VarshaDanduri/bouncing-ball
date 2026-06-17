#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cmath>

int WIDTH = 800, HEIGHT = 600;

std::string loadShader(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open shader: " << path << std::endl;
        exit(-1);
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}


GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    int ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, NULL, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
        exit(-1);
    }
    return shader;
}


GLuint makeProgram(GLuint vert, GLuint frag) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);

    int ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, NULL, log);
        std::cerr << "Program link error:\n" << log << std::endl;
        exit(-1);
    }
    return prog;
}


GLuint makeStateTexture(int w, float* data) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, 1, 0, GL_RGBA, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}


GLuint makeFBO(GLuint tex) {
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer not complete!" << std::endl;
        exit(-1);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return fbo;
}

int main(int argc, char* argv[]) {


    int NUM_BALLS = (argc > 1) ? std::atoi(argv[1]) : 100;
    std::cout << "Simulating " << NUM_BALLS << " balls" << std::endl;


    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ball Simulation", NULL, NULL);
if (!window) { glfwTerminate(); return -1; }
glfwMakeContextCurrent(window);
glfwSwapInterval(0);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW failed" << std::endl; return -1; }


    if (!glewIsSupported("GL_EXT_color_buffer_float")) {
        std::cerr << "Warning: EXT_color_buffer_float not supported, may still work" << std::endl;
    }

    glfwGetFramebufferSize(window, &WIDTH, &HEIGHT);
    glViewport(0, 0, WIDTH, HEIGHT);


    std::string physVertSrc = loadShader("physics.vert.glsl");
    std::string physFragSrc = loadShader("physics.frag.glsl");
    GLuint physVert = compileShader(GL_VERTEX_SHADER,   physVertSrc.c_str());
    GLuint physFrag = compileShader(GL_FRAGMENT_SHADER, physFragSrc.c_str());
    GLuint physProg = makeProgram(physVert, physFrag);
    glDeleteShader(physVert);
    glDeleteShader(physFrag);


    std::string renVertSrc = loadShader("render.vert.glsl");
    std::string renFragSrc = loadShader("render.frag.glsl");
    GLuint renVert = compileShader(GL_VERTEX_SHADER,   renVertSrc.c_str());
    GLuint renFrag = compileShader(GL_FRAGMENT_SHADER, renFragSrc.c_str());
    GLuint renProg = makeProgram(renVert, renFrag);
    glDeleteShader(renVert);
    glDeleteShader(renFrag);


    std::vector<float> ballData(NUM_BALLS * 4);
    srand(time(NULL));
    float RADIUS = 0.03f;
    for (int i = 0; i < NUM_BALLS; i++) {

        ballData[i*4 + 0] = ((float)rand()/RAND_MAX) * 1.8f - 0.9f; // x
        ballData[i*4 + 1] = ((float)rand()/RAND_MAX) * 1.8f - 0.9f; // y

        float angle = ((float)rand()/RAND_MAX) * 2.0f * 3.14159f;
        float speed = 0.3f + ((float)rand()/RAND_MAX) * 0.4f;
        ballData[i*4 + 2] = cos(angle) * speed; // vx
        ballData[i*4 + 3] = sin(angle) * speed; // vy
    }


    GLuint tex[2];
    tex[0] = makeStateTexture(NUM_BALLS, ballData.data());
    tex[1] = makeStateTexture(NUM_BALLS, nullptr); // empty, will be written to

    GLuint fbo[2];
    fbo[0] = makeFBO(tex[0]);
    fbo[1] = makeFBO(tex[1]);

    int current = 0; 


    float fsQuad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
    };
    GLuint fsVAO, fsVBO;
    glGenVertexArrays(1, &fsVAO);
    glGenBuffers(1, &fsVBO);
    glBindVertexArray(fsVAO);
    glBindBuffer(GL_ARRAY_BUFFER, fsVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(fsQuad), fsQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);


    float unitQuad[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
    };
    GLuint ballVAO, ballVBO;
    glGenVertexArrays(1, &ballVAO);
    glGenBuffers(1, &ballVBO);
    glBindVertexArray(ballVAO);
    glBindBuffer(GL_ARRAY_BUFFER, ballVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(unitQuad), unitQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    double lastTime = glfwGetTime();
    int frameCount = 0;
    double lastFrame = lastTime;


    while (!glfwWindowShouldClose(window)) {

        double now = glfwGetTime();
        float dt  = (float)(now - lastFrame);
        lastFrame = now;
        dt = std::min(dt, 0.016f); 


        frameCount++;
        if (now - lastTime >= 1.0) {
            std::string title = "Ball Sim | Balls: " + std::to_string(NUM_BALLS)
                              + " | FPS: " + std::to_string(frameCount)
                              + " | ms/frame: " + std::to_string(1000/frameCount);
            glfwSetWindowTitle(window, title.c_str());
            frameCount = 0;
            lastTime = now;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, fbo[1 - current]);
        glViewport(0, 0, NUM_BALLS, 1);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT); 

        glUseProgram(physProg);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex[current]);
        glUniform1i(glGetUniformLocation(physProg, "uState"),    0);
        glUniform1i(glGetUniformLocation(physProg, "uNumBalls"), NUM_BALLS);
        glUniform1f(glGetUniformLocation(physProg, "uDelta"),    dt);
        glUniform1f(glGetUniformLocation(physProg, "uRadius"),   RADIUS);

        glBindVertexArray(fsVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        current = 1 - current; 


        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, WIDTH, HEIGHT);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(renProg);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex[current]);
        glUniform1i(glGetUniformLocation(renProg, "uState"),  0);
        glUniform1f(glGetUniformLocation(renProg, "uRadius"), RADIUS);

        glBindVertexArray(ballVAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, NUM_BALLS); // one call, all balls

        glfwSwapBuffers(window);
        glfwPollEvents();
    }


    glDeleteVertexArrays(1, &fsVAO);
    glDeleteBuffers(1, &fsVBO);
    glDeleteVertexArrays(1, &ballVAO);
    glDeleteBuffers(1, &ballVBO);
    glDeleteTextures(2, tex);
    glDeleteFramebuffers(2, fbo);
    glDeleteProgram(physProg);
    glDeleteProgram(renProg);
    glfwTerminate();
    return 0;
}
