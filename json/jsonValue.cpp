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

	try {
		if (other.arrayValue != nullptr) {
			newArray = new JsonArray(*other.arrayValue);
		}

		if (other.objectValue != nullptr) {
			newObject = new JsonObject(*other.objectValue);
		}

		stringValue = other.stringValue;
	}
	catch (...) {
		delete newArray;
		delete newObject;
		throw;
	}

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
		throw runtime_error("");
	}
	return intValue;
}

const string& JsonValue::asString() const {
	if (type != JsonType::String) {
		throw runtime_error("");
	}
	return stringValue;
}

bool JsonValue::asBool() const {
	if (type != JsonType::Boolean) {
		throw runtime_error("");
	}
	return boolValue;
}

JsonArray& JsonValue::asArray(){
	if (type != JsonType::Array) {
		throw runtime_error("");
	}
	return *arrayValue;
}

JsonObject& JsonValue::asObject() {
	if (type != JsonType::Object) {
		throw runtime_error("");
	}
	return *objectValue;
}

void JsonArray::add(const JsonValue& value) {
	elements.pushBack(value);
}

JsonValue& JsonArray::get(size_t index) {
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

JsonValue* JsonObject::find(const string& key) {
	for (size_t i = 0; i < fields.length(); ++i) {
		if (fields.get(i).key == key) return &fields.get(i).value;
	}
	return nullptr;
}

size_t JsonObject::length() const {
	return fields.length();
}

JsonValue::~JsonValue() {
	delete arrayValue;
	delete objectValue;
}
