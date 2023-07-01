#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <filesystem>

#include "base.h"
#include "thread_pool.h"
#include <curl/curl.h>
#include "strutil.h"
#include "httplib.h"
#include "cxxopts.hpp"

httplib::Server svr;

static size_t curl_write_func(void* buffer, size_t size, size_t nmemb, void* param) {
    std::string& text = *static_cast<std::string*>(param);
    size_t totalsize = size * nmemb;
    text.append(static_cast<char*>(buffer), totalsize);
    return totalsize;
}

std::string checkerTask(std::string uid, std::vector<std::string> *vErrorList) {
    //std::cout << uid;
    CURL* curl;
    CURLcode res;
    std::string curlData;
    std::string url = "https://ok.ru/profile/" + uid + "/video";
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        //curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/ssl/certs/ca-certificates.crt");
        curl_easy_setopt(curl, CURLOPT_DNS_SERVERS, "1.1.1.1,1.0.0.1");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/113.0.0.0 Safari/537.36");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_func);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlData);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::stringstream sstime;
            std::time_t etimr = std::time(nullptr);
            sstime << std::put_time(std::localtime(&etimr), "[%H:%M:%S, %d-%m-%Y] checkerTask error ") << res << ": " << curl_easy_strerror(res) << ", at: " << url;
            vErrorList->push_back(sstime.str());
            curl_easy_cleanup(curl);
            return "";
        }
        if (strutil::contains(curlData, "video-card_live __active")) {
            curlData = strutil::split(curlData, "video-card_live __active")[0];
            curlData = strutil::split(curlData, "video-card_img-w")[1];
            curlData = strutil::split(curlData, "href=\"")[1];
            curlData = strutil::split(curlData, "\"")[0];
            curl_easy_cleanup(curl);
            return "live-" + uid + "," + curlData;
        }
        if (strutil::contains(curlData, "page-not-found")) {
            curl_easy_cleanup(curl);
            return "404-" + uid;
        }

        curl_easy_cleanup(curl);
        return "offline-" + uid;
    }
    curl_easy_cleanup(curl);
    return "";
}

std::string downloaderTask(std::string vdata,std::string dir) {
    std::string uid = strutil::split(vdata, ",")[0];
    std::string vid = strutil::split(vdata, ",")[1];
    time_t t = std::time(nullptr);
    tm tm = *std::localtime(&t);
    std::stringstream sstime;
    sstime << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S_live_");
    std::string saveLocation = dir + "vid-ok/" + uid + "/" + sstime.str() + uid + ".mkv";
    std::string command = "yt-dlp --downloader ffmpeg -o \"" + saveLocation + "\" -q https://ok.ru" + vid;
    system(command.c_str());
    std::this_thread::sleep_for(std::chrono::seconds(20));
    return vdata;
}

