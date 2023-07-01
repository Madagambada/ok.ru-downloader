#pragma once
#include <fstream>
#include <string>
#include <vector>

class base {
public:
	bool load();
	bool save();
    std::vector<std::string> UserIDList;
    std::vector<std::string> vErrorList;
    std::vector<std::string> _vVideoID;
    std::vector<std::string> _todownload;
    std::vector<std::string> toRemove;
    std::vector<std::string> VidIDDownloading;
    std::string WorkingDir;
    std::string name = "OK.ru capture";
    std::string version = "1.0.0";
private:
   
};

bool base::load() {
    std::string line;
    std::ifstream file("ok-user.list");
    if (file.is_open()) {
        while (std::getline(file, line)) {
            UserIDList.push_back(line);
        }
        file.close();
        return 0;
    }
    return 1;
}

bool base::save() {
    std::ofstream file("ok-user.list");
    if (file.is_open()) {
        for (int i = 0; i < UserIDList.size(); i++) {
            file << UserIDList[i];
            if (i != UserIDList.size() - 1) {
                file << "\n";
            }
        }
        file.close();
        return 0;
    }
    return 1;
}

