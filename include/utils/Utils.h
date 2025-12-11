#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <cmath>
#include <functional>

#include <stb_image.h>
#include <GLFW/glfw3.h>

namespace Utils {
    std::string trim(const std::string& str);
    std::vector<std::string> split(const std::string& s, char delimiter);
    bool hasEnding(std::string const &fullString, std::string const &ending);

    std::string readFile(const std::string& path);
    static bool fileExists(const std::string& path);

    std::string formatMemorySize(size_t bytes);

    double pNorm(const std::vector<double>& values, const std::vector<double>& weights, double P);
    double calculateStandardDeviation(const std::vector<double>& array);

    bool loadWindowIcon(GLFWwindow* window, const std::string& filepath);
}

#endif