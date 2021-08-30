#include "Parcing.h"

std::pair<std::string_view, std::optional<std::string_view>> SplitTwoStrict(
    std::string_view str, const std::string_view delim) {
  const size_t pos = str.find(delim);
  if (pos != str.npos) {
    return {str.substr(0, pos), str.substr(pos + delim.length())};
  } else {
    return {str, std::nullopt};
  }
}

std::pair<std::string_view, std::string_view> SplitTwo(
    std::string_view str, std::string_view delim) {
  const auto [left, right] = SplitTwoStrict(str, delim);
  return {left, right.value_or("")};
}


std::string_view TrimRight(std::string_view str, std::string_view ch) {
  size_t pos = str.find_last_not_of(ch);
  return pos != str.npos ? str.substr(0, pos + 1) : "";
}

std::string_view TrimLeft(std::string_view str, std::string_view ch) {
  size_t pos = str.find_first_not_of(ch);
  return pos != str.npos ? str.substr(pos) : "";
}

std::string_view ReadToken(std::string_view& str,
                           std::string_view delim) {
  const auto [lhs, rhs] = SplitTwo(str, delim);
  str = rhs;
  return lhs;
}

template <>
double StoNum<double>(std::string_view str, size_t& pos) {
  return std::stod(std::string(str), &pos);
}

template <>
int StoNum<int>(std::string_view str, size_t& pos) {
  return std::stoi(std::string(str), &pos);
}

template <>
size_t StoNum<size_t>(std::string_view str, size_t& pos) {
  return std::stoull(std::string(str), &pos);
}