void renamefolder(std::string uid, std::string path) {
    while (std::filesystem::exists(path + "/vid-ok/" + uid)) {
        std::error_code ec;
        std::filesystem::rename((path + "/vid-ok/" + uid), (path + "/vid-ok/" + uid + "-404-"), ec);
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void srvStart(base *data, int port) {
    svr.Post("/", [&](const httplib::Request& req, httplib::Response& res) {
        std::string msg = req.body;

        if (msg.find("a ") != std::string::npos) {

            if (msg.size() <= 2) {
                res.set_content("please add a UserID", "text/plain");
                return;
            }

            msg.erase(0, 2);

            std::vector<std::string> tmp_UserIDList = data->UserIDList;

            if (std::find(tmp_UserIDList.begin(), tmp_UserIDList.end(), msg) != tmp_UserIDList.end()) {
                res.set_content("\"" + msg + "\"" + " already in the database.", "text/plain");
                return;
            }

            std::string tmpResult = checkerTask(msg, &data->vErrorList);

            if (strutil::contains(tmpResult, "404-")) {
                res.set_content("\"" + msg + "\"" + " is not a valid UserID.", "text/plain");
                return;
            }

            if (strutil::contains(tmpResult, "offline-")) {
                data->UserIDList.push_back(msg);
                res.set_content("\"" + msg + "\"" + " added to the database.", "text/plain");
                return;
            }

            if (strutil::contains(tmpResult, "live-")) {
                data->UserIDList.push_back(msg);
                res.set_content("\"" + msg + "\"" + " added to the database and will shortly be downloaded.", "text/plain");
                return;
            }
        }

        if (msg.find("r ") != std::string::npos) {

            if (msg.size() <= 2) {
                res.set_content("please add a UserID to remove", "text/plain");
                return;
            }

            msg.erase(0, 2);

            std::vector<std::string> tmp_UserIDList = data->UserIDList;

            if (std::find(tmp_UserIDList.begin(), tmp_UserIDList.end(), msg) != tmp_UserIDList.end()) {
                data->toRemove.push_back(msg);
                res.set_content("\"" + msg + "\"" + " will be shortly removed.", "text/plain");
                return;
            }
            res.set_content("\"" + msg + "\"" + " is not a in the database.", "text/plain");
            return;
        }

        if (msg == "ea") {
            std::vector<std::string> tmpErrorList = data->vErrorList;

            if (tmpErrorList.empty()) {
                res.set_content("no errors", "text/plain");
                return;
            }

            std::string dat;
            for (int i = 0; i < tmpErrorList.size(); i++) {
                dat += tmpErrorList[i];
                if (i + 1 < tmpErrorList.size()) {
                    dat += ";";
                }
            }
            res.set_content(dat, "text/plain");
            return;
        }

        if (msg == "stop") {
            res.set_content("stopping...", "text/plain");
            svr.stop();
        }

        });

    svr.Get("/data", [&](const httplib::Request& req, httplib::Response& res) {
        std::string dat;
        dat += "\"name:" + data->name + "\";";
        dat += "\"version:" + data->version + "\";";

        dat += "\"downloading:";
        std::vector<std::string> TMPVidIDDownloading = data->VidIDDownloading;
        for (int i = 0; i < TMPVidIDDownloading.size(); i++) {
            dat += strutil::split(TMPVidIDDownloading[i], ",")[0];
            if (i + 1 < TMPVidIDDownloading.size()) {
                dat += ",";
            }
        }
        dat += "\";";

        dat += "\"uidlist:";
        std::vector<std::string> TMPUserIDList = data->UserIDList;
        for (int i = 0; i < TMPUserIDList.size(); i++) {
            dat += TMPUserIDList[i];
            if (i + 1 < TMPUserIDList.size()) {
                dat += ",";
            }
        }
        dat += "\";";

        dat += "\"ErrorListSize:" + std::to_string(data->vErrorList.size()) + "\";";

        dat += "\"WorkingDir:" + data->WorkingDir + "\";";

        res.set_content(dat, "text/plain");
        });

    svr.listen("127.0.0.1", port);
}

int main(int argc, char** argv) {
    base data;
    std::cout << data.name + " " << data.version << std::endl;

    cxxopts::Options options(data.name + " " + data.version, "OK.ru downloader");

    options.add_options()
        ("d,dir", "folder for save stuff", cxxopts::value<std::string>())
        ("p,port", "port for server", cxxopts::value<int>()->default_value("34568"))
        ("h,help", "Print usage")
        ("v,version", "Print version")
        ;
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    if (result.count("version")) {
        std::cout << data.name + " " << data.version + "\nBuild with: \n\n" + std::string(curl_version()) + "\nhttplib/" + CPPHTTPLIB_VERSION << std::endl;
        return 0;
    }

    data.WorkingDir = result["d"].as<std::string>();
    if (data.WorkingDir.back() != '\\') {
        data.WorkingDir += "\\";
    }

    data.load();
    curl_global_init(CURL_GLOBAL_DEFAULT);

    std::thread(srvStart, &data, result["p"].as<int>()).detach();

    while (!svr.is_running()) { std::this_thread::sleep_for(std::chrono::milliseconds(300)); }

    dp::thread_pool checkerTaskPool(2);

    std::vector<std::future<std::string>> VIDDownloaderTasks;

    while (svr.is_running()) {
        std::vector<std::future<std::string>> tmp_CheckerFutures;
        std::vector<std::string> tmp_list404;

        for (int i = 0; i < data.UserIDList.size(); i++) {
            tmp_CheckerFutures.push_back(checkerTaskPool.enqueue(checkerTask, data.UserIDList[i], &data.vErrorList));
        }

        for (int i = 0; i < tmp_CheckerFutures.size(); i++) {
            tmp_CheckerFutures[i].wait();
            std::string result = tmp_CheckerFutures[i].get();
            if (strutil::contains(result, "offline-")) {
                continue;
            }

            if (strutil::contains(result, "404-")) {
                std::thread(renamefolder, strutil::split(result, "404-")[1], data.WorkingDir).detach();
                for (int i = 0; i < data.UserIDList.size(); i++) {
                    if (data.UserIDList[i] == strutil::split(result, "404-")[1]) {
                        data.UserIDList.erase(data.UserIDList.begin() + i);
                        data.save();
                        break;
                    }
                }
                continue;
            }

            if (strutil::contains(result, "live-")) {
                result = strutil::split(result, "live-")[1];
                if (std::find(data.VidIDDownloading.begin(), data.VidIDDownloading.end(), result) == data.VidIDDownloading.end() ) {
                    data.VidIDDownloading.push_back(result);
                    VIDDownloaderTasks.push_back(std::async(downloaderTask, result, data.WorkingDir));
                    continue;
                }
                continue;
            }            
        }

        for (int i = 0; i < VIDDownloaderTasks.size(); i++) {
            std::future_status status = VIDDownloaderTasks[i].wait_for(std::chrono::seconds(0));

            if (status != std::future_status::ready) { continue; }

            for (int i = 0; i < data.VidIDDownloading.size(); i++) {
                if (data.VidIDDownloading[i] == VIDDownloaderTasks[i].get()) {
                    data.VidIDDownloading.erase(data.VidIDDownloading.begin() + i);
                    break;
                }
            }
            VIDDownloaderTasks.erase(VIDDownloaderTasks.begin() + i);
            i = 0;
        }

        for (int i = 0; i < data.toRemove.size(); i++) {
            for (int i = 0; i < data.UserIDList.size(); i++) {
                if (data.UserIDList[i] == data.toRemove[i]) {
                    data.UserIDList.erase(data.UserIDList.begin() + i);
                    data.save();
                    break;
                }
            }
        }


        std::this_thread::sleep_for(std::chrono::seconds(3));
    }

    curl_global_cleanup();
    return 0;
}