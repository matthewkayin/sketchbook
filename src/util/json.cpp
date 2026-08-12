#include "json.h"

#include "core/logger.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdarg>

Json* json_create_null() {
    Json* json = (Json*)malloc(sizeof(Json));
    if (!json) {
        log_error("Failed to malloc Json.");
        return NULL;
    }

    json->type = JSON_TYPE_NULL;
    return json;
}

Json* json_create_boolean(bool value) {
    Json* json = (Json*)malloc(sizeof(Json));
    if (!json) {
        log_error("Failed to malloc Json.");
        return NULL;
    }

    json->type = JSON_TYPE_BOOLEAN;
    json->boolean.value = value;
    return json;
}

Json* json_create_number(double value) {
    Json* json = (Json*)malloc(sizeof(Json));
    if (!json) {
        log_error("Failed to malloc Json.");
        return NULL;
    }

    json->type = JSON_TYPE_NUMBER;
    json->number.value = value;
    return json;
}

Json* json_create_string(const char* value) {
    Json* json = (Json*)malloc(sizeof(Json));
    if (!json) {
        log_error("Failed to malloc Json.");
        return NULL;
    }

    json->type = JSON_TYPE_STRING;
    json->string.length = strlen(value);
    json->string.value = (char*)malloc(json->string.length + 1);
    if (!json->string.value) {
        log_error("Failed to malloc Json string.");
        json_destroy(json);
        return NULL;
    }
    strcpy(json->string.value, value);

    return json;
}

Json* json_create_object() {
    Json* json = (Json*)malloc(sizeof(Json));
    if (!json) {
        log_error("Failed to malloc Json.");
        return NULL;
    }

    json->type = JSON_TYPE_OBJECT;
    json->object.capacity = 1;
    json->object.length = 0;

    json->object.keys = (char**)malloc(json->object.capacity * sizeof(char*));
    if (!json->object.keys) {
        log_error("Failed to malloc Json object keys.");
        free(json);
        return NULL;
    }

    json->object.values = (Json**)malloc(json->object.capacity * sizeof(Json*));
    if (!json->object.values) {
        log_error("Failed to malloc Json object values.");
        free(json->object.keys);
        free(json);
        return NULL;
    }

    return json;
}

Json* json_create_array() {
    Json* json = (Json*)malloc(sizeof(Json));
    if (!json) {
        log_error("Failed to malloc Json.");
        return NULL;
    }

    json->type = JSON_TYPE_ARRAY;
    json->array.capacity = 1;
    json->array.length = 0;
    json->array.values = (Json**)malloc(json->array.capacity * sizeof(Json*));
    if (!json->array.values) {
        log_error("Failed to malloc Json array values.");
        free(json);
        return NULL;
    }

    return json;
}

void json_destroy(Json* json) {
    if (!json) {
        return;
    }

    if (json->type == JSON_TYPE_OBJECT) {
        for (size_t index = 0; index < json->object.length; index++) {
            free(json->object.keys[index]);
            json_destroy(json->object.values[index]);
        }
        free(json->object.keys);
        free(json->object.values);
    } else if (json->type == JSON_TYPE_ARRAY) {
        for (size_t index = 0; index < json->array.length; index++) {
            json_destroy(json->array.values[index]);
        }
        free(json->array.values);
    }

    free(json);
}

Json* json_object_get(const Json* json, const char* key) {
    if (!json) {
        log_error("Called json_object_get with key %s on a null json.", key);
        return NULL;
    }
    if (json->type != JSON_TYPE_OBJECT) {
        log_error("Called json_object_get with key %s on a non-object json.", key);
        return NULL;
    }

    for (size_t index = 0; index < json->object.length; index++) {
        if (strcmp(json->object.keys[index], key) == 0) {
            return json->object.values[index];
        }
    }

    return NULL;
}

