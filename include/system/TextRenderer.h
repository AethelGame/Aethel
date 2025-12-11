#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <string>
#include <map>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

enum TextAlignment {
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_CENTER,
    TEXT_ALIGN_RIGHT
};

enum Alignment {
    ALIGN_LEFT,
    ALIGN_CENTER,
    ALIGN_RIGHT,
    ALIGN_TOP,
    ALIGN_MIDDLE,
    ALIGN_BOTTOM
};

struct Character {
    GLuint textureID;
    glm::ivec2 size;
    glm::ivec2 bearing;
    unsigned int advance;
};

struct FontKey {
    std::string path;
    int size;
    
    bool operator<(const FontKey& other) const {
        if (path != other.path) return path < other.path;
        return size < other.size;
    }
};

struct FontData {
    std::map<char, Character> characters;
    FT_Face face;
};

class TextRenderer {
public:
    TextRenderer(int screenWidth, int screenHeight);
    ~TextRenderer();
    
    bool loadFont(const std::string& fontPath, int fontSize);
    void renderText(const std::string& text, float x, float y, float scale, 
                   const glm::vec4& color, const std::string& fontPath, int fontSize, 
                   float lineGap = 0.0f, TextAlignment alignment = TEXT_ALIGN_LEFT);
    void getTextSize(const std::string& text, float scale, float& width, float& height,
                    const std::string& fontPath, int fontSize, float lineGap = 0.0f);
    
    void setViewport(int width, int height);
    
private:
    bool initFreeType();
    bool compileShaders();
    void setupBuffers();
    
    FT_Library ft_;
    std::map<FontKey, FontData> fonts_; 
    
    GLuint shaderProgram_;
    GLuint VAO_, VBO_;
    glm::mat4 projection_;
    int screenWidth_, screenHeight_;
    
    FontData* getFontData(const std::string& fontPath, int fontSize);
};

class TextObject {
public:
    TextObject(TextRenderer* renderer, const std::string& fontPath, int fontSize);
    ~TextObject();
    
    void setText(const std::string& text);
    void setPosition(float x, float y);
    void setColor(float r, float g, float b, float a = 1.0f);
    void setScale(float scale) { scale_ = scale; updateDimensions(); }
    void setTextGap(float gap) { textGap_ = gap; }

    void setTextAlignment(TextAlignment alignment) {
        textAlignment_ = alignment;
    }
    void setAlignment(Alignment horizontal, Alignment vertical) {
        alignmentX_ = horizontal;
        alignmentY_ = vertical;
    }
    
    void render();
    bool hitTest(float x, float y) const;
    
    float getRenderedWidth() const { return cachedWidth_; }
    float getRenderedHeight() const { return cachedHeight_; }
    void getPosition(float& x, float& y) const;
    
    float getAnchorX() const { return anchorX_; }
    float getAnchorY() const { return anchorY_; }

    void updateDimensions();
private:
    void calculateRenderPosition(float& renderX, float& renderY) const;
    
    TextRenderer* renderer_;
    std::string fontPath_;
    int fontSize_;
    std::string text_;
    
    float posX_, posY_;
    float anchorX_, anchorY_;
    glm::vec4 color_;
    float scale_;
    float textGap_;
    
    TextAlignment textAlignment_;
    Alignment alignmentX_;
    Alignment alignmentY_;
    
    float cachedWidth_;
    float cachedHeight_;
};

#endif