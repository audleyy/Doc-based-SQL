#include "jsonSerializer.h"
#include <sstream>

string JsonSerializer::serialize(const JsonValue& value) {
  switch(value.getType()) {
    case JsonType::Null:
      return "null";

    case JsonType::String:
      return serializeString(value.asString());

    case JsonType::Integer:
      return to_string(value.asInt());

    case JsonType::Double: {
      stringstream stream;
      stream.precision(17);
      stream << value.asDouble();

      string number = stream.str();

      if (number.find('.') == string::npos && number.find('e') == string::npos) number += ".0";

      return number;
    }

    case JsonType::Boolean:
      return value.asBool() ? "true" : "false";

    case JsonType::Array:
      return serializeArray(value.asArray());

    case JsonType::Object:
      return serializeObject(value.asObject());
  }

  throw runtime_error("Unknown type");
}

string JsonSerializer::serializeArray(const JsonArray& array) {
	string result = "[";

	for (size_t i = 0; i < array.length(); ++i) {
		if (i > 0) {
			result += ", ";
		}

		result += serialize(array.get(i));
	}

	result += "]";

	return result;
}

string JsonSerializer::serializeObject(const JsonObject& object) {
	string result = "{";

	for (size_t i = 0; i < object.length(); ++i) {
		if (i > 0) {
			result += ", ";
		}

		const Field& field = object.get(i);

		result += serializeString(field.key);
		result += ": ";
		result += serialize(field.value);
	}

	result += "}";

	return result;
}

string JsonSerializer::serializeString(const string& value) {
  string result = "\"";

  for (size_t i = 0; i < value.length(); ++i) {
    char c = value[i];

    switch (c) {
      case '"':
        result += "\\\"";
        break;

      case '\\':
        result += "\\\\";
        break;

      case '\n':
        result += "\\n";
        break;

      case '\t':
        result += "\\t";
        break;

      case '\r':
        result += "\\r";
        break;

      case '\b':
        result += "\\b";
        break;

      case '\f':
        result += "\\f";
        break;

      default:
        result += c;
        break;
    }
  }

  result += "\"";

  return result;
}