Json* json_object_set(Json* json, const char* key, Json* value) {
    if (!json) {
        log_error("Called json_object_set key %s on a null json.", key);
        return NULL;
    }
    if (json->type != JSON_TYPE_OBJECT) {
        log_error("Called json_object_set key %s on a non-object json.", key);
        return NULL;
    }

    size_t index;
    for (index = 0; index < json->object.length; index++) {
        if (strcmp(json->object.keys[index], key) == 0) {
            break;
        }
    }

    // If existing entry is not found, create new entry
    if (index == json->object.length) {
        // Resize capacity if necessary
        if (json->object.length + 1 > json->object.capacity) {
            size_t new_capacity = json->object.capacity * 2;

            char** new_keys = (char**)malloc(new_capacity * sizeof(char*));
            if (!new_keys) {
                log_error("Failed to malloc new keys array when resizing Json object.");
                return NULL;
            }

            Json** new_values = (Json**)malloc(new_capacity * sizeof(Json*));
            if (!new_values) {
                free(new_keys);
                log_error("Failed to malloc new values array when resizing Json object.");
                return NULL;
            }

            memcpy(new_keys, json->object.keys, json->object.length * sizeof(char*));
            memcpy(new_values, json->object.values, json->object.length * sizeof(Json*));

            free(json->object.keys);
            free(json->object.values);

            json->object.keys = new_keys;
            json->object.values = new_values;
            json->object.capacity = new_capacity;
        }

        json->object.length++;
        strcpy(json->object.keys[index], key);
        json->object.values[index] = NULL;
    }

    // Regardless of whether we increased length or not, index will now match the key
    json_destroy(json->object.values[index]);
    json->object.values[index] = value;

    return json->object.values[index];
}

Json* json_object_set_boolean(Json* json, const char* key, bool value) {
    Json* value_json = json_create_boolean(value);
    if (!value_json) {
        return NULL;
    }

    Json* result = json_object_set(json, key, value_json);
    if (!result) {
        json_destroy(value_json);
    }

    return result;
}

Json* json_object_set_number(Json* json, const char* key, double value) {
    Json* value_json = json_create_number(value);
    if (!value_json) {
        return NULL;
    }

    Json* result = json_object_set(json, key, value_json);
    if (!result) {
        json_destroy(value_json);
    }

    return result;
}

Json* json_object_set_string(Json* json, const char* key, const char* value) {
    Json* value_json = json_create_string(value);
    if (!value_json) {
        return NULL;
    }

    Json* result = json_object_set(json, key, value_json);
    if (!result) {
        json_destroy(value_json);
    }

    return result;
}

Json* json_array_get(const Json* json, size_t index) {
    if (!json) {
        log_error("Called json_array_get index %zu on a null json.", index);
        return NULL;
    }
    if (json->type != JSON_TYPE_ARRAY) {
        log_error("Called json_array_get index %zu on a non-array json.", index);
        return NULL;
    }
    if (index >= json->array.length) {
        log_error("Called json_array_get index %zu on an array with length %zu.", index, json->array.length);
        return NULL;
    }

    return json->array.values[index];
}

Json* json_array_push(Json* json, Json* value) {
    if (!json) {
        log_error("Called json_array_push on a null json.");
        return NULL;
    }
    if (json->type != JSON_TYPE_ARRAY) {
        log_error("Called json_array_push on a non-array json.");
        return NULL;
    }

    // Reisze capacity if necessary
    if (json->array.length + 1 > json->array.capacity) {
        size_t new_capacity = json->array.capacity * 2;

        Json** new_values = (Json**)malloc(new_capacity * sizeof(Json*));
        if (!new_values) {
            log_error("Failed to malloc new values array when resizing Json array.");
            return NULL;
        }

        memcpy(new_values, json->array.values, json->array.length * sizeof(Json*));

        free(json->array.values);

        json->array.values = new_values;
        json->array.capacity = new_capacity;
    }

    json->array.length++;
    json->array.values[json->array.length - 1] = value;

    return json->array.values[json->array.length - 1];
}

