#ifndef SCHEMEMANAGER_H
#define SCHEMEMANAGER_H

#include <string>
#include <fstream>
#include <sstream>
#include "../json/jsonValue.h"
#include "../json/jsonParser.h"

using namespace std;

class SchemeManager {
private:
	JsonValue scheme;

public:
	bool load(const string& filename);

	string getName();
	int getTuplesLimit();

	JsonObject* getCollections();
	JsonObject* getCollection(const string& name);

	bool validateDocument(const string& collectionName, JsonValue& document);
};

#endif