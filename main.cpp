#include <iostream>
#include <string>

#include "src/schemeManager.h"
#include "src/collectionStorage.h"
#include "src/queryEngine.h"
#include "src/commandParser.h"

using namespace std;

int main() {
  SchemeManager schemeManager;

  if (!schemeManager.load("json/scheme.json")) {
    cout << "Scheme load error" << endl;
    return 1;
  }

  CollectionStorage storage(schemeManager);

  if (!storage.initialize()) {
    cout << "Storage initialization error" << endl;
    return 1;
  }

  QueryEngine queries(storage, schemeManager);
  CommandParser parser(queries);

  cout << "Document database" << endl;
  cout << "Enter exit to quit" << endl;

  string command;

  while (true) {
    cout << "db> ";

    if (!getline(cin, command)) break;

    if (command == "exit") break;

    if (command.empty()) continue;

    if (parser.execute(command)) {
      cout << "OK" << endl;
    }
    else {
      cout << "Query error" << endl;
    }
  }

  return 0;
}
