#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <libinput.h>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

#include <fstream>
#include <iostream>
#include <json/forwards.h>
#include <json/json.h>
#include <json/reader.h>
#include <json/value.h>
#include <regex>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <vector>

using namespace std;
namespace fs = filesystem;

// Enhanced data structures for keymap configuration
struct Keymap {
  std::string name;                     // Optional name/identifier
  std::vector<std::string> keySequence; // Key sequence array
  std::string command;                  // Command to execute
  bool enabled = true;                  // Whether keymap is active
  std::string description;              // Optional description
};

struct GlobalConfig {
  std::string mainmod = "SUPER"; // Default main modifier
  bool enableAll = true;         // Global enable/disable
  std::map<std::string, Json::Value> additionalProperties;
};

struct Config {
  std::string schema;
  std::vector<Keymap> keymaps;
  GlobalConfig globals;
};

class JSONCParser {
private:
  bool initializedCorrectly = false;
  Config config;

  // Remove single-line comments (// ...)
  std::string removeSingleLineComments(const std::string &input) {
    std::regex singleLineComment(R"(//.*?$)", std::regex_constants::multiline);
    return std::regex_replace(input, singleLineComment, "");
  }

  // Remove multi-line comments (/* ... */)
  std::string removeMultiLineComments(const std::string &input) {
    std::regex multiLineComment(R"(/\*.*?\*/)",
                                std::regex_constants::ECMAScript);
    return std::regex_replace(input, multiLineComment, "");
  }

  // Convert JSONC to JSON by removing comments
  std::string preprocessJSONC(const std::string &jsonc) {
    std::string result = jsonc;
    result = removeMultiLineComments(result);
    result = removeSingleLineComments(result);
    return result;
  }

  // Parse global configuration
  GlobalConfig parseGlobalConfig(const Json::Value &globalsJson) {
    GlobalConfig globals;

    if (globalsJson.isMember("mainmod") && globalsJson["mainmod"].isString()) {
      globals.mainmod = globalsJson["mainmod"].asString();
    }

    if (globalsJson.isMember("enable_all") &&
        globalsJson["enable_all"].isBool()) {
      globals.enableAll = globalsJson["enable_all"].asBool();
    }

    // Store any additional global properties
    for (const auto &memberName : globalsJson.getMemberNames()) {
      if (memberName != "mainmod" && memberName != "enable_all") {
        globals.additionalProperties[memberName] = globalsJson[memberName];
      }
    }

    return globals;
  }

  // Parse a single keymap from array
  Keymap parseKeymap(const Json::Value &keymapJson) {
    Keymap keymap;

    if (!keymapJson.isObject()) {
      return keymap; // Invalid keymap
    }

    // Parse optional name
    if (keymapJson.isMember("name") && keymapJson["name"].isString()) {
      keymap.name = keymapJson["name"].asString();
    }

    // Parse key sequence (required)
    if (keymapJson.isMember("key-sequence") &&
        keymapJson["key-sequence"].isArray()) {
      const Json::Value &keySequence = keymapJson["key-sequence"];
      for (Json::Value::ArrayIndex i = 0; i < keySequence.size(); ++i) {
        if (keySequence[i].isString()) {
          keymap.keySequence.push_back(keySequence[i].asString());
        }
      }
    }

    // Parse command (required)
    if (keymapJson.isMember("command") && keymapJson["command"].isString()) {
      keymap.command = keymapJson["command"].asString();
    }

    // Parse optional enabled flag
    if (keymapJson.isMember("enabled") && keymapJson["enabled"].isBool()) {
      keymap.enabled = keymapJson["enabled"].asBool();
    }

    // Parse optional description
    if (keymapJson.isMember("description") &&
        keymapJson["description"].isString()) {
      keymap.description = keymapJson["description"].asString();
    }

    return keymap;
  }

