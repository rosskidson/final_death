#pragma once

#include <map>
#include <string>
#include <vector>
class SimpleProfiler {
 public:
  SimpleProfiler() = default;

  void LogEvent(const std::string& key){}

 private:
  std::map<std::string, std::vector<int>> time_measurements_;
};