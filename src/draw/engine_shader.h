#ifndef ENGINE_SHADER_H
#define ENGINE_SHADER_H

#include <stdint.h>

enum engine_shader_op_codes{
    SHADER_OPACITY_BLEND,    // blend the next two `bg` bytes to the subsequent `fg` bytes
    SHADER_RGB_INTERPOLATE,  // Interpolate `fg` to the color stored in the next two bytes using the 4 bytes after as the bytes containing a float from 0.0 to 1.0
};

typedef struct engine_shader_t{
    uint8_t program[UINT8_MAX];                         // Shader program consisting of op codes from `engine_shader_op_codes` (256 max codes allowed)
    uint8_t program_len;                                // Length of `program` in bytes
    void (*execute)(struct engine_shader_t *shader);    // Function to execute the bytes/op codes in `program` (sometimes switched out for speed if some effects are not used)
}engine_shader_t;

#endif  // ENGINE_SHADER_H