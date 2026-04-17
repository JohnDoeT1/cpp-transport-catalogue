#include "json_builder.h"
#include <stdexcept>

using namespace std::literals;

namespace json {

Builder::KeyItemContext Builder::Key(std::string key) {
    EnsureNotCompleted();
    
    if (nodes_stack_.empty() || !nodes_stack_.top().is_dict) {
        throw std::logic_error("Key can only be called inside a dictionary"s);
    }
    
    if (nodes_stack_.top().key_set) {
        throw std::logic_error("Key already set"s);
    }
    
    current_key_ = std::move(key);
    nodes_stack_.top().key_set = true;
    
    return KeyItemContext(*this);
}

Builder& Builder::Value(Node::Value value) {
    EnsureNotCompleted();
    
    Node node;
    std::visit([&node](auto&& arg) {
        node = Node(std::forward<decltype(arg)>(arg));
    }, value);
    
    if (nodes_stack_.empty()) {
        if (root_.has_value()) {
            throw std::logic_error("Builder already has a root"s);
        }
        root_ = std::move(node);
        completed_ = true;
    } else {
        auto& top = nodes_stack_.top();
        if (top.is_dict) {
            if (!top.key_set) {
                throw std::logic_error("Key not set"s);
            }
            top.dict[current_key_] = std::move(node);
            top.key_set = false;
            current_key_.clear();
        } else if (top.is_array) {
            top.array.emplace_back(std::move(node));
        } else {
            throw std::logic_error("Invalid state"s);
        }
    }
    
    return *this;
}

Builder::DictItemContext Builder::StartDict() {
    EnsureNotCompleted();
    
    NodeContext new_context;
    new_context.is_dict = true;
    new_context.dict = Dict{};
    new_context.key_set = false;
    
    if (nodes_stack_.empty()) {
        if (root_.has_value()) {
            throw std::logic_error("Builder already has a root"s);
        }
    } else {
        auto& top = nodes_stack_.top();
        if (top.is_dict && !top.key_set) {
            throw std::logic_error("Key not set"s);
        }
    }
    
    nodes_stack_.push(std::move(new_context));
    
    return DictItemContext(*this);
}

Builder& Builder::EndDict() {
    EnsureNotCompleted();
    
    if (nodes_stack_.empty() || !nodes_stack_.top().is_dict) {
        throw std::logic_error("EndDict called outside of dictionary"s);
    }
    
    if (nodes_stack_.top().key_set) {
        throw std::logic_error("Key without value"s);
    }
    
    Node node(std::move(nodes_stack_.top().dict));
    nodes_stack_.pop();
    
    AddNode(std::move(node));
    
    return *this;
}

Builder::ArrayItemContext Builder::StartArray() {
    EnsureNotCompleted();
    
    NodeContext new_context;
    new_context.is_array = true;
    new_context.array = Array{};
    new_context.key_set = false;
    
    if (nodes_stack_.empty()) {
        if (root_.has_value()) {
            throw std::logic_error("Builder already has a root"s);
        }
    } else {
        auto& top = nodes_stack_.top();
        if (top.is_dict && !top.key_set) {
            throw std::logic_error("Key not set"s);
        }
    }
    
    nodes_stack_.push(std::move(new_context));
    
    return ArrayItemContext(*this);
}

Builder& Builder::EndArray() {
    EnsureNotCompleted();
    
    if (nodes_stack_.empty() || !nodes_stack_.top().is_array) {
        throw std::logic_error("EndArray called outside of array"s);
    }
    
    Node node(std::move(nodes_stack_.top().array));
    nodes_stack_.pop();
    
    AddNode(std::move(node));
    
    return *this;
}

Node Builder::Build() {
    if (!completed_) {
        throw std::logic_error("Builder is not ready"s);
    }
    
    if (!root_.has_value()) {
        throw std::logic_error("Empty builder"s);
    }
    
    return *root_;
}

void Builder::EnsureNotCompleted() const {
    if (completed_) {
        throw std::logic_error("Cannot modify completed builder"s);
    }
}

void Builder::AddNode(Node node) {
    if (nodes_stack_.empty()) {
        root_ = std::move(node);
        completed_ = true;
    } else {
        auto& top = nodes_stack_.top();
        if (top.is_dict) {
            if (!top.key_set) {
                throw std::logic_error("Key not set"s);
            }
            top.dict[current_key_] = std::move(node);
            top.key_set = false;
            current_key_.clear();
        } else if (top.is_array) {
            top.array.emplace_back(std::move(node));
        }
    }
}

} // namespace json