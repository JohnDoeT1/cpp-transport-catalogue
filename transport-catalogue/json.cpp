#include "json.h"

#include <sstream>
#include <cctype>
#include <cmath>

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);
Node LoadNumber(istream& input);
Node LoadString(istream& input);
Node LoadArray(istream& input);
Node LoadDict(istream& input);
Node LoadBool(istream& input);
Node LoadNull(istream& input);

void SkipWhitespace(istream& input) {
    while (input && isspace(input.peek())) {
        input.get();
    }
}

bool IsValidAfterValue(char c) {
    return isspace(c) || c == ',' || c == ']' || c == '}' || c == EOF;
}

Node LoadArray(istream& input) {
    Array result;
    
    SkipWhitespace(input);
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing array");
    }
    
    char c;
    input >> c;
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing array");
    }
    
    SkipWhitespace(input);
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing array");
    }
    
    if (input.peek() == ']') {
        input.get();
        return Node(move(result));
    }
    
    while (true) {
        SkipWhitespace(input);
        if (!input) {
            throw ParsingError("Unexpected end of stream while parsing array");
        }
        
        result.push_back(LoadNode(input));
        
        SkipWhitespace(input);
        if (!input) {
            throw ParsingError("Unexpected end of stream while parsing array");
        }
        
        if (!(input >> c)) {
            throw ParsingError("Unexpected end of stream while parsing array");
        }
        
        if (c == ']') {
            break;
        } else if (c != ',') {
            throw ParsingError("Expected ',' or ']' in array");
        }
    }
    
    return Node(move(result));
}

Node LoadNumber(istream& input) {
    string number_str;
    
    while (input) {
        char c = input.peek();
        if (isdigit(c) || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E') {
            number_str += input.get();
        } else {
            break;
        }
    }
    
    if (number_str.empty()) {
        throw ParsingError("Empty number");
    }
    
    if (input) {
        char c = input.peek();
        if (isalpha(c) && c != 'e' && c != 'E') {
            throw ParsingError("Invalid character after number");
        }
    }
    
    try {
        size_t pos;
        int int_val = stoi(number_str, &pos);
        if (pos == number_str.length()) {
            return Node(int_val);
        }
    } catch (const out_of_range&) {
    } catch (...) {
    }
    
    try {
        double double_val = stod(number_str);
        return Node(double_val);
    } catch (const out_of_range&) {
        throw ParsingError("Number out of range");
    } catch (...) {
        throw ParsingError("Invalid number format");
    }
}

string LoadEscapedChar(istream& input) {
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing escape sequence");
    }
    
    char c = input.get();
    switch (c) {
        case 'n': return "\n";
        case 'r': return "\r";
        case 't': return "\t";
        case '"': return "\"";
        case '\\': return "\\";
        default: throw ParsingError("Unknown escape sequence");
    }
}

Node LoadString(istream& input) {
    string result;
    
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing string");
    }
    
    input.get();
    
    while (true) {
        if (!input) {
            throw ParsingError("Unterminated string");
        }
        
        char c = input.get();
        if (!input && c != '"') {
            throw ParsingError("Unterminated string");
        }
        
        if (c == '"') {
            break;
        } else if (c == '\\') {
            result += LoadEscapedChar(input);
        } else {
            result += c;
        }
    }
    
    return Node(move(result));
}

Node LoadDict(istream& input) {
    Dict result;
    
    SkipWhitespace(input);
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing dict");
    }
    
    char c;
    input >> c;
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing dict");
    }
    
    SkipWhitespace(input);
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing dict");
    }
    
    if (input.peek() == '}') {
        input.get();
        return Node(move(result));
    }
    
    while (true) {
        SkipWhitespace(input);
        if (!input) {
            throw ParsingError("Unexpected end of stream while parsing dict");
        }
        
        if (input.peek() != '"') {
            throw ParsingError("Expected '\"' at the beginning of key");
        }
        string key = LoadString(input).AsString();
        
        SkipWhitespace(input);
        if (!input) {
            throw ParsingError("Unexpected end of stream while parsing dict");
        }
        
        if (!(input >> c) || c != ':') {
            throw ParsingError("Expected ':' after key");
        }
        
        SkipWhitespace(input);
        if (!input) {
            throw ParsingError("Unexpected end of stream while parsing dict");
        }
        
        result.insert({move(key), LoadNode(input)});
        
        SkipWhitespace(input);
        if (!input) {
            throw ParsingError("Unexpected end of stream while parsing dict");
        }
        
        if (!(input >> c)) {
            throw ParsingError("Unexpected end of stream while parsing dict");
        }
        
        if (c == '}') {
            break;
        } else if (c != ',') {
            throw ParsingError("Expected ',' or '}' in dict");
        }
    }
    
    return Node(move(result));
}

Node LoadBool(istream& input) {
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing bool");
    }
    
    char c = input.peek();
    
    if (c == 't') {
        string word;
        for (int i = 0; i < 4; ++i) {
            if (!input) {
                throw ParsingError("Invalid boolean value");
            }
            word += input.get();
        }
        if (word != "true") {
            throw ParsingError("Invalid boolean value");
        }
        
        // Проверяем, что после true идёт допустимый разделитель
        if (input) {
            char next = input.peek();
            if (!IsValidAfterValue(next) && next != EOF) {
                throw ParsingError("Invalid characters after boolean value");
            }
        }
        
        return Node(true);
    } else if (c == 'f') {
        string word;
        for (int i = 0; i < 5; ++i) {
            if (!input) {
                throw ParsingError("Invalid boolean value");
            }
            word += input.get();
        }
        if (word != "false") {
            throw ParsingError("Invalid boolean value");
        }
        
        // Проверяем, что после false идёт допустимый разделитель
        if (input) {
            char next = input.peek();
            if (!IsValidAfterValue(next) && next != EOF) {
                throw ParsingError("Invalid characters after boolean value");
            }
        }
        
        return Node(false);
    }
    
    throw ParsingError("Invalid boolean value");
}

