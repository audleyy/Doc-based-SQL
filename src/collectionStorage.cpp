#include "collectionStorage.h"

CollectionStorage::CollectionStorage(SchemeManager& manager)
    : schemeManager(manager) {}

bool CollectionStorage::openCollection(const string& collectionName) {
	if (schemeManager.getCollection(collectionName) == nullptr) {
		return false;
	}
	
	collectionPath = "database/" + schemeManager.getName() + "/" + collectionName;

	return true;
}

string CollectionStorage::getFilePath(int fileNumber) {
	return collectionPath + "/" + to_string(fileNumber) + ".json";
}

bool CollectionStorage::fileExists(int fileNumber) {
	return filesystem::exists(getFilePath(fileNumber));
}

bool CollectionStorage::readFile(int fileNumber, JsonValue& result) {
	ifstream file(getFilePath(fileNumber));

	if (!file.is_open()) return false;

	string content;
	string line;

	while (getline(file, line)) {
		content += line + '\n';
	}

	if (file.bad()) return false;

	if (content.find_first_not_of(" \t\n\r") == string::npos) {
		content = "{}";
	}

	try {
		JsonParser parser(content);
		result = parser.parse();

		return result.getType() == JsonType::Object;
		
	} catch (const exception&) {
		return false;
	}
}

bool CollectionStorage::writeFile(int fileNumber, const JsonValue& data) {
	if (data.getType() != JsonType::Object) return false;

	string filename = getFilePath(fileNumber);
	string tempFilename = filename + ".tmp";

	ofstream temp(tempFilename, ios::trunc);

	if (!temp.is_open()) return false;

	JsonSerializer serializer;

	temp << serializer.serialize(data);

	temp.close();

	if (!temp) {
		filesystem::remove(tempFilename);
		return false;
	}

	error_code error;
	filesystem::rename(tempFilename, filename, error);

	if (error) {
		filesystem::remove(tempFilename);
		return false;
	}

	return true;
}

bool CollectionStorage::initialize() {
	try {
		string schemeName = "database/" + schemeManager.getName();

		filesystem::create_directories(schemeName);

		JsonObject* collections = schemeManager.getCollections();
		
		if (collections == nullptr) return false;

		for (size_t i = 0; i < collections->length(); i++) {
			string collectionName = collections->get(i).key;

			string path = schemeName + "/" + collectionName;

			filesystem::create_directories(path);

			string filename = path + "/1.json";

			if (!filesystem::exists(filename)) {
				ofstream file(filename);

				if (!file.is_open()) {
					return false;
				}

				file.close();
			}
		}

		return true;
	} catch (exception&) {
		return false;
	}
}

bool CollectionStorage::insertDocuments(const string& collectionName, JsonArray& documents) {
	if (!openCollection(collectionName)) return false;

	int limit = schemeManager.getTuplesLimit();

	if (limit <= 0 || documents.length() == 0) return false;

	int maxId = 0;
	int lastFile = 0;
	size_t recordsInLast = 0;

	for (int i = 1; fileExists(i); i++) {
		JsonValue data;

		if (!readFile(i, data)) return false;

		JsonObject& object = data.asObject();

		lastFile = i;
		recordsInLast = object.length();

		for (size_t j = 0; j < object.length(); j++) {
			int id = stoi(object.get(j).key);

			if (id > maxId) maxId = id;
		}
	}

	if (lastFile == 0) return false;

	int fileNumber = lastFile;
	size_t maxRecords = limit;

	if (recordsInLast >= maxRecords) {
		fileNumber++;
	}

	size_t index = 0;

	while (index < documents.length()) {
		JsonValue data;

		if (fileExists(fileNumber)) {
			if (!readFile(fileNumber, data)) return false;
		} else {
			string emptyObject = "{}";
			JsonParser parser(emptyObject);
			data = parser.parse();
		}

		JsonObject& object = data.asObject();

		while (index < documents.length() && object.length() < maxRecords) {
			maxId++;

			object.add(to_string(maxId), documents.get(index));

			index++;
		}

		if (!writeFile(fileNumber, data)) return false;

		fileNumber++;
	}

	return true;
}