#include <stdio.h>
#include "mathlib.h"
#include <cjson/cJSON.h>

int main() {
    // 1. Test Mathlib
    int sum = add_numbers(10, 20);
    printf("Mathlib Test: 10 + 20 = %d\n", sum);

    // 2. Test cJSON
    const char *json_string = "{\"name\":\"BeagleBone\", \"status\":\"active\"}";
    cJSON *json = cJSON_Parse(json_string);
    if (json != NULL) {
        printf("cJSON Test - Name: %s\n", cJSON_GetObjectItem(json, "name")->valuestring);
        cJSON_Delete(json);
    }
    return 0;
}