Node LoadNull(istream& input) {
    if (!input) {
        throw ParsingError("Unexpected end of stream while parsing null");
    }
    
    string word;
    for (int i = 0; i < 4; ++i) {
        if (!input) {
            throw ParsingError("Invalid null value");
        }
        word += input.get();
    }
    
    if (word != "null") {
        throw ParsingError("Invalid null value");
    }
    
    // Проверяем, что после null идёт допустимый разделитель
    if (input) {
        char next = input.peek();
        if (!IsValidAfterValue(next) && next != EOF) {
            throw ParsingError("Invalid characters after null value");
        }
    }
    
    return Node(nullptr);
}

Node LoadNode(istream& input) {
    SkipWhitespace(input);
    
    if (!input) {
        throw ParsingError("Unexpected end of stream");
    }
    
    char c = input.peek();
    
    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 't' || c == 'f') {
        return LoadBool(input);
    } else if (c == 'n') {
        return LoadNull(input);
    } else if (isdigit(c) || c == '-' || c == '+') {
        return LoadNumber(input);
    } else {
        throw ParsingError("Unexpected character");
    }
}

void PrintNode(const Node& node, ostream& output);

void PrintString(const string& str, ostream& output) {
    output << '"';
    for (char c : str) {
        switch (c) {
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            default: output << c; break;
        }
    }
    output << '"';
}

void PrintArray(const Array& arr, ostream& output) {
    output << '[';
    bool first = true;
    for (const auto& node : arr) {
        if (!first) {
            output << ',';
        }
        first = false;
        PrintNode(node, output);
    }
    output << ']';
}

void PrintDict(const Dict& dict, ostream& output) {
    output << '{';
    bool first = true;
    for (const auto& [key, node] : dict) {
        if (!first) {
            output << ',';
        }
        first = false;
        PrintString(key, output);
        output << ':';
        PrintNode(node, output);
    }
    output << '}';
}

void PrintNode(const Node& node, ostream& output) {
    if (node.IsNull()) {
        output << "null";
    } else if (node.IsBool()) {
        output << (node.AsBool() ? "true" : "false");
    } else if (node.IsInt()) {
        output << node.AsInt();
    } else if (node.IsPureDouble()) {
        output << node.AsDouble();
    } else if (node.IsString()) {
        PrintString(node.AsString(), output);
    } else if (node.IsArray()) {
        PrintArray(node.AsArray(), output);
    } else if (node.IsMap()) {
        PrintDict(node.AsMap(), output);
    }
}

}  // namespace

Node::Node(std::nullptr_t) : value_(nullptr) {}
Node::Node(Array array) : value_(move(array)) {}
Node::Node(Dict map) : value_(move(map)) {}
Node::Node(bool value) : value_(value) {}
Node::Node(int value) : value_(value) {}
Node::Node(double value) : value_(value) {}
Node::Node(string value) : value_(move(value)) {}

bool Node::IsInt() const {
    return holds_alternative<int>(value_);
}

bool Node::IsDouble() const {
    return holds_alternative<int>(value_) || holds_alternative<double>(value_);
}

bool Node::IsPureDouble() const {
    return holds_alternative<double>(value_);
}

bool Node::IsBool() const {
    return holds_alternative<bool>(value_);
}

bool Node::IsString() const {
    return holds_alternative<string>(value_);
}

bool Node::IsNull() const {
    return holds_alternative<nullptr_t>(value_);
}

bool Node::IsArray() const {
    return holds_alternative<Array>(value_);
}

bool Node::IsMap() const {
    return holds_alternative<Dict>(value_);
}

int Node::AsInt() const {
    if (!IsInt()) {
        throw logic_error("Node doesn't contain int");
    }
    return get<int>(value_);
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw logic_error("Node doesn't contain bool");
    }
    return get<bool>(value_);
}

double Node::AsDouble() const {
    if (IsPureDouble()) {
        return get<double>(value_);
    } else if (IsInt()) {
        return static_cast<double>(get<int>(value_));
    } else {
        throw logic_error("Node doesn't contain numeric value");
    }
}

const string& Node::AsString() const {
    if (!IsString()) {
        throw logic_error("Node doesn't contain string");
    }
    return get<string>(value_);
}

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw logic_error("Node doesn't contain array");
    }
    return get<Array>(value_);
}

const Dict& Node::AsMap() const {
    if (!IsMap()) {
        throw logic_error("Node doesn't contain map");
    }
    return get<Dict>(value_);
}

bool Node::operator==(const Node& rhs) const {
    return value_ == rhs.value_;
}

bool Node::operator!=(const Node& rhs) const {
    return !(*this == rhs);
}

Document::Document(Node root) : root_(move(root)) {}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& rhs) const {
    return root_ == rhs.root_;
}

bool Document::operator!=(const Document& rhs) const {
    return !(*this == rhs);
}

Document Load(istream& input) {
    try {
        Document doc{LoadNode(input)};
        
        // Проверяем, что после корневого элемента нет лишних символов кроме пробелов
        SkipWhitespace(input);
        if (input && input.peek() != EOF) {
            throw ParsingError("Unexpected characters after JSON document");
        }
        
        return doc;
    } catch (const ParsingError&) {
        throw;
    } catch (const exception& e) {
        throw ParsingError(e.what());
    }
}

void Print(const Document& doc, ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

}  // namespace json