#include "file.h"

#include "core/logger.h"
#include <cstdio>

bool file_read_blob(const char* path, std::vector<uint8_t>* out_data) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        log_error("Unable to open file %s.", path);
        return false;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    clearerr(file);
    fseek(file, 0L, SEEK_SET);

    *out_data = std::vector<uint8_t>(file_size);
    size_t bytes_read = fread(out_data->data(), 1, file_size, file);
    if (bytes_read != file_size) {
        log_error("Error reading file %s: bytes_read was %zu, but file size is %zu", path, bytes_read, file_size);
        return false;
    }

    fclose(file);
    return true;
}