Json* json_array_push_boolean(Json* json, bool value) {
    Json* value_json = json_create_boolean(value);
    if (!value_json) {
        return NULL;
    }

    Json* result = json_array_push(json, value_json);
    if (!result) {
        json_destroy(value_json);
    }

    return result;
}

Json* json_array_push_number(Json* json, double value) {
    Json* value_json = json_create_number(value);
    if (!value_json) {
        return NULL;
    }

    Json* result = json_array_push(json, value_json);
    if (!result) {
        json_destroy(value_json);
    }

    return result;
}

Json* json_array_push_string(Json* json, const char* value) {
    Json* value_json = json_create_string(value);
    if (!value_json) {
        return NULL;
    }

    Json* result = json_array_push(json, value_json);
    if (!result) {
        json_destroy(value_json);
    }

    return result;
}

Json* json_array_set(Json* json, size_t index, Json* value) {
    if (!json) {
        log_error("Called json_array_set index %zu on a null json.", index);
        return NULL;
    }
    if (json->type != JSON_TYPE_ARRAY) {
        log_error("Called json_array_set index %zu on a non-array json.", index);
        return NULL;
    }
    if (index >= json->array.length) {
        log_error("Called json_array_set index %zu on a Json array with length %zu.", index, json->array.length);
        return NULL;
    }

    json_destroy(json->array.values[index]);
    json->array.values[index] = value;
    return json->array.values[index];
}

Json* json_array_set_boolean(Json* json, size_t index, bool value) {
    Json* value_json = json_create_boolean(value);
    if (!value_json) {
        return NULL;
    }

    Json* result = json_array_set(json, index, value_json);
    if (!result) {
        json_destroy(value_json);
    }

    return result;
}

Json* json_array_set_number(Json* json, size_t index, double value) {
    Json* value_json = json_create_number(value);
    if (!value_json) {
        return NULL;
    }

    Json* result = json_array_set(json, index, value_json);
    if (!result) {
        json_destroy(value_json);
    }

    return result;
}

Json* json_array_set_string(Json* json, size_t index, const char* value) {
    Json* value_json = json_create_string(value);
    if (!value_json) {
        return NULL;
    }

    Json* result = json_array_set(json, index, value_json);
    if (!result) {
        json_destroy(value_json);
    }

    return result;
}

void json_fprintf(FILE* file, const Json* json, size_t depth) {
    switch (json->type) {
        case JSON_TYPE_NULL: {
            fprintf(file, "null");
            break;
        }
        case JSON_TYPE_BOOLEAN: {
            fprintf(file, json->boolean.value ? "true" : "false");
            break;
        }
        case JSON_TYPE_NUMBER: {
            const bool is_integer = (double)((int)json->number.value) == json->number.value;
            if (is_integer) {
                fprintf(file, "%i", (int)json->number.value);
            } else {
                fprintf(file, "%f", json->number.value);
            }
            break;
        }
        case JSON_TYPE_STRING: {
            fprintf(file, "\"%s\"", json->string.value);
            break;
        }
        case JSON_TYPE_ARRAY: {
            if (json->array.length == 0) {
                fprintf(file, "[]");
                break;
            }

            fprintf(file, "[\n");

            // For each element
            for (size_t index = 0; index < json->array.length; index++) {
                // Print tabs equal to depth
                for (size_t depth_index = 0; depth_index < depth; depth_index++) {
                    fprintf(file, "\t");
                }

                // Print the element itself
                json_fprintf(file, json_array_get(json, index), depth + 1);

                // Print trailing comma
                if (index < json->array.length - 1) {
                    fprintf(file, ",");
                }
                fprintf(file, "\n");
            }

            // Print closing square bracket tabbed to depth - 1
            for (size_t depth_index = 0; depth_index < depth - 1; depth_index++) {
                fprintf(file, "\t");
            }
            fprintf(file, "]");

            break;
        }
        case JSON_TYPE_OBJECT: {
            if (json->object.length == 0) {
                fprintf(file, "{}");
                break;
            }

            fprintf(file, "{\n");

            for (size_t index = 0; index < json->object.length; index++) {
                // Print tabs equal to depth
                for (size_t depth_index = 0; depth_index < depth; depth_index++) {
                    fprintf(file, "\t");
                }

                // Print key
                fprintf(file, "\"%s\": ", json->object.keys[index]);

                // Print value
                json_fprintf(file, json->object.values[index], depth + 1);

                if (index < json->object.length - 1) {
                    fprintf(file, ",");
                }
                fprintf(file, "\n");
            }

            // Print closing curly bracket tabbed to depth - 1
            for (size_t depth_index = 0; depth_index < depth - 1; depth_index++) {
                fprintf(file, "\t");
            }
            fprintf(file, "}");

            break;
        }
    }
}

