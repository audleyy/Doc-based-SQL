#include "queryEngine.h"
#include "../json/jsonParser.h"
#include "../json/jsonSerializer.h"
#include <climits>

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
	if (operation == "$not") {
		if (second.getType() != JsonType::Object) return false;

		const JsonObject& conditions = second.asObject();

		if (conditions.length() == 0) return false;

		for (size_t i = 0; i < conditions.length(); i++) {
			const Field& condition = conditions.get(i);

			if (!compareValues(first, condition.value, condition.key)) {
				return true;
			}
		}

		return false;
	}
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

bool QueryEngine::matchesFilter(JsonValue& document, JsonValue& filter, const string* id) {
  if (document.getType() != JsonType::Object || filter.getType() != JsonType::Object) {
    return false;
  }

  JsonObject& documentObject = document.asObject();
  JsonObject& conditions = filter.asObject();

  for (size_t i = 0; i < conditions.length(); i++) {
    Field& condition = conditions.get(i);

    if (condition.key == "$and") {
      JsonArray& array = condition.value.asArray();

      for (size_t j = 0; j < array.length(); j++) {
        if (!matchesFilter(document, array.get(j), id)) {
          return false;
        }
      }
    }
    else if (condition.key == "$or") {
      JsonArray& array = condition.value.asArray();

      bool found = false;

      for (size_t j = 0; j < array.length(); j++) {
        if (matchesFilter(document, array.get(j), id)) {
          found = true;
          break;
        }
      }

      if (!found) return false;
    }
    else if (condition.key == "$not") {
      if (matchesFilter(document, condition.value, id)) {
        return false;
      }
    }
    else {
      JsonValue idValue;
      JsonValue* value;

      if (condition.key == "_id" && id != nullptr) {
        idValue = JsonValue(*id);
        value = &idValue;
      }
      else {
        value = documentObject.find(condition.key);
      }

      if (value == nullptr) return false;

      if (condition.value.getType() != JsonType::Object) {
        if (!valuesEqual(*value, condition.value)) return false;
      }
      else if (value->getType() == JsonType::Object) {
        if (!matchesFilter(*value, condition.value)) return false;
      }
      else {
        JsonObject& operators = condition.value.asObject();

        for (size_t j = 0; j < operators.length(); j++) {
          Field& operation = operators.get(j);

          if (!compareValues(*value, operation.value, operation.key)) {
            return false;
          }
        }
      }
    }
  }

  return true;
}

bool QueryEngine::find(const string& collectionName, const string& filterText, const string& projectionText, int limit, const string& sortKey, bool ascending) {
  try {
    JsonParser filterParser(filterText);
    JsonValue filter = filterParser.parse();

    JsonParser projectionParser(projectionText);
    JsonValue projectionValue = projectionParser.parse();

    if (!validateFilter(collectionName, filter)) return false;

    if (projectionValue.getType() != JsonType::Array) return false;

    JsonArray& projection = projectionValue.asArray();

    for (size_t i = 0; i < projection.length(); i++) {
      if (projection.get(i).getType() != JsonType::String) {
        return false;
      }
    }

    if (limit < -1) return false;

    if (!storage.openCollection(collectionName)) return false;

    if (!sortKey.empty()) {
      return findSorted(collectionName, filter, projection, limit, sortKey, ascending);
    }

    if (limit == 0) return true;

    int found = 0;

    for (int i = 1; storage.fileExists(i); i++) {
      JsonValue data;

      if (!storage.readFile(i, data)) return false;

      JsonObject& documents = data.asObject();

      for (size_t j = 0; j < documents.length(); j++) {
        Field& field = documents.get(j);

        if (!matchesFilter(field.value, filter, &field.key)) {
          continue;
        }

        printDocument(field, projection);

        found++;

        if (found == limit) return true;
      }
    }

    if (found == 0) {
      cout << "No documents found" << endl;
    }
    return true;
  }
  catch (const exception&) {
    return false;
  }
}

bool QueryEngine::sortAndPrint(const string& directory, JsonValue& filter, JsonArray& projection, int limit, const string& key, bool ascending) {
  ExternalSort sorter;
  size_t runCount = 0;

  for (int i = 1; storage.fileExists(i); i++) {
    JsonValue data;

    if (!storage.readFile(i, data)) return false;

    JsonObject& documents = data.asObject();

    size_t j = 0;

    while (j < documents.length()) {
      Field& field = documents.get(j);

      if (!matchesFilter(field.value, filter, &field.key)) {
        documents.remove(j);
      }
      else {
        j++;
      }
    }

    if (documents.length() == 0) continue;

    sorter.sortRun(documents, key, ascending);

    runCount++;

    string filename = directory + "/run" + to_string(runCount) + ".jsonl";

    if (!sorter.writeRun(filename, documents)) {
      return false;
    }
  }

  if (runCount == 0) return true;

  if (!sorter.mergeAllRuns(directory, runCount, key, ascending)) {
    return false;
  }

  ifstream file(directory + "/run1.jsonl");

  if (!file.is_open()) return false;

  Field record;
  int found = 0;

  int status = sorter.readRecord(file, record);

  while (status == 1) {
    printDocument(record, projection);

    if (limit > 0) {
      found++;

      if (found == limit) return true;
    }

    status = sorter.readRecord(file, record);
  }

  return status == 0;
}

