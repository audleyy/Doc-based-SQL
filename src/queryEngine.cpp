#include "queryEngine.h"
#include "../json/jsonParser.h"
#include "../json/jsonSerializer.h"

QueryEngine::QueryEngine(CollectionStorage& storage, SchemeManager& schemeManager)
    : storage(storage), schemeManager(schemeManager) {}

bool QueryEngine::valuesEqual(const JsonValue& first, const JsonValue& second) {
	if (first.getType() != second.getType()) {
		return false;
	}

	switch (first.getType()) {
		case JsonType::String:
			return first.asString() == second.asString();

		case JsonType::Integer:
			return first.asInt() == second.asInt();

		case JsonType::Double:
			return first.asDouble() == second.asDouble();

		case JsonType::Boolean:
			return first.asBool() == second.asBool();

		case JsonType::Null:
			return true;

		default:
			return false;
	}
}

bool QueryEngine::compareValues(const JsonValue& first, const JsonValue& second, const string& operation) {
	if (operation == "$eq") return valuesEqual(first, second);
	if (operation == "$ne") return !valuesEqual(first, second);
	if (operation == "$in") {
		if (second.getType() != JsonType::Array) return false;
		const JsonArray& values = second.asArray();

		for (size_t i = 0; i < values.length(); ++i) {
			if (valuesEqual(first, values.get(i))) return true;
		}
	}

	if (first.getType() != second.getType()) return false;

	if (first.getType() == JsonType::Integer) {
		if (operation == "$gt") return first.asInt() > second.asInt();
		if (operation == "$lt") return first.asInt() < second.asInt();
		if (operation == "$gte") return first.asInt() >= second.asInt();
		if (operation == "$lte") return first.asInt() <= second.asInt();
	}

	if (first.getType() == JsonType::Double) {
		if (operation == "$gt") return first.asInt() > second.asInt();
		if (operation == "$lt") return first.asInt() < second.asInt();
		if (operation == "$gte") return first.asInt() >= second.asInt();
		if (operation == "$lte") return first.asInt() <= second.asInt();
	}

	return false;
}


bool QueryEngine::matchesFilter(JsonValue& document, JsonValue& filter) {
	if (document.getType() != JsonType::Object ||
		filter.getType() != JsonType::Object) {
		return false;
	}

	JsonObject& documentObject = document.asObject();
	JsonObject& filterObject = filter.asObject();

	for (size_t i = 0; i < filterObject.length(); ++i) {
		Field& operation = filterObject.get(i);

		if (operation.key == "$and") {
			if (operation.value.getType() != JsonType::Array) {
				return false;
			}

			JsonArray& operators = operation.value.asArray();

			if (operators.length() == 0) return false;

			for (size_t j = 0; j < operators.length(); ++j) {
				if (!matchesFilter(document, operators.get(j))) {
					return false;
				}
			}
		}

		else if (operation.key == "$or") {
			if (operation.value.getType() != JsonType::Array) {
				return false;
			}
			
			JsonArray& operators = operation.value.asArray();

			if (operators.length() == 0) return false;
			bool found = false;

			for (size_t j = 0; j < operators.length(); ++j) {
				if (matchesFilter(document, operators.get(j))) {
					found = true;
					break;
				}
			}

			if (!found) return false;
		}

		else if (operation.key == "$not") {
			if (operation.value.getType() != JsonType::Object) {
				return false;
			}

			if (matchesFilter(document, operation.value)) {
				return false;
			}
		}

		else {
			JsonValue* value = documentObject.find(operation.key);

			if (value == nullptr) {
				return false;
			}

			if (operation.value.getType() == JsonType::Object) {
				JsonObject& operators = operation.value.asObject();

				if (operators.length() == 0) {
					return false;
				}

				for (size_t j = 0; j < operators.length(); j++) {
					Field& operation = operators.get(j);

					if (!compareValues(*value, operation.value, operation.key)) {
						return false;
					}
				}
			} else {
				if (!valuesEqual(*value, operation.value)) {
					return false;
				}
			}
		}
	}

	return true;
}

bool QueryEngine::find(const string& collectionName, const string& filterText) {
	try {
		JsonParser parser(filterText);
		JsonValue filter = parser.parse();

		if (filter.getType() != JsonType::Object) return false;

		if (!storage.openCollection(collectionName)) return false;

		JsonSerializer serializer;

		string id;
		JsonValue document;

		while (true) {
			int status = storage.nextDocument(id, document);

			if (status == 0) {
				break;
			}

			if (status == -1) {
				return false;
			}

			if (matchesFilter(document, filter)) {
				cout << "ID: " << id << endl;
				cout << serializer.serialize(document) << endl;
			}
		}

		return true;
	}
	catch (const exception&) {
		return false;
	}
}

bool QueryEngine::insertOne(const string& collectionName, const string& documentText
) {
	try {
		JsonParser parser(documentText);
		JsonValue document = parser.parse();

		if (!schemeManager.validateDocument(collectionName, document)) {
			return false;
		}

		// return storage.insertDocument(collectionName, document);
	}
	catch (const exception&) {
		return false;
	}
}