bool json_write(const Json* json, const char* path) {
    FILE* file = fopen(path, "w");
    if (!file) {
        log_error("Failed to open JSON file %s for writing.", path);
        return false;
    }

    json_fprintf(file, json, 1);
    fclose(file);

    return true;
}

bool json_char_is_whitespace(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\n':
        case '\v':
        case '\f':
        case '\r':
            return true;
        default:
            return false;
    }
}

#define JSON_PARSE_CURSOR_ERROR_BUFFER_SIZE 512

struct JsonParseCursor {
    const char* str_ptr;
    size_t line_number;
    size_t char_number;
    char error_buffer[JSON_PARSE_CURSOR_ERROR_BUFFER_SIZE];
};

Json* json_parse(JsonParseCursor* cursor);

void json_parse_set_error(JsonParseCursor* cursor, const char* message, ...) {
    char* error_ptr = cursor->error_buffer;
    error_ptr += sprintf(error_ptr, "JSON parse error on line %zu, char %zu: ",
        cursor->line_number,
        cursor->char_number);

    __builtin_va_list arg_ptr;
    va_start(arg_ptr, message);
    vsprintf(error_ptr, message, arg_ptr);
    va_end(arg_ptr);
}

void json_parse_step(JsonParseCursor* cursor, size_t amount) {
    cursor->str_ptr += amount;
    cursor->char_number += amount;
}

void json_parse_skip_to_next_non_whitespace_character(JsonParseCursor* cursor) {
    while (!json_char_is_whitespace(*(cursor->str_ptr)) && *(cursor->str_ptr) != '\0') {
        if (*(cursor->str_ptr) == '\n') {
            cursor->line_number++;
            cursor->char_number = 0;
        }

        json_parse_step(cursor, 1);
    }
}

bool json_parse_consume_string(JsonParseCursor* cursor, const char* str) {
    const char* str_ptr = str;
    while (*str_ptr != '\0') {
        if (*(cursor->str_ptr) != *str_ptr) {
            json_parse_set_error(cursor, "Unexpected token '%u'", *(cursor->str_ptr));
            return false;
        }

        json_parse_step(cursor, 1);
        str_ptr++;
    }

    return true;
}

Json* json_parse_null(JsonParseCursor* cursor) {
    if (!json_parse_consume_string(cursor, "null")) {
        return NULL;
    }

    return json_create_null();
}

Json* json_parse_true(JsonParseCursor* cursor) {
    if (!json_parse_consume_string(cursor, "true")) {
        return NULL;
    }

    return json_create_boolean(true);
}

Json* json_parse_false(JsonParseCursor* cursor) {
    if (!json_parse_consume_string(cursor, "false")) {
        return NULL;
    }

    return json_create_boolean(false);
}

bool json_char_marks_end_of_number(char c) {
    switch (c) {
        case '\0':
        case ',':
        case '}':
        case ']':
            return true;
        default:
            return false;
    }
}

