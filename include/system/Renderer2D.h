#ifndef RENDERER2D_H
#define RENDERER2D_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>

struct Color {
    float r, g, b, a;
    
    Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}
    
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : r(r / 255.0f), g(g / 255.0f), b(b / 255.0f), a(a / 255.0f) {}
};

struct Rect {
    float x;
    float y;
    float width;
    float height;
};

class Renderer2D {
public:
    Renderer2D();
    ~Renderer2D();
    
    bool initialize(int width, int height);
    void setViewport(int width, int height);
    void clear(const Color& color);
    
    void drawRect(float x, float y, float width, float height, const Color& color);
    void drawRectOutline(float x, float y, float width, float height, float thickness, const Color& color);
    void drawLine(float x1, float y1, float x2, float y2, float thickness, const Color& color);
    void drawCircle(float x, float y, float radius, const Color& color, int segments = 32);
    
    GLuint loadTexture(const std::string& filepath, bool flipVertically = true);
    void unloadTexture(GLuint textureID);
    void drawTexture(GLuint textureID, float x, float y, float width, float height, 
                    const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f));
    
    void drawTextureFullscreen(GLuint textureID, const Color& tint = Color(1.0f, 1.0f, 1.0f, 1.0f));
    
private:
    bool compileShaders();
    void setupBuffers();
    
    GLuint shaderProgram_;
    GLuint textureShaderProgram_;
    GLuint VAO_, VBO_, EBO_;
    glm::mat4 projection_;
    int screenWidth_, screenHeight_;
    Color currentColor_;
};

#endif