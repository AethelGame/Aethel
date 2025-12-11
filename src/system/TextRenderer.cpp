#include "system/TextRenderer.h"
#include "system/Logger.h"
#include <iostream>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>

const char* textVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec4 vertex;
out vec2 TexCoords;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

const char* textFragmentShaderSource = R"(
#version 330 core
in vec2 TexCoords;
out vec4 color;
uniform sampler2D text;
uniform vec4 textColor;
void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = textColor * sampled;
}
)";

TextRenderer::TextRenderer(int screenWidth, int screenHeight)
    : screenWidth_(screenWidth), screenHeight_(screenHeight), ft_(nullptr),
      shaderProgram_(0), VAO_(0), VBO_(0) {
    
    if (!initFreeType()) {
        std::cerr << "Failed to initialize FreeType" << std::endl;
    }
    
    if (!compileShaders()) {
        std::cerr << "Failed to compile text shaders" << std::endl;
    }
    
    setupBuffers();
    setViewport(screenWidth, screenHeight);
}

TextRenderer::~TextRenderer() {
    // Clean up all fonts
    for (auto& pair : fonts_) {
        for (auto& charPair : pair.second.characters) {
            glDeleteTextures(1, &charPair.second.textureID);
        }
        if (pair.second.face) {
            FT_Done_Face(pair.second.face);
        }
    }
    fonts_.clear();
    
    if (VAO_) glDeleteVertexArrays(1, &VAO_);
    if (VBO_) glDeleteBuffers(1, &VBO_);
    if (shaderProgram_) glDeleteProgram(shaderProgram_);
    if (ft_) FT_Done_FreeType(ft_);
}

bool TextRenderer::initFreeType() {
    if (FT_Init_FreeType(&ft_)) {
        std::cerr << "Could not init FreeType Library" << std::endl;
        return false;
    }
    return true;
}

bool TextRenderer::loadFont(const std::string& fontPath, int fontSize) {
    FontKey key = {fontPath, fontSize};
    
    // If already loaded, skip
    if (fonts_.find(key) != fonts_.end()) {
        GAME_LOG_DEBUG("Font already loaded: " + fontPath + " size " + std::to_string(fontSize));
        return true;
    }
    
    FontData fontData;
    fontData.face = nullptr;
    
    if (FT_New_Face(ft_, fontPath.c_str(), 0, &fontData.face)) {
        GAME_LOG_ERROR("Failed to load font: " + fontPath);
        return false;
    }
    
    if (!fontData.face) {
        GAME_LOG_ERROR("Font face is null: " + fontPath);
        return false;
    }
    
    FT_Set_Pixel_Sizes(fontData.face, 0, fontSize);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    // Load ASCII characters
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(fontData.face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load glyph: " << c << std::endl;
            continue;
        }
        
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_RED,
            fontData.face->glyph->bitmap.width,
            fontData.face->glyph->bitmap.rows,
            0, GL_RED, GL_UNSIGNED_BYTE,
            fontData.face->glyph->bitmap.buffer
        );
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        Character character = {
            texture,
            glm::ivec2(fontData.face->glyph->bitmap.width, fontData.face->glyph->bitmap.rows),
            glm::ivec2(fontData.face->glyph->bitmap_left, fontData.face->glyph->bitmap_top),
            (unsigned int)fontData.face->glyph->advance.x
        };
        
        fontData.characters.insert(std::pair<char, Character>(c, character));
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    fonts_[key] = fontData;
    GAME_LOG_DEBUG("Font loaded successfully: " + fontPath + " size " + std::to_string(fontSize));
    
    return true;
}

FontData* TextRenderer::getFontData(const std::string& fontPath, int fontSize) {
    FontKey key = {fontPath, fontSize};
    auto it = fonts_.find(key);
    
    if (it == fonts_.end()) {
        // Try to load it
        if (!loadFont(fontPath, fontSize)) {
            return nullptr;
        }
        it = fonts_.find(key);
    }
    
    return (it != fonts_.end()) ? &it->second : nullptr;
}

