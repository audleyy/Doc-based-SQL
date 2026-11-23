#include "schemeManager.h"

bool SchemeManager::load(const string& filename) {
	ifstream file(filename);

	if (!file.is_open()) return false;

	stringstream buffer;
	buffer << file.rdbuf();

	if (file.bad()) return false;

	string text = buffer.str();

	try {
		JsonParser parser(text);
		JsonValue result = parser.parse();

		if (result.getType() != JsonType::Object) return false;

		JsonObject& root = result.asObject();

		JsonValue* name = root.find("name");
		JsonValue* limit = root.find("tuples_limit");
		JsonValue* structure = root.find("structure");

		if (name == nullptr || limit == nullptr || structure == nullptr) return false;

		if (name->getType() != JsonType::String || limit->getType() != JsonType::Integer || structure->getType() != JsonType::Object) return false;

    if (name->asString().empty() || limit->asInt() <= 0) return false;

		JsonObject& collections = structure->asObject();

		for (size_t i = 0; i < collections.length(); i++) {
			if (collections.get(i).value.getType() != JsonType::Object) {
				return false;
			}
		}

		scheme = result;

		return true;

	} catch (exception&) {
		return false;
	}
}

string SchemeManager::getName() {
	JsonValue* value = scheme.asObject().find("name");

	return value->asString();
}

int SchemeManager::getTuplesLimit() {
	JsonValue* value = scheme.asObject().find("tuples_limit");

	return value->asInt();
}

JsonObject* SchemeManager::getCollections() {
	if (scheme.getType() != JsonType::Object) {
		return nullptr;
	}

	JsonValue* value = scheme.asObject().find("structure");

	if (value == nullptr) {
		return nullptr;
	}

	return &value->asObject();
}

JsonObject* SchemeManager::getCollection(const string& name) {
	JsonObject* collections = getCollections();

	if (collections == nullptr) {
		return nullptr;
	}

	JsonValue* collection = collections->find(name);

	if (collection == nullptr) {
		return nullptr;
	}

	return &collection->asObject();
}

bool SchemeManager::validateDocument(const string& collectionName, JsonValue& document) {
	JsonObject* structure = getCollection(collectionName);

	if (structure == nullptr) return false;

	if (document.getType() != JsonType::Object) return false;

	JsonObject& object = document.asObject();

	for (size_t i = 0; i < structure->length(); ++i) {
		Field& field = structure->get(i);

		JsonValue* value = object.find(field.key);

		if (value == nullptr) return false;
		
		string type = field.value.asString();

		if (type == "str") {
			if (value->getType() != JsonType::String) {
				return false;
			}
		} else if (type == "int") {
			if (value->getType() != JsonType::Integer) {
					return false;
			}
		} else if (type == "time_stamp") {
			if (value->getType() != JsonType::String) {
				return false;
			}
		} else {
			return false;
		}
	}

	for (size_t i = 0; i < object.length(); ++i) {
		Field& field = object.get(i);

		if (structure->find(field.key) == nullptr) {
			return false;
		}
	}

	return true;
}