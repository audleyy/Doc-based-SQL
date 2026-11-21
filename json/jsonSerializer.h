#ifndef JSONSERIALIZER_H
#define JSONSERIALIZER_H

#include <string>
#include "jsonValue.h"
#include "jsonParser.h"


using namespace std;

class JsonSerializer {
private:
	string serializeString(const string& value);
	string serializeArray(const JsonArray& array);
	string serializeObject(const JsonObject& object);

public:
	string serialize(const JsonValue& value);
};

#endif
