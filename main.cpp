#include <filesystem>
#include <fstream>
#include <iostream>
#include <libinput.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include <json/forwards.h>
#include <json/reader.h>
#include <json/value.h>

#include "wayland.h"

using namespace std;
namespace fs = filesystem;

struct Keymap {
  std::vector<string> keySequence;
  std::string command;
  std::string name;
};

class JsonParser {
public:
  JsonParser(const std::string &configFilePath = "") {
    // Set the member variable 'configFile'
    if (configFilePath.empty()) {
      const char *homeDir = std::getenv("HOME");
      if (homeDir) {
        configFile = fs::path(homeDir) / ".config/catalyst/config.json";
      } else {
        std::cerr << "Error: HOME environment variable not set." << std::endl;
        initializedCorrectly = false;
        return;
      }
    } else {
      configFile = fs::path(configFilePath);
    }

    initializedCorrectly = initializeJson();
  }

  // This is the updated keymaps vector.
  std::vector<Keymap> keymaps;
  bool initializedCorrectly;

private:
  fs::path configFile;

  bool initializeJson() {
    if (!fs::exists(configFile)) {
      std::cerr << "Error: Config file not found at " << configFile.string()
                << std::endl;
      return false;
    }

    std::ifstream file(configFile, std::ifstream::binary);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open file " << configFile.string()
                << std::endl;
      return false;
    }

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errs;

    if (!Json::parseFromStream(builder, file, &root, &errs)) {
      std::cerr << "JSON parsing error: " << errs << std::endl;
      return false;
    }

    // Iterate through the keymaps and populate the 'keymaps' vector
    if (root.isObject()) {
      for (Json::Value::const_iterator itr = root.begin(); itr != root.end();
           ++itr) {
        if (itr.name() == "$schema")
          continue; // Skip the schema definition

        const Json::Value &keymapDetails = *itr;
        if (keymapDetails.isObject()) {
          Keymap newKeymap;
          newKeymap.name = itr.name();

          const Json::Value &keySequence = keymapDetails["key-sequence"];
          if (keySequence.isArray()) {
            for (Json::Value::ArrayIndex i = 0; i < keySequence.size(); ++i) {
              newKeymap.keySequence.push_back(keySequence[i].asString());
            }
          }

          const Json::Value &command = keymapDetails["command"];
          if (command.isString()) {
            newKeymap.command = command.asString();
          }

          keymaps.push_back(newKeymap);
        }
      }
    }

    // Check if any keymaps were loaded
    return !keymaps.empty();
  }
};

class InputProccessor {

public:
  bool connectedCorrectly;

  InputProccessor(vector<Keymap> km) {
    keymaps = km;
    printf("Press any key");
  }

private:
  vector<Keymap> keymaps;
};

int main() {
  vector<string> input;

  JsonParser *jsonParser = new JsonParser();

  if (!jsonParser->initializedCorrectly) {
    cout << "Config file not initialized correctly" << endl;
    return 1;
  }

  for (int i = 0; i < jsonParser->keymaps.size(); i++) {
    cout << jsonParser->keymaps[i].command << endl;
  }

  InputProccessor ip(jsonParser->keymaps);
  delete jsonParser;

  return 0;
}
