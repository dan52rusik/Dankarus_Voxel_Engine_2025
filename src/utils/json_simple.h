#ifndef UTILS_JSON_SIMPLE_H_
#define UTILS_JSON_SIMPLE_H_

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdint>
#ifdef _WIN32
#include <windows.h>
#include <vector>
#endif

namespace json_simple {
    class Value {
    public:
    enum Type {
        NULL_TYPE,
        BOOL,
        NUMBER,
        STRING,
        ARRAY,
        OBJECT
    };
        
        Type type;
        bool boolValue;
        double numberValue;
        std::string stringValue;
        std::vector<Value> arrayValue;
        std::map<std::string, Value> objectValue;
        
        Value() : type(NULL_TYPE) {}
        Value(bool b) : type(BOOL), boolValue(b) {}
        Value(double n) : type(NUMBER), numberValue(n) {}
        Value(int n) : type(NUMBER), numberValue((double)n) {}
        Value(int64_t n) : type(NUMBER), numberValue((double)n) {}
        Value(const std::string& s) : type(STRING), stringValue(s) {}
        Value(const char* s) : type(STRING), stringValue(s) {}
        
        // Array access
        Value& operator[](size_t index) {
            if (type != ARRAY) {
                type = ARRAY;
                arrayValue.clear();
            }
            if (index >= arrayValue.size()) {
                arrayValue.resize(index + 1);
            }
            return arrayValue[index];
        }
        
        // Object access
        Value& operator[](const std::string& key) {
            if (type != OBJECT) {
                type = OBJECT;
                objectValue.clear();
            }
            return objectValue[key];
        }
        
        // Getters with defaults
        bool getBool(bool def = false) const {
            return type == BOOL ? boolValue : def;
        }
        
        double getNumber(double def = 0.0) const {
            return type == NUMBER ? numberValue : def;
        }
        
        int getInt(int def = 0) const {
            return type == NUMBER ? (int)numberValue : def;
        }
        
        int64_t getInt64(int64_t def = 0) const {
            return type == NUMBER ? (int64_t)numberValue : def;
        }
        
        std::string getString(const std::string& def = "") const {
            return type == STRING ? stringValue : def;
        }
        
        // Array size
        size_t size() const {
            return type == ARRAY ? arrayValue.size() : 0;
        }
        
        // Check if key exists
        bool has(const std::string& key) const {
            return type == OBJECT && objectValue.find(key) != objectValue.end();
        }
        
        // Serialize to JSON string
        std::string toString(int indent = 0) const {
            std::string indentStr(indent * 2, ' ');
            std::ostringstream oss;
            
            switch (type) {
                case NULL_TYPE:
                    oss << "null";
                    break;
                case BOOL:
                    oss << (boolValue ? "true" : "false");
                    break;
                case NUMBER:
                    if ((int64_t)numberValue == numberValue) {
                        oss << (int64_t)numberValue;
                    } else {
                        oss << numberValue;
                    }
                    break;
                case STRING:
                    oss << "\"" << escapeString(stringValue) << "\"";
                    break;
                case ARRAY:
                    oss << "[\n";
                    for (size_t i = 0; i < arrayValue.size(); i++) {
                        if (i > 0) oss << ",\n";
                        oss << indentStr << "  " << arrayValue[i].toString(indent + 1);
                    }
                    oss << "\n" << indentStr << "]";
                    break;
                case OBJECT:
                    oss << "{\n";
                    size_t i = 0;
                    for (const auto& pair : objectValue) {
                        if (i > 0) oss << ",\n";
                        oss << indentStr << "  \"" << escapeString(pair.first) << "\": " << pair.second.toString(indent + 1);
                        i++;
                    }
                    oss << "\n" << indentStr << "}";
                    break;
            }
            
            return oss.str();
        }
        
    private:
        static std::string escapeString(const std::string& str) {
            std::ostringstream oss;
            for (char c : str) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default: oss << c; break;
                }
            }
            return oss.str();
        }
    };
    
    // Simple JSON parser (very basic, handles our use case)
    class Parser {
    public:
        static Value parse(const std::string& json) {
            Parser p(json);
            return p.parseValue();
        }
        
        static Value parseFile(const std::string& filename) {
#ifdef _WIN32
            // На Windows нужно конвертировать путь в UTF-16 для поддержки Unicode
            int size_needed = MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, NULL, 0);
            if (size_needed <= 0) {
                std::cerr << "[JSON] Failed to convert filename to UTF-16: " << filename << std::endl;
                return Value();
            }
            std::vector<wchar_t> wfilename(size_needed);
            MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, wfilename.data(), size_needed);
            
            std::ifstream file;
            file.open(wfilename.data());
#else
            std::ifstream file(filename);
