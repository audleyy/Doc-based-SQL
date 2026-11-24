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
		if (operation == "$gt") return first.asDouble() > second.asDouble();
    if (operation == "$lt") return first.asDouble() < second.asDouble();
    if (operation == "$gte") return first.asDouble() >= second.asDouble();
    if (operation == "$lte") return first.asDouble() <= second.asDouble();
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

			if (operation.value.getType() != JsonType::Object) {
				if (!valuesEqual(*value, operation.value)) {
					return false;
				}

				continue;
			}

			JsonObject& nested = operation.value.asObject();

			if (nested.length() == 0) return false;

			bool hasOperators = false;
			bool hasFields = false;

			for (size_t j = 0; j < nested.length(); ++j) {
				const string& key = nested.get(j).key;

				if (!key.empty() && key[0] == '$') {
					hasOperators = true;
				} else {
					hasFields = true;
				}
			}

			if (hasOperators && hasFields) return false;

			if (hasOperators) {
				for (size_t j = 0; j < nested.length(); ++j) {
					Field& operation = nested.get(j);

					if (!compareValues(*value, operation.value, operation.key)) {
						return false;
					}
				}
			} else {
				if (!matchesFilter(*value, operation.value)) {
					return false;
				}
			}
		}
	}

	return true;
}

bool QueryEngine::find(const string& collectionName, const string& filterText, bool onlyOne) {
	try {
		JsonParser parser(filterText);
		JsonValue filter = parser.parse();

		if (filter.getType() != JsonType::Object) return false;

		if (!storage.openCollection(collectionName)) return false;

		JsonSerializer serializer;

		for (int i = 1; storage.fileExists(i); ++i) {
			JsonValue data;

			if (!storage.readFile(i, data)) return false;

			JsonObject& documents = data.asObject();

			for (size_t j = 0; j < documents.length(); ++j) {
				Field& field = documents.get(j);

				if (matchesFilter(field.value, filter)) {
					cout << "ID: " << field.key << endl;
					cout << serializer.serialize(field.value) << endl;

					if (onlyOne) return true;
				}
			}
		}

		return true;
	}
	catch (const exception&) {
		return false;
	}
}

bool QueryEngine::findOne(const string& collectionName, const string& filterText) {
	return find(collectionName, filterText, true);
}

bool QueryEngine::insertOne(const string& collectionName, const string& documentText) {
	try {
		JsonParser parser(documentText);
		JsonValue document = parser.parse();

		if (!schemeManager.validateDocument(collectionName, document)) return false;

		JsonArray documents;
		documents.add(document);

		return storage.insertDocuments(collectionName, documents);
	} catch (const exception&) {
		return false;
	}
}

bool QueryEngine::insertMany(const string& collectionName, const string& documentsText) {
	try {
		JsonParser parser(documentsText);
		JsonValue result = parser.parse();

		if (result.getType() != JsonType::Array) return false;

		JsonArray& documents = result.asArray();

		if (documents.length() == 0) return false;

		for (size_t i = 0; i < documents.length(); i++) {
			if (!schemeManager.validateDocument(
				collectionName, documents.get(i))) {
				return false;
			}
		}

		return storage.insertDocuments(collectionName, documents);
	} catch (const exception&) {
		throw false;
	}
}

bool QueryEngine::deleteDocuments(const string& collectionName, const string& filterText, bool onlyOne) {
	try {
		JsonParser parser(filterText);
		JsonValue filter = parser.parse();

		if (filter.getType() != JsonType::Object) return false;

		if (!storage.openCollection(collectionName)) return false;

		for (int i = 1; storage.fileExists(i); i++) {
			JsonValue data;

			if (!storage.readFile(i, data)) return false;

			JsonObject& documents = data.asObject();

			size_t j = 0;

			while (j < documents.length()) {
				if (matchesFilter(documents.get(j).value, filter)) {
					documents.remove(j);

					if (onlyOne) {
						return storage.writeFile(i, data);
					}
				} else {
					++j;
				}
			}
		}

		return true;

	} catch (const exception&) {
		return false;
	}
}

bool QueryEngine::deleteOne(const string& collectionName, const string& filterText) {
	return deleteDocuments(collectionName, filterText, true);
}

bool QueryEngine::deleteMany(const string& collectionName, const string& filterText) {
	return deleteDocuments(collectionName, filterText, false);
}

bool QueryEngine::applyUpdate(JsonValue& document, JsonValue& update) {
	if (document.getType() != JsonType::Object || update.getType() != JsonType::Object) return false;

	JsonObject& object = document.asObject();
	JsonObject& operations = update.asObject();

	for (size_t i = 0; i < operations.length(); i++) {
		Field& operation = operations.get(i);

		if (operation.value.getType() != JsonType::Object) return false;
		
		JsonObject& fields = operation.value.asObject();

		for (size_t j = 0; j < fields.length(); j++) {
			Field& field = fields.get(j);

			JsonValue* value = object.find(field.key);

			if (value == nullptr) return false;

			if (operation.key == "$set") {
				*value = field.value;
			} 

			else if (operation.key == "$inc") {
				if (value->getType() != JsonType::Integer ||
					field.value.getType() != JsonType::Integer) {
					return false;
				}

				int sum = value->asInt();
				sum += field.value.asInt();

				*value = JsonValue(sum);
			}
			
			else if (operation.key == "$push") {
				if (value->getType() != JsonType::Array) {
					return false;
				}

				value->asArray().add(field.value);
			} 
			
			else {
				return false;
			}
    }
	}

	return true;
}

bool QueryEngine::updateDocuments(const string& collectionName, const string& filterText, const string& updateText, bool onlyOne) {
	try {
		JsonParser filterParser(filterText);
		JsonValue filter = filterParser.parse();

		JsonParser updateParser(updateText);
		JsonValue update = updateParser.parse();

		if (filter.getType() != JsonType::Object ||
			update.getType() != JsonType::Object) {
			return false;
		}

		if (!storage.openCollection(collectionName)) return false;

		for (int i = 1; storage.fileExists(i); i++) {
			JsonValue data;

			if (!storage.readFile(i, data)) return false;

			JsonObject& documents = data.asObject();

			int updated = 0;

			for (size_t j = 0; j < documents.length(); j++) {
				JsonValue& document = documents.get(j).value;

				if (!matchesFilter(document, filter)) continue;

				if (!applyUpdate(document, update)) return false;

				if (!schemeManager.validateDocument(collectionName, document)) return false;

				updated++;

				if (onlyOne) return storage.writeFile(i, data);
			}

			if (updated > 0) {
				if (!storage.writeFile(i, data)) return false;
			}
		}

		return true;
	
	} catch (const exception&) {
		return false;
	}
}

bool QueryEngine::updateOne(const string& collectionName, const string& filterText, const string& updateText) {
	return updateDocuments(collectionName, filterText, updateText, true);
}

bool QueryEngine::updateMany(const string& collectionName, const string& filterText, const string& updateText) {
	return updateDocuments(collectionName, filterText, updateText, false);
}