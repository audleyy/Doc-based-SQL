#ifndef JSONPARSER_H
#define JSONPARSER_H

#include "jsonValue.h"
#include <string>

class JsonParser {
private:
	string text;
	size_t pos = 0;

	void skipSpaces();

	JsonValue parseValue();
	JsonValue parseObject();
	JsonValue parseArray();

	string parseString();
	JsonValue parseNumber();

public:
	JsonParser(const string& text);

	JsonValue parse();
};

#endif
