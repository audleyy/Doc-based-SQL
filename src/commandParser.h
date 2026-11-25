#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

#include <string>
#include "queryEngine.h"
#include "../includes/array.h"

using namespace std;

class CommandParser {
private:
  QueryEngine& queries;

  string trim(const string& text);
  bool splitArguments(const string& text, Array<string>& arguments);
  bool executeFind(const string& collectionName, Array<string>& arguments);
  bool executeUpdate(const string& collectionName, Array<string>& arguments);

public:
  CommandParser(QueryEngine& engine);

  bool execute(const string& command);
};

#endif