#endif
            if (!file.is_open()) {
                std::cerr << "[JSON] Failed to open file: " << filename << std::endl;
                return Value();
            }
            
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            file.close();
            
            return parse(content);
        }
        
    private:
        std::string json;
        size_t pos;
        
        Parser(const std::string& j) : json(j), pos(0) {}
        
        void skipWhitespace() {
            while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\n' || json[pos] == '\r' || json[pos] == '\t')) {
                pos++;
            }
        }
        
        Value parseValue() {
            skipWhitespace();
            if (pos >= json.size()) {
                Value v;
                v.type = Value::NULL_TYPE;
                return v;
            }
            
            char c = json[pos];
            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == '"') return Value(parseString());
            if (c == 't' && pos + 3 < json.size() && json.substr(pos, 4) == "true") { 
                pos += 4; 
                return Value(true); 
            }
            if (c == 'f' && pos + 4 < json.size() && json.substr(pos, 5) == "false") { 
                pos += 5; 
                return Value(false); 
            }
            if (c == 'n' && pos + 3 < json.size() && json.substr(pos, 4) == "null") { 
                pos += 4; 
                Value v;
                v.type = Value::NULL_TYPE;
                return v;
            }
            if (c == '-' || (c >= '0' && c <= '9')) return Value(parseNumber());
            
            Value v;
            v.type = Value::NULL_TYPE;
            return v;
        }
        
        Value parseObject() {
            Value obj;
            obj.type = Value::OBJECT;
            pos++; // skip '{'
            
            skipWhitespace();
            while (pos < json.size() && json[pos] != '}') {
                skipWhitespace();
                if (json[pos] != '"') break;
                std::string key = parseString();
                skipWhitespace();
                if (json[pos] != ':') break;
                pos++; // skip ':'
                Value value = parseValue();
                obj.objectValue[key] = value;
                skipWhitespace();
                if (json[pos] == ',') {
                    pos++;
                } else if (json[pos] != '}') {
                    break;
                }
            }
            
            if (pos < json.size() && json[pos] == '}') pos++;
            return obj;
        }
        
        Value parseArray() {
            Value arr;
            arr.type = Value::ARRAY;
            pos++; // skip '['
            
            skipWhitespace();
            while (pos < json.size() && json[pos] != ']') {
                Value value = parseValue();
                arr.arrayValue.push_back(value);
                skipWhitespace();
                if (json[pos] == ',') {
                    pos++;
                } else if (json[pos] != ']') {
                    break;
                }
            }
            
            if (pos < json.size() && json[pos] == ']') pos++;
            return arr;
        }
        
        std::string parseString() {
            pos++; // skip '"'
            std::ostringstream oss;
            while (pos < json.size() && json[pos] != '"') {
                if (json[pos] == '\\' && pos + 1 < json.size()) {
                    pos++;
                    switch (json[pos]) {
                        case '"': oss << '"'; break;
                        case '\\': oss << '\\'; break;
                        case 'n': oss << '\n'; break;
                        case 'r': oss << '\r'; break;
                        case 't': oss << '\t'; break;
                        default: oss << json[pos]; break;
                    }
                } else {
                    oss << json[pos];
                }
                pos++;
            }
            if (pos < json.size() && json[pos] == '"') pos++;
            return oss.str();
        }
        
        double parseNumber() {
            size_t start = pos;
            if (json[pos] == '-') pos++;
            while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') pos++;
            if (pos < json.size() && json[pos] == '.' && pos + 1 < json.size()) {
                pos++;
                while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') pos++;
            }
            if (pos < json.size() && (json[pos] == 'e' || json[pos] == 'E')) {
                pos++;
                if (json[pos] == '+' || json[pos] == '-') pos++;
                while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') pos++;
            }
            
            std::string numStr = json.substr(start, pos - start);
            return std::stod(numStr);
        }
    };
    
    // Write JSON to file
    inline bool writeFile(const std::string& filename, const Value& value) {
#ifdef _WIN32
        // На Windows нужно конвертировать путь в UTF-16 для поддержки Unicode
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, NULL, 0);
        if (size_needed <= 0) {
            std::cerr << "[JSON] Failed to convert filename to UTF-16: " << filename << std::endl;
            return false;
        }
        std::vector<wchar_t> wfilename(size_needed);
        MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, wfilename.data(), size_needed);
        
        std::ofstream file;
        file.open(wfilename.data(), std::ios::out | std::ios::trunc);
#else
        std::ofstream file(filename);
#endif
        if (!file.is_open()) {
            std::cerr << "[JSON] Failed to open file for writing: " << filename << std::endl;
            return false;
        }
        file << value.toString(0);
        file.close();
        return true;
    }
}

#endif /* UTILS_JSON_SIMPLE_H_ */

