#ifndef COLLECTIONSTORAGE_H
#define COLLECTIONSTORAGE_H

#include <string>
#include "schemeManager.h"
#include "../json/jsonParser.h"
#include "../json/jsonSerializer.h"
#include <filesystem>
#include <fstream>

using namespace std;

class CollectionStorage {
private:
	SchemeManager& schemeManager;
	string collectionPath;

	string getFilePath(int fileNumber);

public:
	CollectionStorage(SchemeManager& manager);

	bool initialize();
	bool openCollection(const string& collectionName);
	bool fileExists(int fileNumber);
	bool readFile(int fileNumber, JsonValue& result);
	bool writeFile(int fileNumber, const JsonValue& data);	
	bool insertDocuments(const string& collectionName, JsonArray& documents);
};

#endif