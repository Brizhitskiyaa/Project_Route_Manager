#pragma once

#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <type_traits>
#include <cmath>
#include <iostream>
namespace Json {


class Node : private std::variant<std::vector<Node>, std::map<std::string, Node>,
                                   size_t, std::string, bool, double> {
  friend void ToJson(const Node& node, std::ostream& out, uint32_t lvl);

 public:
  using variant::variant;

  const auto& AsArray() const {
    return std::get<std::vector<Node>>(*this);
  }
  const auto& AsMap() const {
    return std::get<std::map<std::string, Node>>(*this);
  }
  size_t AsInt() const {
    return std::get<size_t>(*this);
  }
  const auto& AsString() const {
    return std::get<std::string>(*this);
  }

  bool AsBool() const {
    return std::get<bool>(*this);
  }

  double AsDouble() const {
    return std::get<double>(*this);
  }
};

class Document {
 public:
  explicit Document(Node root);

  const Node& GetRoot() const;

 private:
  Node root;
};

Document Load(std::istream& input);

void ToJson(const Node& node, std::ostream& out, uint32_t lvl = 0);

template <typename Num, typename = std::enable_if_t<std::is_arithmetic_v<Num>>>
void ToJson(const Num num, std::ostream& out, uint32_t lvl = 0);

void LeftWhiteSpace(uint32_t lvl, std::ostream& out);

void ToJson(bool value, std::ostream& out, uint32_t lvl = 0);

void ToJson(const std::string_view value, std::ostream& out, uint32_t lvl = 0);

void ToJson(const std::string& value, std::ostream& out, uint32_t lvl = 0);

template <typename K, typename V, typename... Types,
          template <typename, typename, typename...> class MapType,
          typename = decltype(std::string(
              std::declval<MapType<K, V, Types...>>().begin()->first))>
void ToJson(const MapType<K, V, Types...>& map, std::ostream& out, uint32_t lvl = 0 );

template <typename V>
void ToJson(const std::vector<V>& array, std::ostream& out, uint32_t lvl = 0);

template <
    typename K, typename V, typename... Types,
    template <typename, typename, typename...> class MapType,
    typename/* = decltype(std::string(std::declval<MapType<K,V,Types...>>().begin()->first))*/>
void ToJson(const MapType<K, V, Types...>& map, std::ostream& out, uint32_t lvl) {
  bool first = true;
  out << "{";
  for (const auto& [k, v] : map) {
    if (first) {
      out << '\n';
      first = false;
    } else {
      out << ",\n";
    }
    LeftWhiteSpace(lvl + 2, out);
    ToJson(k, out);
    out << ": ";
    ToJson(v, out, lvl + 2);
  }
  if (!first) {
    out << "\n";
    LeftWhiteSpace(lvl, out);
  }
  out << "}";
}

template <typename V>
void ToJson(const std::vector<V>& array, std::ostream& out, uint32_t lvl) {
  bool first = true;
  out << "[";
  for (const auto& value : array) {
    if (first) {
      first = false;
      out << "\n";
    } else {
      out << ",\n";
    }
    LeftWhiteSpace(lvl + 2, out);
    ToJson(value, out, lvl + 2);
  }
  if (!first) {
    out << "\n";
    LeftWhiteSpace(lvl, out);
  }
  out << "]";
}

template <typename Num, typename/* = std::enable_if_t<std::is_arithmetic_v<Num>>*/>
void ToJson(Num num, std::ostream& out, uint32_t lvl) {
  out << num;
}

}  // namespace Json