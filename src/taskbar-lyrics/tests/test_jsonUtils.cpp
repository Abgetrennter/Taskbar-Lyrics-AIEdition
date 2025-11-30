#include "../utils/jsonUtils.hpp"
#include <iostream>
#include <cassert>

void test_getString()
{
    std::string json = "{\"name\": \"Taskbar Lyrics\", \"version\": \"1.0.0\"}";
    assert(JsonUtils::getString(json, "name") == "Taskbar Lyrics");
    assert(JsonUtils::getString(json, "version") == "1.0.0");
    assert(JsonUtils::getString(json, "missing") == "");
    std::cout << "test_getString passed" << std::endl;
}

void test_getInt()
{
    std::string json = "{\"count\": 42, \"negative\": -10}";
    assert(JsonUtils::getInt(json, "count") == 42);
    assert(JsonUtils::getInt(json, "negative") == -10);
    assert(JsonUtils::getInt(json, "missing") == 0);
    std::cout << "test_getInt passed" << std::endl;
}

void test_getFloat()
{
    std::string json = "{\"opacity\": 0.85, \"pi\": 3.14159}";
    float opacity = JsonUtils::getFloat(json, "opacity");
    assert(opacity > 0.84 && opacity < 0.86);
    std::cout << "test_getFloat passed" << std::endl;
}

void test_getBool()
{
    std::string json = "{\"enabled\": true, \"visible\": false}";
    assert(JsonUtils::getBool(json, "enabled") == true);
    assert(JsonUtils::getBool(json, "visible") == false);
    assert(JsonUtils::getBool(json, "missing") == false);
    std::cout << "test_getBool passed" << std::endl;
}

int main()
{
    test_getString();
    test_getInt();
    test_getFloat();
    test_getBool();
    std::cout << "All tests passed!" << std::endl;
    return 0;
}
