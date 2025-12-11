#define _USE_MATH_DEFINES

#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <fstream>
#include <cmath>
#include <sys/stat.h>

#include <utils/Utils.h>
#include <system/Logger.h>

namespace Utils
{
    std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (std::string::npos == first)
            return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    std::vector<std::string> split(const std::string &s, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter))
        {
            tokens.push_back(token);
        }
        return tokens;
    }

    std::string formatMemorySize(size_t bytes)
    {
        const char *sizes[] = {"B", "KB", "MB", "GB", "TB"};
        int order = 0;
        double size = static_cast<double>(bytes);
        while (size >= 1024 && order < 4)
        {
            order++;
            size = size / 1024;
        }
        std::ostringstream oss;
        oss.precision(2);
        oss << std::fixed << size << " " << sizes[order];
        return oss.str();
    }

    bool fileExists(const std::string &path)
    {
#ifdef _WIN32
        struct _stat buffer;
        return (_stat(path.c_str(), &buffer) == 0);
#else
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
#endif
    }

    bool hasEnding(std::string const &fullString, std::string const &ending)
    {
        if (fullString.length() >= ending.length())
        {
            return std::equal(ending.rbegin(), ending.rend(), fullString.rbegin(), [](char a, char b)
                              { return std::tolower(a) == std::tolower(b); });
        }
        return false;
    }

    std::string readFile(const std::string &fullPath)
    {
        std::ifstream t(fullPath);
        if (!t.is_open())
            return "";

        std::stringstream buffer;
        buffer << t.rdbuf();
        return buffer.str();
    }

    double pNorm(const std::vector<double> &values, const std::vector<double> &weights, double P)
    {
        if (values.empty())
            return 0.0;
        double sum = 0.0;
        for (size_t i = 0; i < values.size(); ++i)
        {
            sum += std::pow(values[i] * weights[i], P);
        }
        return std::pow(sum, 1.0 / P);
    }

    double calculateStandardDeviation(const std::vector<double> &array)
    {
        if (array.size() <= 1)
            return 0.0;
        double sum = std::accumulate(array.begin(), array.end(), 0.0);
        double mean = sum / array.size();
        double sq_sum = 0.0;
        for (double val : array)
            sq_sum += std::pow(val - mean, 2);
        return std::sqrt(sq_sum / array.size());
    }

    bool loadWindowIcon(GLFWwindow* window, const std::string& iconPath) {
        int width, height, channels;

        stbi_set_flip_vertically_on_load(false);
        unsigned char* pixels = stbi_load(iconPath.c_str(), &width, &height, &channels, 4);
        
        if (!pixels) {
            GAME_LOG_ERROR("Failed to load window icon: " + iconPath);
            return false;
        }
        
        GLFWimage icon;
        icon.width = width;
        icon.height = height;
        icon.pixels = pixels;
        
        glfwSetWindowIcon(window, 1, &icon);
        stbi_image_free(pixels);
        
        GAME_LOG_INFO("Window icon loaded successfully: " + iconPath);
        return true;
    }
}