#include "json.h"

using namespace std;

namespace Json {

Document::Document(Node root) : root(move(root)) {
}

const Node& Document::GetRoot() const {
  return root;
}

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
  vector<Node> result;

  for (char c; input >> c && c != ']';) {
    if (c != ',') {
      input.putback(c);
    }
    result.push_back(LoadNode(input));
  }

  return Node(move(result));
}

//Node LoadInt(istream& input) {
//  size_t result = 0;
//  while (isdigit(input.peek())) {
//    result *= 10;
//    result += input.get() - '0';
//  }
//  return Node(result);
//}

Node LoadDouble(istream& input) {
  bool negative = false;
  size_t base = 0;
  size_t part = 0;
  uint32_t digits = 0;
  if (input.peek() == '-') {
    negative = true;
    input.get();
  }
  while (isdigit(input.peek())) {
    base *= 10;
    base += static_cast<size_t>(input.get()) - '0';
  }
  if (input.peek() == '.') {
    input.get();
    while (isdigit(input.peek())) {
      part *= 10;
      part += static_cast<size_t>(input.get()) - '0';
      ++digits;
    }
  }
  double result = digits ? base + static_cast<double>(part) / pow(10, digits) : base; 
  return negative ? -result : result;
}

Node LoadString(istream& input) {
  string line;
  getline(input, line, '"');
  return Node(move(line));
}

Node LoadDict(istream& input) {
  map<string, Node> result;

  for (char c; input >> c && c != '}';) {
    if (c == ',') {
      input >> c;
    }

    string key = LoadString(input).AsString();
    input >> c;
    result.emplace(move(key), LoadNode(input));
  }

  return Node(move(result));
}

Node LoadBool(istream& input) {
  char c;
  uint32_t count = 2;
  input >> c;
  if (c == 'a') {
    count = 3;
  }
  for (uint32_t i = 0; i < count; ++i) {
    input >> c;
  }
  return count == 2 ? Node(true) : Node(false);
}

Node LoadNode(istream& input) {
  char c;
  input >> c;

  if (c == '[') {
    return LoadArray(input);
  } else if (c == '{') {
    return LoadDict(input);
  } else if (c == '"') {
    return LoadString(input);
  } else if (c == 't' || c == 'f') {
    return LoadBool(input);
  } else {
    input.putback(c);
    return LoadDouble(input);
  }
}

Document Load(istream& input) {
  return Document{LoadNode(input)};
}


//-------------------------------------------------------------------
// Print Node

void ToJson(const Node& node, std::ostream& out, uint32_t lvl) {
  if (std::holds_alternative<std::vector<Node>>(node)) {
    ToJson(node.AsArray(), out, lvl);
  } else if (std::holds_alternative<std::string>(node)) {
    ToJson(node.AsString(), out, lvl);
  } else if (std::holds_alternative<std::map<std::string, Node>>(node)) {
    ToJson(node.AsMap(), out, lvl);
  } else if (std::holds_alternative<double>(node)) {
    ToJson(node.AsDouble(), out, lvl);
  } else if (std::holds_alternative<size_t>(node)) {
    ToJson(node.AsInt(), out, lvl);
  } else {
    ToJson(node.AsBool(), out, lvl);
  }
}

//-------------------------------------------------------------------


void ToJson(bool value, std::ostream& out, uint32_t lvl) {
  out << (value ? "true" : "false");
}

void ToJson(const std::string_view value, std::ostream& out, uint32_t lvl) {
  out << '"' << value << '"';
}

void ToJson(const std::string& value, std::ostream& out, uint32_t lvl) {
  out << '"' << value << '"';
}

void LeftWhiteSpace(uint32_t lvl, std::ostream& out) {
  for (uint32_t i = 0; i < lvl; ++i) {
    out << ' ';
  }
}

}  // namespace Json