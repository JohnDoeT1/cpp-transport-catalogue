#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

class Node;
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node : private std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string> {
public:
    using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

    // Наследуем конструкторы от variant
    using variant::variant;

    // Методы проверки типа
    bool IsInt() const { return std::holds_alternative<int>(*this); }
    bool IsPureDouble() const { return std::holds_alternative<double>(*this); }
    bool IsDouble() const { return IsInt() || IsPureDouble(); }
    bool IsBool() const { return std::holds_alternative<bool>(*this); }
    bool IsString() const { return std::holds_alternative<std::string>(*this); }
    bool IsNull() const { return std::holds_alternative<std::nullptr_t>(*this); }
    bool IsArray() const { return std::holds_alternative<Array>(*this); }
    bool IsMap() const { return std::holds_alternative<Dict>(*this); }

    // Методы получения значений
    int AsInt() const { return std::get<int>(*this); }
    bool AsBool() const { return std::get<bool>(*this); }
    double AsDouble() const {
        if (IsPureDouble()) return std::get<double>(*this);
        if (IsInt()) return static_cast<double>(std::get<int>(*this));
        throw std::logic_error("Not a number");
    }
    const std::string& AsString() const { return std::get<std::string>(*this); }
    const Array& AsArray() const { return std::get<Array>(*this); }
    const Dict& AsMap() const { return std::get<Dict>(*this); }

    // Возвращаем самого себя как const Value&
    const Value& GetValue() const { return *this; }

    bool operator==(const Node& rhs) const {
        return static_cast<const Value&>(*this) == static_cast<const Value&>(rhs);
    }
    bool operator!=(const Node& rhs) const { return !(*this == rhs); }
};

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& rhs) const;
    bool operator!=(const Document& rhs) const;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json