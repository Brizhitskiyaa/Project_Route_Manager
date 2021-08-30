#pragma once

#include <tuple>
#include <string_view>
#include <optional>
#include <charconv>
#include <stdexcept>
#include <sstream>

std::pair<std::string_view, std::optional<std::string_view>> SplitTwoStrict(
    std::string_view str, const std::string_view delim = " ");

std::pair<std::string_view, std::string_view> SplitTwo(
    std::string_view str, std::string_view delim = " ");

std::string_view ReadToken(std::string_view& str, std::string_view delim = " ");

std::string_view TrimRight(std::string_view str, std::string_view ch = " ");

std::string_view TrimLeft(std::string_view str, std::string_view ch = " ");

template <typename NumType>
NumType StoNum(std::string_view str, size_t& pos);

template <typename NumType>
NumType ConvertToNum(std::string_view str) {
  size_t pos = 0;
  NumType result = StoNum<NumType>(str, pos);
  // const auto [ptr, ec] =
  //    std::from_chars(std::data(str), std::data(str) + std::size(str),
  //    result);
  // if (ec == std::errc()) {
  //  return result;
  //}
  if (pos == str.length()) {
    return result;
  }
  
  std::stringstream message;
  message << "string " << str << " contains " << (str.length() - pos)
                    << " trailing chars";
  throw(std::invalid_argument(message.str()));
}

template <typename Number>
Number ReadNumberOnLine(std::istream& stream) {
  Number number;
  stream >> number;
  std::string dummy;
  std::getline(stream, dummy);
  return number;
}