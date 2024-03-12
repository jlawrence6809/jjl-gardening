#include <Arduino.h>
#include <map>

String escapeString(String str)
{
    String result;
    for (char c : str)
    {
        switch (c)
        {
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

String buildJson(std::map<String, String> data)
{
    String json = "{";
    for (auto const &entry : data)
    {

        json += "\"" + escapeString(entry.first) + "\":\"" + escapeString(entry.second) + "\",";
    }
    json = json.substring(0, json.length() - 1);
    json += "}";
    return json;
}

// // type enum
// enum Type
// {
//     STRING,
//     NUMBER,
//     OBJECT,
//     ARRAY,
//     BOOLEAN
// };

// struct JsonPrimitive
// {
//     Type type;
//     String strValue;
//     float numValue;
//     bool boolValue;
//     std::map<String, JsonPrimitive> objectValue;
//     std::vector<JsonPrimitive> arrayValue;
//     int currentIndex;
// };

// JsonPrimitive createString(String value) {
//     return {STRING, value};
// }

// JsonPrimitive createNumber(float value) {
//     return {NUMBER, "", value};
// }

// JsonPrimitive createBool(bool value) {
//     return {BOOLEAN, "", 0, value};
// }

// JsonPrimitive createObject(std::map<String, JsonPrimitive> value) {
//     return {OBJECT, "", 0, false, value};
// }

// JsonPrimitive createArray(std::vector<JsonPrimitive> value) {
//     return {ARRAY, "", 0, false, {}, value};
// }

// /**
//  * Simple recursive json parser, only works for simple json objects/arrays
// */
// JsonPrimitive parseJson(String json, int currentIndex)
// {
//     // check first character
//     // if it's a {, then it's an object
//     // else if it's a [, then it's an array
//     // else if it's a ", then it's a string
//     // else if it's true/false then it's a boolean
//     // else if it's a number, then it's a number (use float)

//     // skip any whitespace:
//     while (json.charAt(currentIndex) == ' ')
//     {
//         currentIndex++;
//     }

//     char firstChar = json.charAt(currentIndex++);

//     switch (firstChar)
//     {
//     case '{':
//     {
//         std::map<String, JsonPrimitive> object;
//         while (json.charAt(currentIndex) != '}')
//         {
//             // // skip whitespace and quotes
//             // while (json.charAt(currentIndex) == ' ' || json.charAt(currentIndex) == '\"')
//             // {
//             //     currentIndex++;
//             // }
//             // get key
//             int keyStart = currentIndex;

//             while (json.charAt(currentIndex) != ':')
//             {
//                 currentIndex++;
//             }
//             String key = json.substring(keyStart, currentIndex);
//             // remove whitespace and quotes:
//             key.replace(" ", "");
//             key.replace("\"", "");

//             currentIndex++; // skip the :
//             JsonPrimitive value = parseJson(json, currentIndex);
//             object[key] = value;
//             currentIndex++; // skip the value
//             while (json.charAt(currentIndex) != ',' && json.charAt(currentIndex) != '}')
//             {
//                 currentIndex++;
//             }
//             if (json.charAt(currentIndex) == ',')
//             {
//                 currentIndex++;
//             }
//         }
//     }

// }