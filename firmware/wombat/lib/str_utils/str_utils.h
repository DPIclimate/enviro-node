#ifndef STR_UTILS_H
#define STR_UTILS_H
#include <stddef.h>

namespace wombat {
    const char *stripTrailingZeros(const float value);
    size_t stripLeadingWS(char *str);
    size_t stripTrailingWS(char *str);
    size_t stripWS(char *str);

    bool get_cp_destination(const char *input_ptr, const size_t input_len, int &dest, const char **dest_filename_ptr, size_t &dest_output_len);
}
#endif //STR_UTILS_H
