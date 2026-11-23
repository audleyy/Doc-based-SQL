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

	ifstream file;
	string collectionPath;
	int fileNumber = 1;
	bool started = false;
	bool firstRecording = true;
	bool fileFinished = false;
	bool collectionWasReaded = false;

	void skipSpaces(ifstream& file);

public:
	CollectionStorage(SchemeManager& manager);

	bool openCollection(const string& collectionName);
	int nextDocument(string& id, JsonValue& document);
	
	bool initialize();

	bool insertDocument(const string& collectionName, const JsonValue& document);

};

#endif