Json* json_parse_number(JsonParseCursor* cursor) {
    const char* previous_str_ptr = cursor->str_ptr;

    // Check that each character in the number is numeric or a period
    bool has_encountered_period = false;
    while (!json_char_marks_end_of_number(*(cursor->str_ptr))) {
        if (*(cursor->str_ptr) == '.') {
            if (has_encountered_period) {
                json_parse_set_error(cursor, "Unexpected token '.'");
                return NULL;
            }
            has_encountered_period = true;
        } else if (*(cursor->str_ptr) < '0' || *(cursor->str_ptr) > '9') {
            json_parse_set_error(cursor, "Unexpected token '%c'", *(cursor->str_ptr));
            return NULL;
        }

        json_parse_step(cursor, 1);
    }

    double value = strtod(previous_str_ptr, NULL);
    return json_create_number(value);
}

bool json_parse_string_and_get_pointer(JsonParseCursor* cursor, const char** out_str_ptr, size_t* out_length) {
    // Check that the string is enclosed with quotes and count the string length
    if (*(cursor->str_ptr) != '"') {
        json_parse_set_error(cursor, "Unexpected token '%c'", *(cursor->str_ptr));
        return false;
    }
    json_parse_step(cursor, 1);
    const char* string_value_ptr = cursor->str_ptr;

    // Step until we hit the end quote
    while (*(cursor->str_ptr) != '"') {
        if (*(cursor->str_ptr) == '\0') {
            json_parse_set_error(cursor, "Unexpected null-terminator.");
            return false;
        }
        json_parse_step(cursor, 1);
    }

    if (out_str_ptr) {
        *out_str_ptr = string_value_ptr;
    }
    if (out_length) {
        *out_length = cursor->str_ptr - string_value_ptr;
    }

    // Step past the end quote
    json_parse_step(cursor, 1);

    return true;
}

Json* json_parse_string(JsonParseCursor* cursor) {
    const char* str_ptr;
    size_t length;
    if (!json_parse_string_and_get_pointer(cursor, &str_ptr, &length)) {
        return NULL;
    }

    Json* json = (Json*)malloc(sizeof(Json));
    if (!json) {
        log_error("Failed to malloc Json.");
        return NULL;
    }

    json->type = JSON_TYPE_STRING;
    json->string.length = length;

    json->string.value = (char*)malloc(length + 1);
    memcpy(json->string.value, str_ptr, length);
    json->string.value[length] = '\0';

    return json;
}

Json* json_parse_array(JsonParseCursor* cursor) {
    if (*(cursor->str_ptr) != '[') {
        json_parse_set_error(cursor, "Unexpected token '%c'", *(cursor->str_ptr));
        return NULL;
    }
    json_parse_step(cursor, 1);

    Json* json_array = json_create_array();
    while (*(cursor->str_ptr) != ']') {
        // Error if null terminator before closing brace
        if (*(cursor->str_ptr) == '\0') {
            json_parse_set_error(cursor, "Unexpected null-terminator");
            json_destroy(json_array);
            return NULL;
        }

        // Consume past whitespace
        if (json_char_is_whitespace(*(cursor->str_ptr))) {
            json_parse_step(cursor, 1);
            continue;
        }

        // Parse and push the item
        Json* json_array_item = json_parse(cursor);
        if (!json_array_item) {
            json_destroy(json_array);
            return NULL;
        }
        json_array_push(json_array, json_array_item);

        // Consume past comma
        if (*(cursor->str_ptr) == ',') {
            json_parse_step(cursor, 1);
        }
    }

    // Consume the closing brace
    json_parse_step(cursor, 1);

    return json_array;
}

