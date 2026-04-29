#ifndef UTILS_HPP
#define UTILS_HPP

#ifndef PI
#define PI 3.14159265358979323846f
#endif

#include <string>
#include <raylib.h>
#include <raymath.h>

#include <vector>

class Utils {
   public:
    static float ConvertAngleToRadial(float angle) {
        return angle * PI / 180;
    }
    
    static std::string Vector3ToString(const Vector3 &vec) {
        return "(" + std::to_string(vec.x) + ", " +
               std::to_string(vec.y) + ", " +
               std::to_string(vec.z) + ")";
    }

    static std::string ResolveProjectPath(const std::string& relativePath) {
        if (FileExists(relativePath.c_str()) || DirectoryExists(relativePath.c_str())) {
            return relativePath;
        }

        const char* appDir = GetApplicationDirectory();
        std::string appBase = appDir != nullptr ? std::string(appDir) : std::string();
        std::string candidates[] = {
            appBase + "/" + relativePath,
            appBase + "/../" + relativePath,
            appBase + "/../../" + relativePath
        };

        for (std::size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); ++i) {
            if (FileExists(candidates[i].c_str()) || DirectoryExists(candidates[i].c_str())) {
                return candidates[i];
            }
        }

        return relativePath;
    }

    static std::string ResolveProjectRootPath() {
        if (DirectoryExists("resources")) {
            return ".";
        }

        const char* appDir = GetApplicationDirectory();
        std::string appBase = appDir != nullptr ? std::string(appDir) : std::string();
        std::string candidates[] = {
            appBase + "/..",
            appBase + "/../.."
        };

        for (std::size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); ++i) {
            if (DirectoryExists((candidates[i] + "/resources").c_str())) {
                return candidates[i];
            }
        }

        return ".";
    }

    static std::string ResolveWritableProjectPath(const std::string& relativePath) {
        return ResolveProjectRootPath() + "/" + relativePath;
    }
};

#endif
