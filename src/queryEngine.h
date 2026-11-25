#ifndef QUERYENGINE_H
#define QUERYENGINE_H

#include <string>
#include "collectionStorage.h"
#include "../json/jsonValue.h"
#include "externalSort.h"
#include <filesystem>
#include <fstream>

using namespace std;

class QueryEngine {
private:
  CollectionStorage& storage;
  SchemeManager& schemeManager;

  bool valuesEqual(const JsonValue& first, const JsonValue& second);
  bool compareValues(const JsonValue& first, const JsonValue& second, const string& operation);
  bool matchesFilter(JsonValue& document, JsonValue& filter, const string* id = nullptr);
  bool deleteDocuments(const string& collectionName, const string& filterText, bool onlyOne);
  bool applyUpdate(JsonValue& document, JsonValue& update);
  bool updateDocuments(const string& collectionName, const string& filterText, const string& updateText, bool onlyOne);
  void printDocument(Field& field, JsonArray& projection);
  bool sortAndPrint(const string& directory, JsonValue& filter, JsonArray& projection, int limit, const string& key, bool ascending);
  bool findSorted(const string& collectionName, JsonValue& filter, JsonArray& projection, int limit, const string& key, bool ascending);
  bool matchesSchemeType(const JsonValue& value, const string& type);
  bool validateOperators(JsonValue& value, const string& type);
  bool validateFilter(JsonObject& structure, JsonValue& filter, bool allowId = true);
  bool validateFilter(const string& collectionName, JsonValue& filter);
  bool validateUpdate(const string& collectionName, JsonValue& update);

public:
  QueryEngine(CollectionStorage& storage, SchemeManager& schemeManager);

  bool find(const string& collectionName, const string& filterText, const string& projectionText = "[]", int limit = -1, const string& sortKey = "", bool ascending = true);
  bool findOne(const string& collectionName, const string& filterText);
  bool insertOne(const string& collectionName, const string& documentText);
  bool insertMany(const string& collectionName, const string& documentsText);
  bool deleteOne(const string& collectionName, const string& filterText);
  bool deleteMany(const string& collectionName, const string& filterText);
  bool updateOne(const string& collectionName, const string& filterText, const string& updateText);
  bool updateMany(const string& collectionName, const string& filterText, const string& updateText);
  JsonValue applyProjection(JsonValue& document, JsonArray& projection);

};

#endif