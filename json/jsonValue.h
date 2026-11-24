#ifndef JSONVALUE_H
#define JSONVALUE_H

#include <iostream>
#include "../includes/array.h"
#include <string>

using namespace std;

class JsonArray;
class JsonObject;

enum class JsonType {
	Null,
	String,
	Integer,
	Double,
	Boolean,
	Array,
	Object
};

class JsonValue {
private:
	JsonType type = JsonType::Null;

	string stringValue;
	int intValue = 0;
	double doubleValue = 0.0;
	bool boolValue = false;

	JsonArray* arrayValue = nullptr;
	JsonObject* objectValue = nullptr;

public:
	JsonValue();
	JsonValue(JsonType type);
	JsonValue(const string& value);
	JsonValue(const char* value);
	JsonValue(int value);
	JsonValue(double value);
	JsonValue(bool value);
	JsonValue(const JsonValue& other);
	JsonValue& operator=(const JsonValue& other);

	JsonType getType() const;
	int asInt() const;
	double asDouble() const;
	const string& asString() const;
	bool asBool() const;
	JsonArray& asArray();
	JsonObject& asObject();
	const JsonArray& asArray() const;
	const JsonObject& asObject() const;

	~JsonValue();
};

struct Field {
	string key;
	JsonValue value;
};

class JsonArray {
private:
	Array<JsonValue> elements;

public:
	void add(const JsonValue& value);
	JsonValue& get(size_t index);
	const JsonValue& get(size_t index) const;
	size_t length() const;
};

class JsonObject {
private:
	Array<Field> fields;

public:
	void add(const string& key, const JsonValue& value);
	Field& get(size_t index);
	const Field& get(size_t index) const;
	JsonValue* find(const string& key);
	size_t length() const;
	void remove(size_t index);
};

#endif
