#pragma once

#include <any>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>

class ParameterServer {
 public:
  ParameterServer() = default;

  // Throws an exception if the key already exists
  template <typename T>
  void AddParameter(const std::string& key,
                    const T& initial_value,
                    const std::string& description) {
    static_assert(std::is_arithmetic_v<T> || std::is_same_v<T, std::string>,
                  "Only basic types or std::string are allowed.");

    if (parameters_.count(key)) {
      throw std::runtime_error("Parameter key already exists: " + key);
    }

    parameters_[key] = Parameter{initial_value, description};
  }

  // Throws an exception if the key is not available
  template <typename T>
  T GetParameter(const std::string& key) {
    auto it = parameters_.find(key);
    if (it == parameters_.end()) {
      throw std::runtime_error("Parameter key not found: " + key);
    }

    try {
      return std::any_cast<T>(it->second.value);
    } catch (const std::bad_any_cast&) {
      throw std::runtime_error("Parameter type mismatch for key: " + key);
    }
  }

  // Throws an exception if the key is not available
  template <typename T>
  void SetParameter(const std::string& key, const T& value) {
    auto it = parameters_.find(key);
    if (it == parameters_.end()) {
      throw std::runtime_error("Parameter key not found: " + key);
    }

    if (it->second.value.type() != typeid(T)) {
      throw std::runtime_error("Parameter type mismatch for key: " + key);
    }

    it->second.value = value;
  }

  // Throws an exception if the key is not available
  std::string GetParameterInfo(const std::string& key) {
    auto it = parameters_.find(key);
    if (it == parameters_.end()) {
      throw std::runtime_error("Parameter key not found: " + key);
    }

    std::ostringstream oss;
    oss << "Key: " << key << ", Type: " << it->second.value.type().name()
        << ", Description: " << it->second.description;
    return oss.str();
  }

 private:
  struct Parameter {
    std::any value;
    std::string description;
  };

  std::map<std::string, Parameter> parameters_;
};