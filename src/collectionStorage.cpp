#include "collectionStorage.h"

CollectionStorage::CollectionStorage(SchemeManager& manager)
    : schemeManager(manager) {}

bool CollectionStorage::openCollection(const string& collectionName) {
	if (schemeManager.getCollection(collectionName) == nullptr) {
		return false;
	}

	file.close();

	collectionPath = "database/" + schemeManager.getName() + "/" + collectionName;
	fileNumber = 1;

	started = false;
	firstRecording = true;
	fileFinished = false;
	collectionWasReaded = true;

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

void CollectionStorage::skipSpaces(ifstream& file) {
	while (true) {
		int c = file.peek();

		if (c == ' ' || c == '\n' ||
			c == '\t' || c == '\r') {
			file.get();
		}
		else {
			break;
		}
	}
}

int CollectionStorage::nextDocument(string& id, JsonValue& document) {
	if (!collectionWasReaded) {
		return -1;
	}

	while (true) {
		if (fileFinished) {
			file.close();
			fileNumber++;
			started = false;
			firstRecording = true;
			fileFinished = false;
		}

		if (!file.is_open()) {
			string filename = collectionPath + "/" +
												to_string(fileNumber) + ".json";

			if (!filesystem::exists(filename)) {
				if (fileNumber == 1) return -1;
				return 0;
			}

			file.open(filename);

			if (!file.is_open()) return -1;
		}

		if (!started) {
			skipSpaces(file);

			if (file.peek() == EOF) {
				fileFinished = true;
				continue;
			}

			char c;
			file.get(c);

			if (c != '{') return -1;

			started = true;
		}

		skipSpaces(file);

		if (file.peek() == '}') {
			if (!firstRecording) return -1;

			file.get();
			skipSpaces(file);

			if (file.peek() != EOF) return -1;

			fileFinished = true;
			continue;
		}

		string entry;
		int depth = 0;
		bool inString = false;
		bool escaped = false;
		char separator = 0;
		char c;

		while (file.get(c)) {
			if (inString) {
				if (escaped) {
					escaped = false;
				} else if (c == '\\') {
					escaped = true;
				} else if (c == '"') {
					inString = false;
				}
			} else {
				if (c == '"') {
					inString = true;
				} else if (c == '{' || c == '[') {
					depth++;
				} else if (c == '}' || c == ']') {
					if (c == '}' && depth == 0) {
						separator = '}';
						break;
					}

					if (depth == 0) return -1;

					depth--;
				} else if (c == ',' && depth == 0) {
					separator = ',';
					break;
				}
			}

			entry += c;
		}

		if (separator == 0) {
			return -1;
		}

		try {
			JsonParser parser("{" + entry + "}");
			JsonValue result = parser.parse();

			JsonObject& object = result.asObject();

			if (object.length() != 1) return -1;

			Field& field = object.get(0);

			if (field.value.getType() != JsonType::Object) return -1;


			id = field.key;
			document = field.value;
		} catch (const exception&) {
			return -1;
		}

		firstRecording = false;

		if (separator == '}') {
			skipSpaces(file);

			if (file.peek() != EOF) {
				return -1;
			}

			fileFinished = true;
		}

		return 1;
	}
}

// bool CollectionStorage::insertDocument(const string& collectionName, const JsonValue& document) {

// }