  // Parse JSONC file and populate internal config
  bool parseConfigFile(const std::string &filename) {
    // Use default config file if none provided
    std::string configPath = filename;
    if (configPath.empty()) {
      const char *homeDir = std::getenv("HOME");
      if (homeDir) {
        configPath = fs::path(homeDir) / ".config/catalyst/config.jsonc";
      } else {
        std::cerr << "Error: Home environment variable not set and no config "
                     "file provided"
                  << std::endl;
        return false;
      }
    }

    // Read file content
    std::ifstream file(configPath);
    if (!file.is_open()) {
      std::cerr << "Error: Could not open file " << configPath << std::endl;
      return false;
    }

    std::string fileContent((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();

    // Preprocess JSONC to remove comments
    std::string jsonContent = preprocessJSONC(fileContent);

    // Parse JSON
    Json::Value root;
    Json::CharReaderBuilder readerBuilder;
    std::string parseErrors;
    std::istringstream jsonStream(jsonContent);

    if (!Json::parseFromStream(readerBuilder, jsonStream, &root,
                               &parseErrors)) {
      std::cerr << "Error parsing JSON: " << parseErrors << std::endl;
      return false;
    }

    // Parse schema
    if (root.isMember("$schema") && root["$schema"].isString()) {
      config.schema = root["$schema"].asString();
    }

    // Parse globals configuration
    if (root.isMember("globals") && root["globals"].isObject()) {
      config.globals = parseGlobalConfig(root["globals"]);
    }

    // Parse keymaps array
    if (root.isMember("keymaps") && root["keymaps"].isArray()) {
      const Json::Value &keymapsArray = root["keymaps"];
      for (Json::Value::ArrayIndex i = 0; i < keymapsArray.size(); ++i) {
        Keymap keymap = parseKeymap(keymapsArray[i]);
        if (!keymap.keySequence.empty() && !keymap.command.empty()) {
          config.keymaps.push_back(keymap);
        }
      }
    }

    return true;
  }

public:
  // Constructor that takes config file path
  JSONCParser(const std::string &filename = "") {
    initializedCorrectly = parseConfigFile(filename);
  }

  // Getter for keymaps (for compatibility with existing code)
  const std::vector<Keymap> &getKeymaps() const { return config.keymaps; }

  // Getter for the entire config
  const Config &getConfig() const { return config; }

  // Public access to initialization status
  bool isInitializedCorrectly() const { return initializedCorrectly; }

  // Helper function to print parsed configuration (for debugging)
  void printConfig() {
    std::cout << "Schema: " << config.schema << std::endl;
    std::cout << "Global mainmod: " << config.globals.mainmod << std::endl;
    std::cout << "Global enable_all: "
              << (config.globals.enableAll ? "true" : "false") << std::endl;

    std::cout << "Keymaps (" << config.keymaps.size() << "):" << std::endl;
    for (size_t i = 0; i < config.keymaps.size(); ++i) {
      const Keymap &keymap = config.keymaps[i];
      std::cout << "  [" << i << "] ";
      if (!keymap.name.empty()) {
        std::cout << "Name: " << keymap.name << ", ";
      }

      std::cout << "Keys: [";
      for (size_t j = 0; j < keymap.keySequence.size(); ++j) {
        if (j > 0)
          std::cout << ", ";
        std::cout << keymap.keySequence[j];
      }
      std::cout << "], Command: " << keymap.command;

      if (!keymap.enabled) {
        std::cout << " (DISABLED)";
      }

      if (!keymap.description.empty()) {
        std::cout << " - " << keymap.description;
      }

      std::cout << std::endl;
    }
  }
};

class FileEditor {
  fs::path configFile;
  string fileContents;
  string catalystStartKey = "# ------- Generated by Catalyst ------- \n\n";
  string catalystEndKey = "# ------ End of code by Catalyst ------ \n";

  bool initializedCorrectly;
  vector<Keymap> keymaps;

  vector<int> getPositionsOfCommas(string line) {
    vector<int> val;

    for (int i = 0; i < line.size(); i++) {
      if (line[i] == ',')
        val.push_back(i);
    }

    return val;
  }

  string formatKeyMap(Keymap km) {
    string line = "bind = , , exec, ";
    string modifiers[] = {"CONTROL", "MAINMOD", "SHIFT", "SUPER"};

    vector<int> commaPos = getPositionsOfCommas(line);
    line.append(km.command); // We always know the command will be last

    for (int i = 0; i < km.keySequence.size(); i++) {
      string key = km.keySequence[i];
      commaPos = getPositionsOfCommas(line);

      if (key == "MAINMOD")
        key = "$mainMod";

      bool isModifier =
          find(begin(modifiers), end(modifiers), key) != end(modifiers);
      if (isModifier) {
        line.insert(commaPos[0], " " + key);
      } else {
        line.insert(commaPos[1], " " + key);
      }
    }

    cout << "Written line: " << line << endl;
    return line;
  }

  bool writeToConfigFile() {
    fileContents.append(catalystStartKey);

    // Loop through all of the keymaps
    for (int i = 0; i < keymaps.size(); ++i) {
      // Only write enabled keymaps
      if (keymaps[i].enabled) {
        string line = formatKeyMap(keymaps[i]);
        fileContents.append(line + "\n");
      }
    }

    fileContents.append("\n");
    fileContents.append(catalystEndKey);

    cout << "New file contents: \n" << fileContents << endl;

    ofstream fileOfstream(configFile.string());
    fileOfstream << fileContents;

    return true;
  }

  bool readFile() {
    ifstream f(configFile.string());

    if (!f.is_open()) {
      cerr << "Error opening " << configFile.string() << endl;
      return false;
    }

    string s;
    bool withinGeneratedCode = false;

    // Reads the file but not the lines generated by catalyst
    while (getline(f, s)) {
      if (withinGeneratedCode) {
        if (s + "\n\n" == catalystEndKey) {
          withinGeneratedCode = false;
        }
        continue;
      }

      if (s + "\n\n" == catalystStartKey) {
        withinGeneratedCode = true;
        continue; // Add this line to skip the start key line itself
      }

      fileContents += s + "\n";
    }

    f.close();
    return true;
  }

public:
  FileEditor(vector<Keymap> km) {
    keymaps = km;

    // Set the config file
    const char *homeDir = std::getenv("HOME");
    if (homeDir) {
      configFile = fs::path(homeDir) / ".config/hypr/hyprland.conf";
    } else {
      std::cerr << "Error: Home environment variable not set";
      initializedCorrectly = false;
      return;
    }

    // Open the file
    readFile();
    writeToConfigFile();
  }

  FileEditor(vector<Keymap> km, string keybindsFile) {
    keymaps = km;
    configFile = fs::path(keybindsFile);

    // Open the file
    readFile();
    writeToConfigFile();
  }
};

int main(int argc, char *argv[]) {
  string configFile = "";
  string keybindsFile = "";

  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "--config") == 0) {
      if (i + 1 < argc) {
        configFile = argv[i + 1];
        ++i; // Skip next argument since we consumed it
      } else {
        std::cerr << "Error: --config requires an argument." << endl;
        return 1;
      }
    } else if (strcmp(argv[i], "--keybinds-file") == 0) {
      if (i + 1 < argc) {
        keybindsFile = argv[i + 1];
        ++i; // Skip next argument since we consumed it
      } else {
        std::cerr << "Error --keybinds-file requires argument." << endl;
        return 1;
      }
    } else if (strcmp(argv[i], "--help") == 0) {
      cout << "What is catalyst?" << endl;
      cout << "Catalyst is a package which allows you to use JSON to create "
              "keybinds in hyprland \n"
              "Your config file is by default stored in "
              "~/.config/catalyst/config.jsonc \n"
              "Args: "
           << endl
           << endl;
      cout << "catalyst --help:                 Shows this text" << endl;
      cout << "catalyst --config:               Sets your config file" << endl;
      cout << "catalyst --keybinds-file:        Sets the file with your "
              "keybinds"
           << endl;
      return 0;
    }
  }

  // Create JSONCParser instance
  JSONCParser *jsoncParser = new JSONCParser(configFile);

  if (!jsoncParser->isInitializedCorrectly()) {
    cout << "Config file not initialized correctly" << endl;
    delete jsoncParser;
    return 1;
  }

  // Get keymaps from the parser
  const vector<Keymap> &keymaps = jsoncParser->getKeymaps();

  // Debug: print commands
  for (size_t i = 0; i < keymaps.size(); i++) {
    cout << keymaps[i].command << endl;
  }

  // Create FileEditor with the keymaps
  FileEditor *fileEditor;
  if (keybindsFile.empty()) {
    fileEditor = new FileEditor(keymaps);
  } else {
    fileEditor = new FileEditor(keymaps, keybindsFile);
  }

  delete jsoncParser;
  delete fileEditor;

  return 0;
}
