#include "str_utils.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <regex>

//
// This file is a project-local platformio library so it can be unit tested.
//
// Do not include anything other than standard C++ headers.
//

namespace wombat {
    /**
     * @brief Returns a pointer to a string representation of value, with trailing zeros removed.
     *
     * The default %f formatter is used to get the initial string representation, so 6 digits after the
     * decimal point. From that, trailing zeros are removed.
     *
     * The pointer returned from this function must not be passed to free.
     * This function is not re-entrant.
     *
     * @param value the value to convert to a string.
     * @return a pointer to a string representation of value, with trailing zeros removed.
     */
    const char *stripTrailingZeros(const float value) {
        static char float_buf[20];

        int len = snprintf(float_buf, sizeof(float_buf)-1, "%f", value);
        len--;
        while (len > 0) {
            if (float_buf[len] != '0') {
                break;
            }

            // Don't strip zeros immediately after the decimal point, ie don't
            // convert '1.0' to '1.'
            if (len > 1 && float_buf[len - 1] == '.') {
                break;
            }

            float_buf[len] = 0;
            len--;
        }

        return float_buf;
    }

    /**
     * @brief Removes leading whitespace characters from str.
     *
     * Any character with a value of less than space is considered to be whitespace.
     *
     * @param str the string whose leading whitespace is stripped.
     * @return the new length of str, or 0 if str is null or a zero-length string.
     */
    size_t stripLeadingWS(char *str) {
        if (str == nullptr || *str == 0) {
            return 0;
        }

        char *p = str;
        while (*p <= ' ') {
            p++;
        }

        char *s = str;
        if (p != str) {
            while (*p != 0) {
                *s++ = *p++;
            }

            *s = 0;
        }

        return strlen(str);
    }

    /**
     * @brief Removes trailing whitespace characters from str.
     *
     * Any character with a value of less than space is considered to be whitespace.
     *
     * @param str the string whose trailing whitespace is stripped.
     * @return the new length of str, or 0 if str is null or a zero-length string.
     */
    size_t stripTrailingWS(char *str) {
        if (str == nullptr || *str == 0) {
            return 0;
        }

        // Strip trailing whitespace. response_buffer + len would point to the trailing null
        // so subtract one from that to get to the last character.
        size_t len = strlen(str);
        if (len == 0) {
            return len;
        }

        char *p = str + len - 1;
        while (p >= str && *p != 0 && *p <= ' ') {
            *p = 0;
            p--;
        }

        // Why 1 is added here: imagine the string is "X"; this means p == msg_buf but the string
        // length is 1, not 0.
        len = p - str + 1;
        return len;
    }

    /**
     * @brief Removes leading and trailing whitespace characters from str.
     *
     * Any character with a value of less than space is considered to be whitespace.
     *
     * @param str the string whose leading and trailing whitespace is stripped.
     * @return the new length of str, or 0 if str is null or a zero-length string.
     */
    size_t stripWS(char *str) {
        stripTrailingWS(str);
        return stripLeadingWS(str);
    }

    /**
     * @brief extract the destination type and filename from a string of the form dest:filename.
     *
     * When copying files between the SD card filesystem, SPIFFS, and the modem, the destination filename must be
     * prefixed with the destination device. Eg r5:msg.txt, sd:msg.json, spiffs:msg.json.
     *
     * @param input_ptr [IN] A pointer to the first character of the dest:filename string.
     * @param input_len [IN] The length of input_ptr, excluding any trailing null.
     * @param dest [OUT] A reference to a file_copy_dest variable.
     * @param dest_filename_ptr [OUT] A pointer to a pointer that will receive the address of the filename portion of dest:filename.
     * @param dest_output_len [OUT] The length of the filename portion of dest:filename
     * @return
     */
    bool get_cp_destination(const char *input_ptr, const size_t input_len, int &dest, const char **dest_filename_ptr, size_t &dest_output_len) {
        dest = 0;

        // dest_val is a temporary var for holding the destination type so dest itself does not need
        // to be set until the filename is known to be ok.
        int dest_val = 0;

        // The shortest valid input is r5:x or sd:x sp input_len must be >= 4.
        if (input_ptr == nullptr || dest_filename_ptr == nullptr || input_len < 4) {
            return false;
        }

        const std::string fname_str(input_ptr, input_len);

        static const std::regex pattern("(r5|sd|spiffs):/?[\\w.:-]+");
        static std::smatch matches;

        // Check if the string matches the regex pattern
        if (! std::regex_match(fname_str, matches, pattern)) {
            return false;
        }

        const auto fs = matches[1];
        if ( ! fs.compare("r5")) {
            dest_val = 1;
        } else if ( ! fs.compare("sd")) {
            dest_val = 2;
        } else {
            // The fs specifier must be valid due to the regex match so it must be spiffs.
            dest_val = 3;
        }

        // The regex has shown the given file specifier meets the pattern so it should be safe to look for the
        // colon and return a pointer to the character after that. The first character of the filename must be
        // after the 3rd character due to the format of the specifier.
        size_t i = 2;
        while (input_ptr[i] != ':') {
            i++;
        }

        i++;
        *dest_filename_ptr = &input_ptr[i];

        dest = dest_val;
        dest_output_len =  input_len - i;
        return true;
    }
}