bool TextRenderer::compileShaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &textVertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Text Vertex Shader Failed:\n" << infoLog << std::endl;
        return false;
    }
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &textFragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Text Fragment Shader Failed:\n" << infoLog << std::endl;
        return false;
    }
    
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);
    
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram_, 512, nullptr, infoLog);
        std::cerr << "Text Shader Linking Failed:\n" << infoLog << std::endl;
        return false;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

void TextRenderer::setupBuffers() {
    glGenVertexArrays(1, &VAO_);
    glGenBuffers(1, &VBO_);
    
    glBindVertexArray(VAO_);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void TextRenderer::setViewport(int width, int height) {
    screenWidth_ = width;
    screenHeight_ = height;
    projection_ = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
}

void TextRenderer::renderText(const std::string& text, float x, float y, float scale, 
                              const glm::vec4& color, const std::string& fontPath, int fontSize,
                              float lineGap, TextAlignment alignment) {
    FontData* fontData = getFontData(fontPath, fontSize);
    if (!fontData) {
        GAME_LOG_ERROR("Font not loaded: " + fontPath);
        return;
    }
    
    glUseProgram(shaderProgram_);
    glUniform4f(glGetUniformLocation(shaderProgram_, "textColor"), color.x, color.y, color.z, color.w);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "projection"), 1, GL_FALSE, &projection_[0][0]);
    
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO_);
    
    // For center/right alignment, we need to calculate line widths first
    std::vector<float> lineWidths;
    if (alignment != TEXT_ALIGN_LEFT) {
        float currentLineWidth = 0;
        for (char c : text) {
            if (c == '\n') {
                lineWidths.push_back(currentLineWidth);
                currentLineWidth = 0;
            } else if (fontData->characters.find(c) != fontData->characters.end()) {
                currentLineWidth += (fontData->characters[c].advance >> 6) * scale;
            }
        }
        lineWidths.push_back(currentLineWidth); // Last line
    }
    
    float startX = x;
    int currentLine = 0;
    
    // Apply alignment offset for current line
    if (alignment == TEXT_ALIGN_CENTER && currentLine < lineWidths.size()) {
        x = startX - (lineWidths[currentLine] / 2.0f);
    } else if (alignment == TEXT_ALIGN_RIGHT && currentLine < lineWidths.size()) {
        x = startX - lineWidths[currentLine];
    } else {
        x = startX;
    }
    
    for (char c : text) {
        if (c == '\n') {
            if (fontData->characters.find('H') != fontData->characters.end()) {
                y += (fontData->characters['H'].size.y * scale) + lineGap;
            } else {
                y += (fontSize * scale) + lineGap;
            }
            currentLine++;
            // Recalculate x for next line based on alignment
            if (alignment == TEXT_ALIGN_CENTER && currentLine < lineWidths.size()) {
                x = startX - (lineWidths[currentLine] / 2.0f);
            } else if (alignment == TEXT_ALIGN_RIGHT && currentLine < lineWidths.size()) {
                x = startX - lineWidths[currentLine];
            } else {
                x = startX;
            }
            continue;
        }
        
        if (fontData->characters.find(c) == fontData->characters.end()) {
            continue;
        }
        
        Character ch = fontData->characters[c];
        
        float xpos = x + ch.bearing.x * scale;
        float ypos = y + (fontData->characters['H'].bearing.y - ch.bearing.y) * scale;
        
        float w = ch.size.x * scale;
        float h = ch.size.y * scale;
        
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos,     ypos,       0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 0.0f },
            
            { xpos,     ypos + h,   0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 0.0f },
            { xpos + w, ypos + h,   1.0f, 1.0f }
        };
        
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        x += (ch.advance >> 6) * scale;
    }
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextRenderer::getTextSize(const std::string& text, float scale, float& width, float& height,
                               const std::string& fontPath, int fontSize, float lineGap) {
    FontData* fontData = getFontData(fontPath, fontSize);
    if (!fontData) {
        width = height = 0;
        return;
    }
    
    width = 0;
    height = 0;
    
    float lineWidth = 0;
    float lineHeight = 0;
    int lineCount = 0;
    
    for (char c : text) {
        if (c == '\n') {
            if (lineWidth > width) width = lineWidth;
            height += lineHeight;
            lineCount++;
            lineWidth = 0;
            lineHeight = 0;
            continue;
        }
        
        if (fontData->characters.find(c) == fontData->characters.end()) {
            continue;
        }
        
        Character ch = fontData->characters[c];
        lineWidth += (ch.advance >> 6) * scale;
        
        float charHeight = ch.size.y * scale;
        if (charHeight > lineHeight) lineHeight = charHeight;
    }
    
    if (lineWidth > width) width = lineWidth;
    height += lineHeight;
    
    // Add line gaps (gaps between lines, not after the last line)
    if (lineCount > 0) {
        height += lineGap * lineCount;
    }
}

