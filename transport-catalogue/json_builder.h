#pragma once

#include "json.h"
#include <stack>
#include <string>
#include <vector>
#include <optional>

namespace json {

class Builder {
public:
    class DictItemContext;
    class KeyItemContext;
    class ArrayItemContext;
    class ValueItemContext;
    
    Builder() = default;
    
    KeyItemContext Key(std::string key);
    Builder& Value(Node::Value value);
    
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    
    Builder& EndDict();
    Builder& EndArray();
    
    Node Build();
    
    // Контекст после StartDict() - доступны Key и EndDict
    class DictItemContext {
    public:
        DictItemContext(Builder& builder) : builder_(builder) {}
        
        KeyItemContext Key(std::string key) {
            return builder_.Key(std::move(key));
        }
        
        Builder& EndDict() {
            return builder_.EndDict();
        }
        
    private:
        Builder& builder_;
    };
    
    // Контекст после Key() - доступны Value, StartDict, StartArray
    class KeyItemContext {
    public:
        KeyItemContext(Builder& builder) : builder_(builder) {}
        
        DictItemContext Value(Node::Value value) {
            builder_.Value(std::move(value));
            return DictItemContext(builder_);
        }
        
        DictItemContext StartDict() {
            builder_.StartDict();
            return DictItemContext(builder_);
        }
        
        ArrayItemContext StartArray() {
            builder_.StartArray();
            return ArrayItemContext(builder_);
        }
        
    private:
        Builder& builder_;
    };
    
    // Контекст после Value() в массиве или StartArray() - доступны Value, StartDict, StartArray, EndArray
    class ArrayItemContext {
    public:
        ArrayItemContext(Builder& builder) : builder_(builder) {}
        
        ArrayItemContext Value(Node::Value value) {
            builder_.Value(std::move(value));
            return *this;
        }
        
        DictItemContext StartDict() {
            builder_.StartDict();
            return DictItemContext(builder_);
        }
        
        ArrayItemContext StartArray() {
            builder_.StartArray();
            return *this;
        }
        
        Builder& EndArray() {
            return builder_.EndArray();
        }
        
    private:
        Builder& builder_;
    };
    
private:
    struct NodeContext {
        bool is_dict = false;
        bool is_array = false;
        bool key_set = false;
        Dict dict;
        Array array;
    };
    
    void EnsureNotCompleted() const;
    void AddNode(Node node);
    
    std::stack<NodeContext> nodes_stack_;
    std::optional<Node> root_;
    bool completed_ = false;
    std::string current_key_;
};

} // namespace json