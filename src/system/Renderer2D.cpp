#include <cmath>
#include <iostream>

#include "system/Renderer2D.h"
#include "system/Logger.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;

out vec4 ourColor;
uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    ourColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
in vec4 ourColor;
out vec4 FragColor;

void main() {
    FragColor = ourColor;
}
)";

const char* textureVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;
uniform mat4 projection;

void main() {
    gl_Position = projection * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* textureFragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D texture1;
uniform vec4 tintColor;

void main() {
    vec4 texColor = texture(texture1, TexCoord);
    FragColor = texColor * tintColor;
}
)";

Renderer2D::Renderer2D() 
    : shaderProgram_(0), textureShaderProgram_(0), VAO_(0), VBO_(0), EBO_(0),
      screenWidth_(0), screenHeight_(0), currentColor_(1.0f, 1.0f, 1.0f, 1.0f) {
}

Renderer2D::~Renderer2D() {
    if (VAO_) glDeleteVertexArrays(1, &VAO_);
    if (VBO_) glDeleteBuffers(1, &VBO_);
    if (EBO_) glDeleteBuffers(1, &EBO_);
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    if (textureShaderProgram_) glDeleteProgram(textureShaderProgram_);
}

bool Renderer2D::initialize(int width, int height) {
    screenWidth_ = width;
    screenHeight_ = height;
    
    if (!compileShaders()) {
        return false;
    }
    
    setupBuffers();
    setViewport(width, height);
    
    return true;
}

GLuint Renderer2D::loadTexture(const std::string& filepath, bool flipVertically) {
    stbi_set_flip_vertically_on_load(flipVertically);
    
    int width, height, channels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        GAME_LOG_ERROR("Failed to load texture: " + filepath);
        return 0;
    }
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // Determine format based on channels
    GLenum format = GL_RGB;
    if (channels == 1) {
        format = GL_RED;
    } else if (channels == 3) {
        format = GL_RGB;
    } else if (channels == 4) {
        format = GL_RGBA;
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    stbi_image_free(data);
    
    GAME_LOG_INFO("Loaded texture: " + filepath + " (" + std::to_string(width) + "x" + 
                  std::to_string(height) + ", " + std::to_string(channels) + " channels)");
    
    return textureID;
}

void Renderer2D::unloadTexture(GLuint textureID) {
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
}

void Renderer2D::drawTextureFullscreen(GLuint textureID, const Color& tint) {
    drawTexture(textureID, 0, 0, (float)screenWidth_, (float)screenHeight_, tint);
}

bool Renderer2D::compileShaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex Shader Compilation Failed:\n" << infoLog << std::endl;
        return false;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment Shader Compilation Failed:\n" << infoLog << std::endl;
        return false;
    }
    
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);
    
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram_, 512, nullptr, infoLog);
        std::cerr << "Shader Program Linking Failed:\n" << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    GLuint texVertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(texVertexShader, 1, &textureVertexShaderSource, nullptr);
    glCompileShader(texVertexShader);
    
    GLuint texFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(texFragmentShader, 1, &textureFragmentShaderSource, nullptr);
    glCompileShader(texFragmentShader);
    
    textureShaderProgram_ = glCreateProgram();
    glAttachShader(textureShaderProgram_, texVertexShader);
    glAttachShader(textureShaderProgram_, texFragmentShader);
    glLinkProgram(textureShaderProgram_);
    
    glDeleteShader(texVertexShader);
    glDeleteShader(texFragmentShader);
    
    return true;
}

void Renderer2D::setupBuffers() {
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    glGenBuffers(1, &EBO_);
    
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 1000, nullptr, GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 1000, nullptr, GL_DYNAMIC_DRAW);
    
    glBindVertexArray(0);
}

void Renderer2D::setViewport(int width, int height) {
    screenWidth_ = width;
    screenHeight_ = height;
    
    projection_ = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
    
    glViewport(0, 0, width, height);
}

void Renderer2D::clear(const Color& color) {
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer2D::drawRect(float x, float y, float width, float height, const Color& color) {
    float vertices[] = {
        x, y,            color.r, color.g, color.b, color.a,
        x + width, y,    color.r, color.g, color.b, color.a,
        x + width, y + height, color.r, color.g, color.b, color.a,
        x, y + height,   color.r, color.g, color.b, color.a
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    glUseProgram(shaderProgram_);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "projection"), 1, GL_FALSE, &projection_[0][0]);
    
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer2D::drawRectOutline(float x, float y, float width, float height, float thickness, const Color& color) {
    drawRect(x, y, width, thickness, color);
    drawRect(x, y + height - thickness, width, thickness, color);
    drawRect(x, y, thickness, height, color);
    drawRect(x + width - thickness, y, thickness, height, color);
}

void Renderer2D::drawLine(float x1, float y1, float x2, float y2, float thickness, const Color& color) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float length = std::sqrt(dx * dx + dy * dy);
    
    if (length < 0.001f) return;
    
    float nx = -dy / length;
    float ny = dx / length;
    
    float halfThickness = thickness * 0.5f;
    
    float vertices[] = {
        x1 + nx * halfThickness, y1 + ny * halfThickness, color.r, color.g, color.b, color.a,
        x2 + nx * halfThickness, y2 + ny * halfThickness, color.r, color.g, color.b, color.a,
        x2 - nx * halfThickness, y2 - ny * halfThickness, color.r, color.g, color.b, color.a,
        x1 - nx * halfThickness, y1 - ny * halfThickness, color.r, color.g, color.b, color.a
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    glUseProgram(shaderProgram_);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "projection"), 1, GL_FALSE, &projection_[0][0]);
    
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer2D::drawCircle(float x, float y, float radius, const Color& color, int segments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(color.r);
    vertices.push_back(color.g);
    vertices.push_back(color.b);
    vertices.push_back(color.a);
    
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float px = x + radius * std::cos(angle);
        float py = y + radius * std::sin(angle);
        
        vertices.push_back(px);
        vertices.push_back(py);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        vertices.push_back(color.a);
    }
    
    for (int i = 1; i <= segments; i++) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i + 1);
    }
    
    glUseProgram(shaderProgram_);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "projection"), 1, GL_FALSE, &projection_[0][0]);
    
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(unsigned int), indices.data());
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer2D::drawTexture(GLuint textureID, float x, float y, float width, float height, const Color& tint) {
    float vertices[] = {
        x, y,               0.0f, 0.0f,
        x + width, y,       1.0f, 0.0f,
        x + width, y + height, 1.0f, 1.0f,
        x, y + height,      0.0f, 1.0f
    };
    
    unsigned int indices[] = {
        0, 1, 2,
        2, 3, 0
    };
    
    glUseProgram(textureShaderProgram_);
    glUniformMatrix4fv(glGetUniformLocation(textureShaderProgram_, "projection"), 1, GL_FALSE, &projection_[0][0]);
    glUniform4f(glGetUniformLocation(textureShaderProgram_, "tintColor"), tint.r, tint.g, tint.b, tint.a);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glUniform1i(glGetUniformLocation(textureShaderProgram_, "texture1"), 0);
    
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}