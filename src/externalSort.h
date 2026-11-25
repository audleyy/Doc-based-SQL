#ifndef EXTERNALSORT_H
#define EXTERNALSORT_H

#include <fstream>
#include <filesystem>
#include "../json/jsonValue.h"
#include "../json/jsonSerializer.h"

using namespace std;

class ExternalSort {
private:
	bool comesBefore(const JsonValue& first, const JsonValue& second, bool ascending);

public:
	void sortRun(JsonObject& documents, const string& key, bool ascending);
	bool writeRecord(ofstream& file, const Field& record);
	bool writeRun(const string& filename, JsonObject& documents);
	int readRecord(ifstream& file, Field& record);
	bool mergeRuns(const string& leftFilename, const string& rightFilename, const string& resultFilename, const string& key, bool ascending);
	bool mergeAllRuns(const string& directory, size_t runCount, const string& key, bool ascending);
};

#endif
