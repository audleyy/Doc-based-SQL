#include "jsonValue.h"

JsonValue::JsonValue() {}

JsonValue::JsonValue(JsonType type) {
	if (type == JsonType::Object) {
		objectValue = new JsonObject;
	} else if (type == JsonType::Array) {
		arrayValue = new JsonArray;
	} else if (type != JsonType::Null) {
		throw invalid_argument("Invalid JsonType");
	}

	this->type = type;
}

JsonValue::JsonValue(int value) 
	: type(JsonType::Integer), intValue(value) {}

JsonValue::JsonValue(double value)
	: type(JsonType::Double),
		doubleValue(value) {}

JsonValue::JsonValue(const string& value) 
	: type(JsonType::String), 
		stringValue(value) {}

JsonValue::JsonValue(const char* value) 
	: type(JsonType::String), 
		stringValue(value) {}

JsonValue::JsonValue(bool value) 
	: type(JsonType::Boolean), 
		boolValue(value) {}

JsonType JsonValue::getType() const {
	return type;
}

JsonValue::JsonValue(const JsonValue& other) 
	: type(other.type),
		stringValue(other.stringValue),
		intValue(other.intValue),
		boolValue(other.boolValue) {
	if (other.arrayValue != nullptr) {
		arrayValue = new JsonArray(*other.arrayValue);
	}

	if (other.objectValue != nullptr) {
		objectValue = new JsonObject(*other.objectValue);
	}
}

JsonValue& JsonValue::operator=(const JsonValue& other) {
	if (this == &other) {
		return *this;
	}

	JsonArray* newArray = nullptr;
	JsonObject* newObject = nullptr;

	if (other.arrayValue != nullptr) {
		newArray = new JsonArray(*other.arrayValue);
	}

	if (other.objectValue != nullptr) {
		newObject = new JsonObject(*other.objectValue);
	}

	stringValue = other.stringValue;

	delete arrayValue;
	delete objectValue;

	type = other.type;
	intValue = other.intValue;
	boolValue = other.boolValue;

	arrayValue = newArray;
	objectValue = newObject;

	return *this;
}

int JsonValue::asInt() const {
	if (type != JsonType::Integer) {
		throw runtime_error("Value is not int");
	}
	return intValue;
}

double JsonValue::asDouble() const {
	if (type != JsonType::Double) {
		throw runtime_error("Value is not a double");
	}
	return doubleValue;
}

const string& JsonValue::asString() const {
	if (type != JsonType::String) {
		throw runtime_error("Value is not string");
	}
	return stringValue;
}

bool JsonValue::asBool() const {
	if (type != JsonType::Boolean) {
		throw runtime_error("Value is not bool");
	}
	return boolValue;
}

JsonArray& JsonValue::asArray(){
	if (type != JsonType::Array) {
		throw runtime_error("Value is not array");
	}
	return *arrayValue;
}

JsonObject& JsonValue::asObject() {
	if (type != JsonType::Object) {
		throw runtime_error("Value is not object");
	}
	return *objectValue;
}

const JsonArray& JsonValue::asArray() const {
	if (type != JsonType::Array) {
		throw runtime_error("Value is not array");
	}
	return *arrayValue;
}

const JsonObject& JsonValue::asObject() const {
	if (type != JsonType::Object) {
		throw runtime_error("Value is not object");
	}
	return *objectValue;
}

void JsonArray::add(const JsonValue& value) {
	elements.pushBack(value);
}

JsonValue& JsonArray::get(size_t index) {
	return elements.get(index);
}

const JsonValue& JsonArray::get(size_t index) const {
	return elements.get(index);
}

size_t JsonArray::length() const {
	return elements.length();
}


void JsonObject::add(const string& key, const JsonValue& value) {
	Field field;
	field.key = key;
	field.value = value;

	fields.pushBack(field);
}

Field& JsonObject::get(size_t index) {
	return fields.get(index);
}

const Field& JsonObject::get(size_t index) const {
	return fields.get(index);
}

JsonValue* JsonObject::find(const string& key) {
	for (size_t i = 0; i < fields.length(); ++i) {
		if (fields.get(i).key == key) return &fields.get(i).value;
	}
	return nullptr;
}

size_t JsonObject::length() const {
	return fields.length();
}

void JsonObject::remove(size_t index) {
	fields.remove(index);
}

JsonValue::~JsonValue() {
	delete arrayValue;
	delete objectValue;
}