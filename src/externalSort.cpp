#include "externalSort.h"

bool ExternalSort::comesBefore(const JsonValue& first, const JsonValue& second, bool ascending) {
	if (first.getType() == JsonType::Integer) {
		if (ascending) {
			return first.asInt() < second.asInt();
		}

		return first.asInt() > second.asInt();
	}

	if (first.getType() == JsonType::String) {
		if (ascending) {
			return first.asString() < second.asString();
		}

		return first.asString() > second.asString();
	}

	return false;
}


void ExternalSort::sortRun(JsonObject& documents, const string& key, bool ascending) {
	for (size_t i = 1; i < documents.length(); i++) {
		Field current = documents.get(i);

		JsonValue* currentValue = current.value.asObject().find(key);

		size_t j = i;

		while (j > 0) {
			JsonValue* previousValue = documents.get(j - 1).value.asObject().find(key);

			if (!comesBefore(*currentValue, *previousValue, ascending)) break;

			documents.get(j) = documents.get(j - 1);

			j--;
		}

		documents.get(j) = current;
	}
}

bool ExternalSort::writeRecord(ofstream& file, const Field& record) {
	JsonSerializer serializer;

	file << "{"
				<< serializer.serialize(JsonValue(record.key))
				<< ":"
				<< serializer.serialize(record.value)
				<< "}\n";

	return !file.fail();
}

bool ExternalSort::writeRun(const string& filename, JsonObject& documents) {
	ofstream file(filename, ios::trunc);

	if (!file.is_open()) return false;

	for (size_t i = 0; i < documents.length(); i++) {
		if (!writeRecord(file, documents.get(i))) {
			return false;
		}
	}

	file.close();

	return !file.fail();
}


int ExternalSort::readRecord(ifstream& file, Field& record) {
	string line;

	if (!getline(file, line)) {
		if (file.eof() && !file.bad()) return 0;

		return -1;
	}

	try {
		JsonParser parser(line);
		JsonValue data = parser.parse();

		if (data.getType() != JsonType::Object) return -1;

		JsonObject& object = data.asObject();

		if (object.length() != 1) return -1;

		record = object.get(0);

		return 1;
	}
	catch (const exception&) {
		return -1;
	}
}

bool ExternalSort::mergeRuns(const string& leftFilename, const string& rightFilename, const string& resultFilename, const string& key, bool ascending) {
	ifstream leftFile(leftFilename);
	ifstream rightFile(rightFilename);
	ofstream resultFile(resultFilename);

	if (!leftFile.is_open() ||
		!rightFile.is_open() ||
		!resultFile.is_open()) {
		return false;
	}

	Field left;
	Field right;

	int leftStatus = readRecord(leftFile, left);
	int rightStatus = readRecord(rightFile, right);

	while (leftStatus == 1 && rightStatus == 1) {
		JsonValue* leftValue = left.value.asObject().find(key);

		JsonValue* rightValue = right.value.asObject().find(key);

		if (!comesBefore(*rightValue, *leftValue, ascending)) {
			if (!writeRecord(resultFile, left)) return false;

			leftStatus = readRecord(leftFile, left);
		} else {
			if (!writeRecord(resultFile, right)) return false;

			rightStatus = readRecord(rightFile, right);
		}
	}

	if (leftStatus == -1 || rightStatus == -1) return false;
	
	 while (leftStatus == 1) {
		if (!writeRecord(resultFile, left)) return false;

		leftStatus = readRecord(leftFile, left);
	}

	while (rightStatus == 1) {
		if (!writeRecord(resultFile, right)) return false;

		rightStatus = readRecord(rightFile, right);
	}

	resultFile.close();

	return leftStatus == 0 && rightStatus == 0 && !resultFile.fail();
}


bool ExternalSort::mergeAllRuns(const string& directory, size_t runCount, const string& key, bool ascending) {
	if (runCount == 0) return false;

	for (size_t step = 1; step < runCount; step *= 2) {
		for (size_t i = 1; i + step <= runCount; i += 2 * step) {
			string left = directory + "/run" + to_string(i) + ".jsonl";

			string right = directory + "/run" + to_string(i + step) + ".jsonl";

			string result = directory + "/merge.jsonl";

			if (!mergeRuns(left, right, result, key, ascending)) {
				return false;
			}

			error_code error;

			filesystem::rename(result, left, error);

			if (error) return false;

			filesystem::remove(right, error);

			if (error) return false;
		}
	}

	return true;
}
