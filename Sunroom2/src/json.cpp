#include <Arduino.h>
#include <map>

String escapeString(String str) {
    String result;
    for (char c : str) {
        switch (c) {
            case '\"':
                result += "\\\"";
                break;
            case '\n':
                result += "\\n";
                break;
            default:
                result += c;
                break;
        }
    }
    return result;
}

String buildJson(std::map<String, String> data) {
    String json = "{";
    for (auto const &entry : data) {
        json += "\"" + escapeString(entry.first) + "\":\"" + escapeString(entry.second) + "\",";
    }
    json = json.substring(0, json.length() - 1);
    json += "}";
    return json;
}