Json* json_parse_object(JsonParseCursor* cursor) {
    if (*(cursor->str_ptr) != '{') {
        json_parse_set_error(cursor, "Unexpected token '%c'", *(cursor->str_ptr));
        return NULL;
    }
    json_parse_step(cursor, 1);

    Json* json_object = json_create_object();
    const size_t KEY_BUFFER_SIZE = 512;
    char key[KEY_BUFFER_SIZE];
    bool has_parsed_key = false;
    bool has_parsed_colon = false;
    while (*(cursor->str_ptr) != '}') {
        // Error if null terminator beore closing brace
        if (*(cursor->str_ptr) == '\0') {
            json_parse_set_error(cursor, "Unexpected null-terminator");
            json_destroy(json_object);
            return NULL;
        }

        // Consume past whitespace
        if (json_char_is_whitespace(*(cursor->str_ptr))) {
            json_parse_step(cursor, 1);
            continue;
        }

        // Parse key
        if (!has_parsed_key) {
            const char* key_ptr;
            size_t key_length;
            if (!json_parse_string_and_get_pointer(cursor, &key_ptr, &key_length)) {
                json_destroy(json_object);
                return NULL;
            }

            memcpy(key, key_ptr, key_length);
            key[key_length] = '\0';

            has_parsed_key = true;
            continue;
        }

        // Parse colon
        if (!has_parsed_colon) {
            if (*(cursor->str_ptr) != ':') {
                json_parse_set_error(cursor, "Unexpected token '%c'", *(cursor->str_ptr));
                json_destroy(json_object);
                return NULL;
            }

            json_parse_step(cursor, 1);
            has_parsed_colon = true;
            continue;
        }

        // Parse value
        Json* value = json_parse(cursor);
        if (!value) {
            json_destroy(json_object);
            return NULL;
        }

        // Consume past comma
        if (*(cursor->str_ptr) == ',') {
            json_parse_step(cursor, 1);
        }

        // Set key/value
        json_object_set(json_object, key, value);
        has_parsed_key = false;
        has_parsed_colon = false;
    }

    // Consume the closing brace
    json_parse_step(cursor, 1);

    return json_object;
}

Json* json_parse(JsonParseCursor* cursor) {
    if (*(cursor->str_ptr) == '{') {
        return json_parse_object(cursor);
    } else if (*(cursor->str_ptr) == '[') {
        return json_parse_array(cursor);
    } else if (*(cursor->str_ptr) == '\"') {
        return json_parse_string(cursor);
    } else if (*(cursor->str_ptr) == 't') {
        return json_parse_true(cursor);
    } else if (*(cursor->str_ptr) == 'f') {
        return json_parse_false(cursor);
    } else if (*(cursor->str_ptr) == 'n') {
        return json_parse_null(cursor);
    } else {
        json_parse_set_error(cursor, "Unexpected token '%c'", *(cursor->str_ptr));
        return NULL;
    }
}

Json* json_read(const char* path) {
    // Open file
    FILE* file = fopen(path, "r");
    if (!file) {
        log_error("Failed to open JSON file %s for reading.", path);
        return NULL;
    }

    // Seek to end
    if (fseek(file, 0, SEEK_END) != 0) {
        log_error("Error seeking to end of JSON file %s.", path);
        fclose(file);
        return NULL;
    }

    // Get file size
    size_t file_size = ftell(file);
    if (file_size < 0) {
        log_error("Error obtaining size of JSON file %s.", path);
        fclose(file);
        return NULL;
    }

    // Alloc file contents buffer
    char* file_contents = (char*)malloc(file_size + 1);
    if (!file_contents) {
        log_error("Failed to alloc buffer for JSON file %s size %zu.", path, file_size + 1);
        fclose(file);
        return NULL;
    }

    // Rewind file
    rewind(file);

    // Read into contents buffer
    size_t bytes_read = fread(file_contents, 1, file_size, file);
    if (bytes_read != file_size) {
        log_error("Error reading JSON file %s. Read %zu bytes into a buffer but expected %zu.", bytes_read, file_size);
        free(file_contents);
        fclose(file);
        return NULL;
    }

    // Null terminate
    file_contents[bytes_read] = '\0';
    fclose(file);

    JsonParseCursor cursor;
    cursor.str_ptr = file_contents;
    cursor.line_number = 0;
    cursor.char_number = 0;
    cursor.error_buffer[0] = '\0';

    Json* result = json_parse(&cursor);
    if (!result && cursor.error_buffer[0] != '\0') {
        log_error("%s", cursor.error_buffer);
    }

    free(file_contents);

    return result;
}
