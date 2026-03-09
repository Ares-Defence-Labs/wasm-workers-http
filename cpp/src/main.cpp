#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main() {
    json j = {
        {"status", 200},
        {"message", "hello"}
    };
    return 0;
}