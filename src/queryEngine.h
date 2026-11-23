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

public:
	QueryEngine(CollectionStorage& storage, SchemeManager& schemeManager);

	bool find(const string& collectionName, const string& filterText);

	bool insertOne(const string& collectionName, const string& documentText);
};

#endif