bool QueryEngine::findSorted(const string& collectionName, JsonValue& filter, JsonArray& projection, int limit, const string& key, bool ascending) {
  JsonObject* structure =
    schemeManager.getCollection(collectionName);

  if (structure == nullptr) return false;

  JsonValue* field = structure->find(key);

  if (field == nullptr) return false;

  if (field->getType() != JsonType::String) return false;

  string type = field->asString();

  if (type != "int" && type != "str" && type != "time_stamp") {
    return false;
  }

  if (limit == 0) return true;

  string directory = "sort_tmp";
  error_code error;

  if (!filesystem::create_directory(directory, error)) {
    return false;
  }

  bool result = false;

  try {
    result = sortAndPrint(directory, filter, projection, limit, key, ascending);
  }
  catch (const exception&) {
    result = false;
  }

  filesystem::remove_all(directory, error);

  return result && !error;
}

bool QueryEngine::findOne(const string& collectionName, const string& filterText) {
  return find(collectionName, filterText, "[]", 1);
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
    return false;
  }
}

bool QueryEngine::deleteDocuments(const string& collectionName, const string& filterText, bool onlyOne) {
  try {
    JsonParser parser(filterText);
    JsonValue filter = parser.parse();

    if (!validateFilter(collectionName, filter)) return false;

    if (!storage.openCollection(collectionName)) return false;

    for (int i = 1; storage.fileExists(i); i++) {
      JsonValue data;

      if (!storage.readFile(i, data)) return false;

      JsonObject& documents = data.asObject();
      size_t originalLength = documents.length();
      size_t j = 0;

      while (j < documents.length()) {
        Field& field = documents.get(j);

        if (matchesFilter(field.value, filter, &field.key)) {
          documents.remove(j);

          if (onlyOne) return storage.writeFile(i, data);
        }
        else {
          j++;
        }
      }

      if (documents.length() != originalLength) {
        if (!storage.writeFile(i, data)) return false;
      }
    }

    return true;
  }
  catch (const exception&) {
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
  if (document.getType() != JsonType::Object) return false;

  JsonObject& object = document.asObject();
  JsonObject& operations = update.asObject();

  for (size_t i = 0; i < operations.length(); i++) {
    Field& operation = operations.get(i);

    JsonObject& fields = operation.value.asObject();

    for (size_t j = 0; j < fields.length(); j++) {
      Field& field = fields.get(j);

      JsonValue* value = object.find(field.key);

      if (value == nullptr) return false;

      if (operation.key == "$set") {
        *value = field.value;
      } else if (operation.key == "$inc") {
        if (value->getType() != JsonType::Integer) return false;

        long long sum = value->asInt();
        sum += field.value.asInt();

        if (sum > INT_MAX || sum < INT_MIN) return false;

        int number = sum;

        *value = JsonValue(number);
      } else if (operation.key == "$push") {
        if (value->getType() != JsonType::Array) {
          return false;
        }

        value->asArray().add(field.value);
      } else {
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

    if (!validateFilter(collectionName, filter)) return false;

    if (!validateUpdate(collectionName, update)) return false;

    if (!storage.openCollection(collectionName)) return false;

    for (int i = 1; storage.fileExists(i); i++) {
      JsonValue data;

      if (!storage.readFile(i, data)) return false;

      JsonObject& documents = data.asObject();

      int updated = 0;

      for (size_t j = 0; j < documents.length(); j++) {
        Field& field = documents.get(j);
        JsonValue& document = field.value;

        if (!matchesFilter(document, filter, &field.key)) continue;

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

JsonValue QueryEngine::applyProjection(JsonValue& document, JsonArray& projection) {
	string emptyObject = "{}";

	JsonParser parser(emptyObject);
	JsonValue result = parser.parse();

	JsonObject& source = document.asObject();
	JsonObject& target = result.asObject();

	for (size_t i = 0; i < projection.length(); ++i) {
		string key = projection.get(i).asString();

		JsonValue* value = source.find(key);

		if (value != nullptr && target.find(key) == nullptr) {
			target.add(key, *value);
		}
	}

	return result;
}

void QueryEngine::printDocument(Field& field, JsonArray& projection) {
	JsonSerializer serializer;

	cout << "ID: " << field.key << endl;

	if (projection.length() == 0) {
		cout << serializer.serialize(field.value) << endl;
	} else {
		JsonValue result = applyProjection(field.value, projection);

		cout << serializer.serialize(result) << endl;
	}
}

bool QueryEngine::matchesSchemeType(const JsonValue& value, const string& type) {
  if (type == "int") {
    return value.getType() == JsonType::Integer;
  }

  if (type == "str" || type == "time_stamp") {
    return value.getType() == JsonType::String;
  }

  if (type == "array") {
    return value.getType() == JsonType::Array;
  }

  return false;
}

bool QueryEngine::validateOperators(JsonValue& value, const string& type) {
  if (value.getType() != JsonType::Object) return false;

  JsonObject& operators = value.asObject();
  if (operators.length() == 0) return false;

  for (size_t i = 0; i < operators.length(); i++) {
    Field& operation = operators.get(i);

    if (operation.key == "$not") {
      if (!validateOperators(operation.value, type)) return false;
    } else if (operation.key == "$in") {
      if (operation.value.getType() != JsonType::Array) return false;

      JsonArray& array = operation.value.asArray();
      for (size_t j = 0; j < array.length(); j++) {
        if (!matchesSchemeType(array.get(j), type)) return false;
      }
    } else {
      if (operation.key == "$gt" || operation.key == "$lt" ||
          operation.key == "$gte" || operation.key == "$lte") {
        if (type != "int") return false;
      } else if (operation.key != "$eq" && operation.key != "$ne") {
        return false;
      }

      if (!matchesSchemeType(operation.value, type)) return false;
    }
  }

  return true;
}

bool QueryEngine::validateFilter(JsonObject& structure, JsonValue& filter, bool allowId) {
  if (filter.getType() != JsonType::Object) return false;

  JsonObject& conditions = filter.asObject();

  for (size_t i = 0; i < conditions.length(); i++) {
    Field& condition = conditions.get(i);

    if (condition.key == "$and" || condition.key == "$or") {
      if (condition.value.getType() != JsonType::Array) return false;

      JsonArray& array = condition.value.asArray();
      if (array.length() == 0) return false;

      for (size_t j = 0; j < array.length(); j++) {
        if (!validateFilter(structure, array.get(j), allowId)) return false;
      }
      continue;
    }

    if (condition.key == "$not") {
      if (!validateFilter(structure, condition.value, allowId)) return false;
      continue;
    }

    JsonValue idType("str");
    JsonValue* field = structure.find(condition.key);

    if (condition.key == "_id" && allowId) field = &idType;
    if (field == nullptr) return false;

    if (field->getType() == JsonType::Object) {
      if (condition.value.getType() != JsonType::Object) return false;
      if (condition.value.asObject().length() == 0) return false;
      if (!validateFilter(field->asObject(), condition.value, false)) return false;
      continue;
    }

    if (field->getType() != JsonType::String) return false;
    string type = field->asString();
    if (type == "array") return false;

    if (condition.value.getType() == JsonType::Object) {
      if (!validateOperators(condition.value, type)) return false;
    } else {
      if (!matchesSchemeType(condition.value, type)) return false;
    }
  }

  return true;
}

bool QueryEngine::validateFilter(const string& collectionName, JsonValue& filter) {
  JsonObject* structure = schemeManager.getCollection(collectionName);

  if (structure == nullptr) return false;

  return validateFilter(*structure, filter);
}

bool QueryEngine::validateUpdate(const string& collectionName, JsonValue& update) {
  if (update.getType() != JsonType::Object) return false;

  JsonObject* structure = schemeManager.getCollection(collectionName);
  if (structure == nullptr) return false;

  JsonObject& operations = update.asObject();
  if (operations.length() == 0) return false;

  for (size_t i = 0; i < operations.length(); i++) {
    Field& operation = operations.get(i);

    if (operation.key != "$set" && operation.key != "$inc" && operation.key != "$push") return false;
    if (operation.value.getType() != JsonType::Object) return false;

    JsonObject& fields = operation.value.asObject();
    if (fields.length() == 0) return false;

    for (size_t j = 0; j < fields.length(); j++) {
      Field& field = fields.get(j);
      if (field.key == "_id") return false;

      JsonValue* schemeField = structure->find(field.key);
      if (schemeField == nullptr) return false;

      if (operation.key == "$set" && schemeField->getType() == JsonType::Object) {
        if (!schemeManager.validateObject(schemeField->asObject(), field.value)) return false;
        continue;
      }

      if (schemeField->getType() != JsonType::String) return false;
      string type = schemeField->asString();

      if (operation.key == "$push") {
        if (type != "array") return false;
      } else {
        if (operation.key == "$inc" && type != "int") return false;
        if (!matchesSchemeType(field.value, type)) return false;
      }
    }
  }

  return true;
}