// TextObject implementation
TextObject::TextObject(TextRenderer* renderer, const std::string& fontPath, int fontSize)
    : renderer_(renderer), fontPath_(fontPath), fontSize_(fontSize),
      posX_(0), posY_(0), anchorX_(0), anchorY_(0),
      color_(1.0f, 1.0f, 1.0f, 1.0f), scale_(1.0f), textGap_(0.0f),
      textAlignment_(TEXT_ALIGN_LEFT), alignmentX_(ALIGN_LEFT), alignmentY_(ALIGN_TOP),
      cachedWidth_(0), cachedHeight_(0) {
    
    // Load font (will be cached if already loaded)
    if (!renderer_->loadFont(fontPath, fontSize)) {
        GAME_LOG_ERROR("Failed to load font in TextObject: " + fontPath);
    }
}

TextObject::~TextObject() {
}

void TextObject::setText(const std::string& newText) {
    if (text_ != newText) {
        text_ = newText;
        updateDimensions();
    }
}

void TextObject::setPosition(float x, float y) {
    anchorX_ = x;
    anchorY_ = y;
}

void TextObject::setColor(float r, float g, float b, float a) {
    color_ = glm::vec4(r, g, b, a);
}

void TextObject::updateDimensions() {
    if (text_.empty()) {
        cachedWidth_ = 0;
        cachedHeight_ = 0;
        return;
    }
    
    renderer_->getTextSize(text_, scale_, cachedWidth_, cachedHeight_, fontPath_, fontSize_, textGap_);
}

void TextObject::calculateRenderPosition(float& renderX, float& renderY) const {
    renderX = anchorX_;
    renderY = anchorY_;
    
    switch (alignmentX_) {
        case ALIGN_LEFT: break;
        case ALIGN_CENTER: renderX -= cachedWidth_ / 2.0f; break;
        case ALIGN_RIGHT: renderX -= cachedWidth_; break;
    }
    
    switch (alignmentY_) {
        case ALIGN_TOP: break;
        case ALIGN_MIDDLE: renderY -= cachedHeight_ / 2.0f; break;
        case ALIGN_BOTTOM: renderY -= cachedHeight_; break;
    }
}

void TextObject::render() {
    if (text_.empty() || !renderer_) return;
    
    float renderX, renderY;
    calculateRenderPosition(renderX, renderY);
    
    posX_ = renderX;
    posY_ = renderY;
    
    renderer_->renderText(text_, renderX, renderY, scale_, color_, fontPath_, fontSize_, textGap_, textAlignment_);
}

bool TextObject::hitTest(float x, float y) const {
    float renderX, renderY;
    calculateRenderPosition(renderX, renderY);
    
    return x >= renderX && y >= renderY && 
           x <= renderX + cachedWidth_ && y <= renderY + cachedHeight_;
}