#include "jsonParser.h"

JsonParser::JsonParser(const string& text) {
	this->text = text;
}

void JsonParser::skipSpaces() {
	while (pos < text.length()) {
		char c = text[pos];

		if (c == ' ' || c == '\n' || c == '\t' || c == '\r') {
			pos++;
		} else {
			break;
		}
	}
}

JsonValue JsonParser::parseValue() {
	skipSpaces();

	if (pos >= text.length()) {
		throw runtime_error("Unexpected end of JSON");
	}

	char c = text[pos];

	switch (c) {
		case '{':
			return parseObject();

		case '[':
			return parseArray();

		case '"':
			return JsonValue(parseString());

		case 't':
			if (text.compare(pos, 4, "true") == 0) {
				pos += 4;
				return JsonValue(true);
			}
			break;

		case 'f':
			if (text.compare(pos, 5, "false") == 0) {
				pos += 5;
				return JsonValue(false);
			}
			break;

		case 'n':
			if (text.compare(pos, 4, "null") == 0) {
				pos += 4;
				return JsonValue();
			}
			break;

		default:
			if (c == '-' || (c >= '0' && c <= '9')) {
				return parseNumber();
			}
			break;
	}

	throw runtime_error("Invalid JSON value");
}

JsonValue JsonParser::parse() {
	pos = 0;

	JsonValue result = parseValue();

	skipSpaces();

	if (pos != text.length()) {
		throw runtime_error("Unexpected characters after JSON");
	}

	return result;
}

string JsonParser::parseString() {
	if (pos >= text.length() || text[pos] != '"') {
		throw runtime_error("Expected string");
	}

	pos++;

	string result;

	while (pos < text.length()) {
		char c = text[pos];
		pos++;

		if (c == '"') {
			return result;
		}

		if (c == '\\') {
			if (pos >= text.length()) {
				throw runtime_error("Invalid escape sequence");
			}

			char escaped = text[pos];
			pos++;

			switch (escaped) {
				case '"':
					result += '"';
					break;

				case '\\':
					result += '\\';
					break;

				case '/':
					result += '/';
					break;

				case 'n':
					result += '\n';
					break;

				case 't':
					result += '\t';
					break;

				case 'r':
					result += '\r';
					break;

				case 'b':
					result += '\b';
					break;

				case 'f':
					result += '\f';
					break;

				default:
					throw runtime_error("Unsupported escape sequence");
			}
		}
		else {
			if (c >= 0 && c < 32) {
				throw runtime_error("Unescaped control character");
			}

			result += c;
		}
	}

	throw runtime_error("Unterminated string");
}

JsonValue JsonParser::parseNumber() {
	size_t start = pos;
	bool isDouble = false;

	if (pos < text.length() && text[pos] == '-') {
		pos++;
	}

	if (pos >= text.length() || text[pos] < '0' || text[pos] > '9') {
		throw runtime_error("Invalid number");
	}

	if (text[pos] == '0') {
		pos++;

		if (pos < text.length() && text[pos] >= '0' && text[pos] <= '9') {
			throw runtime_error("Leading zeros are not allowed");
		}
	}
	else {
		while (pos < text.length() && text[pos] >= '0' && text[pos] <= '9') {
			pos++;
		}
	}

	if (pos < text.length() && text[pos] == '.') {
		isDouble = true;
		pos++;

		if (pos >= text.length() || text[pos] < '0' || text[pos] > '9') {
			throw runtime_error("Invalid fractional part");
		}

		while (pos < text.length() && text[pos] >= '0' && text[pos] <= '9') {
			pos++;
		}
	}

	if (pos < text.length() && (text[pos] == 'e' || text[pos] == 'E')) {

		isDouble = true;
		pos++;

		if (pos < text.length() && (text[pos] == '+' || text[pos] == '-')) {
			pos++;
		}

		if (pos >= text.length() || text[pos] < '0' || text[pos] > '9') {
			throw runtime_error("Invalid exponent");
		}

		while (pos < text.length() && text[pos] >= '0' && text[pos] <= '9') {
			pos++;
		}
	}

	string number = text.substr(start, pos - start);

	if (isDouble) {
		return JsonValue(stod(number));
	}
	return JsonValue(stoi(number));
}

JsonValue JsonParser::parseArray() {
	if (pos >= text.length() || text[pos] != '[') {
		throw runtime_error("Expected '['");
	}

	pos++;

	JsonValue result(JsonType::Array);

	skipSpaces();

	if (pos < text.length() && text[pos] == ']') {
		pos++;
		return result;
	}

	while (true) {
		JsonValue value = parseValue();
		result.asArray().add(value);

		skipSpaces();

		if (pos >= text.length()) {
			throw runtime_error("Unterminated array");
		}

		if (text[pos] == ']') {
			pos++;
			return result;
		}

		if (text[pos] != ',') {
			throw runtime_error("Expected ',' or ']'");
		}

		pos++;

		skipSpaces();

		if (pos >= text.length() || text[pos] == ']') {
			throw runtime_error("Expected value after comma");
		}
	}
}

JsonValue JsonParser::parseObject() {
	if (pos >= text.length() || text[pos] != '{') {
		throw runtime_error("Expected '{'");
	}

	pos++;

	JsonValue result(JsonType::Object);

	skipSpaces();

	if (pos < text.length() && text[pos] == '}') {
		pos++;
		return result;
	}

	while (true) {
		skipSpaces();

		if (pos >= text.length() || text[pos] != '"') {
			throw runtime_error("Expected object key");
		}

		string key = parseString();

		skipSpaces();

		if (pos >= text.length() || text[pos] != ':') {
			throw runtime_error("Expected ':'");
		}

		pos++;

		JsonValue value = parseValue();

		if (result.asObject().find(key) != nullptr) {
			throw runtime_error("Duplicate object key");
		}

		result.asObject().add(key, value);

		skipSpaces();

		if (pos >= text.length()) {
			throw runtime_error("Unterminated object");
		}

		if (text[pos] == '}') {
			pos++;
			return result;
		}

		if (text[pos] != ',') {
			throw runtime_error("Expected ',' or '}'");
		}

		pos++;

		skipSpaces();

		if (pos >= text.length() || text[pos] != '"') {
			throw runtime_error("Expected key after comma");
		}
	}
}