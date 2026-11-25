#include "commandParser.h"
#include "../json/jsonParser.h"

CommandParser::CommandParser(QueryEngine& engine)
  : queries(engine) {
}

string CommandParser::trim(const string& text) {
  size_t first = text.find_first_not_of(" \t\r\n");

  if (first == string::npos) return "";

  size_t last = text.find_last_not_of(" \t\r\n");

  return text.substr(first, last - first + 1);
}

bool CommandParser::splitArguments(const string& text, Array<string>& arguments) {
  if (trim(text).empty()) return true;

  int depth = 0;
  bool inString = false;
  size_t start = 0;

  for (size_t i = 0; i < text.length(); i++) {
    char c = text[i];

    if (inString && c == '\\') {
      i++;
      if (i == text.length()) return false;
      continue;
    }

    if (c == '"') {
      inString = !inString;
      continue;
    }

    if (inString) continue;

    if (c == '{' || c == '[') depth++;
    if (c == '}' || c == ']') depth--;
    if (depth < 0 || c == '(' || c == ')') return false;

    if (c == ',' && depth == 0) {
      string argument = trim(text.substr(start, i - start));
      if (argument.empty()) return false;
      arguments.pushBack(argument);
      start = i + 1;
    }
  }

  if (inString || depth != 0) return false;

  string argument = trim(text.substr(start));
  if (argument.empty()) return false;

  arguments.pushBack(argument);
  return true;
}

bool CommandParser::executeFind(const string& collectionName, Array<string>& arguments) {
  if (arguments.length() == 0 || arguments.length() > 4) return false;

  string projection;
  string sort;
  string limitText;

  for (size_t i = 1; i < arguments.length(); i++) {
    string argument = arguments.get(i);
    size_t pos = argument.find('=');
    if (pos == string::npos) return false;

    string key = trim(argument.substr(0, pos));
    string value = trim(argument.substr(pos + 1));
    if (value.empty()) return false;

    if (key == "projection" && projection.empty()) projection = value;
    else if (key == "sort" && sort.empty()) sort = value;
    else if (key == "limit" && limitText.empty()) limitText = value;
    else return false;
  }

  if (projection.empty()) projection = "[]";

  int limit = -1;
  string sortKey;
  bool ascending = true;

  try {
    if (!limitText.empty()) {
      JsonParser parser(limitText);
      JsonValue value = parser.parse();
      if (value.getType() != JsonType::Integer) return false;
      limit = value.asInt();
      if (limit < -1) return false;
    }

    if (!sort.empty()) {
      JsonParser parser(sort);
      JsonValue value = parser.parse();
      if (value.getType() != JsonType::Object) return false;

      JsonObject& object = value.asObject();
      if (object.length() != 1) return false;

      Field& field = object.get(0);
      if (field.key.empty() || field.value.getType() != JsonType::Integer) return false;

      int direction = field.value.asInt();
      if (direction != 1 && direction != -1) return false;

      sortKey = field.key;
      ascending = direction == 1;
    }

    return queries.find(collectionName, arguments.get(0), projection, limit, sortKey, ascending);
  } catch (const exception&) {
    return false;
  }
}

bool CommandParser::executeUpdate(const string& collectionName, Array<string>& arguments) {
  if (arguments.length() < 2 || arguments.length() > 3) return false;

  bool multi = false;

  if (arguments.length() == 3) {
    string argument = arguments.get(2);
    size_t pos = argument.find('=');
    if (pos == string::npos) return false;
    if (trim(argument.substr(0, pos)) != "multi") return false;

    string value = trim(argument.substr(pos + 1));
    if (value == "True" || value == "true") multi = true;
    else if (value != "False" && value != "false") return false;
  }

  if (multi) return queries.updateMany(collectionName, arguments.get(0), arguments.get(1));
  return queries.updateOne(collectionName, arguments.get(0), arguments.get(1));
}

bool CommandParser::execute(const string& command) {
  string text = trim(command);
  if (text.substr(0, 3) != "db.") return false;

  size_t open = text.find('(');
  if (open == string::npos || text[text.length() - 1] != ')') return false;

  size_t dot = text.find('.', 3);
  if (dot == string::npos || dot >= open) return false;

  string collectionName = text.substr(3, dot - 3);
  string operation = text.substr(dot + 1, open - dot - 1);
  if (collectionName.empty() || operation.empty()) return false;

  Array<string> arguments;
  if (!splitArguments(text.substr(open + 1, text.length() - open - 2), arguments)) return false;

  if (operation == "find") return executeFind(collectionName, arguments);
  if (operation == "update") return executeUpdate(collectionName, arguments);

  if (arguments.length() == 1) {
    string& argument = arguments.get(0);

    if (operation == "find_one") return queries.findOne(collectionName, argument);
    if (operation == "insert" || operation == "insert_one") return queries.insertOne(collectionName, argument);
    if (operation == "insert_many") return queries.insertMany(collectionName, argument);
    if (operation == "delete_one") return queries.deleteOne(collectionName, argument);
    if (operation == "delete_many") return queries.deleteMany(collectionName, argument);
  }

  if (arguments.length() == 2) {
    if (operation == "update_one") return queries.updateOne(collectionName, arguments.get(0), arguments.get(1));
    if (operation == "update_many") return queries.updateMany(collectionName, arguments.get(0), arguments.get(1));
  }

  return false;
}
