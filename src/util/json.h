#pragma once

#include <cstddef>

enum JsonType {
    JSON_TYPE_NULL,
    JSON_TYPE_BOOLEAN,
    JSON_TYPE_NUMBER,
    JSON_TYPE_STRING,
    JSON_TYPE_ARRAY,
    JSON_TYPE_OBJECT
};

struct Json;

struct JsonBoolean {
    bool value;
};

struct JsonNumber {
    double value;
};

struct JsonString {
    char* value;
    size_t length;
};

struct JsonArray {
    Json** values;
    size_t length;
    size_t capacity;
};

struct JsonObject {
    char** keys;
    Json** values;
    size_t length;
    size_t capacity;
};

struct Json {
    JsonType type;
    union {
        JsonBoolean boolean;
        JsonNumber number;
        JsonString string;
        JsonArray array;
        JsonObject object;
    };
};

Json* json_create_null();
Json* json_create_boolean(bool value);
Json* json_create_number(double value);
Json* json_create_string(const char* value);
Json* json_create_object();
Json* json_create_array();

void json_destroy(Json* json);

Json* json_object_get(const Json* json, const char* key);

Json* json_object_set(Json* json, const char* key, Json* value);
Json* json_object_set_boolean(Json* json, const char* key, bool value);
Json* json_object_set_number(Json* json, const char* key, double value);
Json* json_object_set_string(Json* json, const char* key, const char* value);

Json* json_array_get(const Json* json, size_t index);

Json* json_array_push(Json* json, Json* value);
Json* json_array_push_boolean(Json* json, bool value);
Json* json_array_push_number(Json* json, double value);
Json* json_array_push_string(Json* json, const char* value);

Json* json_array_set(Json* json, size_t index, Json* value);
Json* json_array_set_boolean(Json* json, size_t index, bool value);
Json* json_array_set_number(Json* json, size_t index, double value);
Json* json_array_set_string(Json* json, size_t index, const char* value);

bool json_write(const Json* json, const char* path);
Json* json_read(const char* path);
