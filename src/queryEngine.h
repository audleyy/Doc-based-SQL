#ifndef QUERYENGINE_H
#define QUERYENGINE_H

#include <string>
#include "collectionStorage.h"
#include "../json/jsonValue.h"

using namespace std;

class QueryEngine {
private:
	CollectionStorage& storage;
	SchemeManager& schemeManager;

	bool valuesEqual(const JsonValue& first, const JsonValue& second);
	bool compareValues(const JsonValue& first, const JsonValue& second, const string& operation);
	bool matchesFilter(JsonValue& document, JsonValue& filter);
	bool deleteDocuments(const string& collectionName, const string& filterText, bool onlyOne);
	bool applyUpdate(JsonValue& document, JsonValue& update);
	bool updateDocuments(const string& collectionName, const string& filterText, const string& updateText, bool onlyOne);

public:
	QueryEngine(CollectionStorage& storage, SchemeManager& schemeManager);

	bool find(const string& collectionName, const string& filterText, bool onlyOne);
	bool findOne(const string& collectionName, const string& filterText);
	bool insertOne(const string& collectionName, const string& documentText);
	bool insertMany(const string& collectionName, const string& documentsText);
	bool deleteOne(const string& collectionName, const string& filterText);
	bool deleteMany(const string& collectionName, const string& filterText);
	bool updateOne(const string& collectionName, const string& filterText, const string& updateText);
	bool updateMany(const string& collectionName, const string& filterText, const string& updateText);

};

#endif