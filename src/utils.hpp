#ifndef UTILS_HPP
#define UTILS_HPP

#ifndef PI
#define PI 3.14159265358979323846f
#endif

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
};

#endif