#pragma once
#include "hiface/hiface_c_api.h"
#include <stdexcept>
#include <string>
namespace hiface {
class Error : public std::runtime_error {
public:
    HiFaceStatus status;
    Error(HiFaceStatus value, const std::string& operation);
};
void check(HiFaceStatus status, const char* operation);
// Process-wide C ABI callback; caller owns context and callback lifetime.
inline void setLogCallback(HiFaceLogCallback callback, void* context = nullptr) {
    hiface_set_log_callback_v1(callback, context);
}
}
