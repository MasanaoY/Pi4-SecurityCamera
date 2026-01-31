#include <iostream>
#include <fstream>
#include <string>
#include <map>

// 設定ファイルを読み込んで、キーと値のmapを返す関数
std::map<std::string, std::string> load_config(const std::string& filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);
    std::string line;

    if (!file.is_open()) {
        std::cerr << "設定ファイルを開けません: " << filename << std::endl;
        return config;
    }

    // 一行ずつ取り出す
    while (std::getline(file, line)) {
        // 空行やコメント（#で始まる行）はスキップ
        if (line.empty() || line[0] == '#')
            continue;

        // 「キー=値」を分解
        size_t pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        config[key] = value;
    }

    return config;
}

int main() {
    auto config = load_config("../config.txt");

    if (config.empty()) {
        std::cerr << "設定ファイルの読み込みに失敗しました。" << std::endl;
        return 1;
    }

    std::cout << "CHANNEL_ACCESS_TOKEN: " << config["CHANNEL_ACCESS_TOKEN"] << std::endl;
    std::cout << "USER_ID_TO_SEND: " << config["USER_ID_TO_SEND"] << std::endl;
    std::cout << "NGROK_URL_BASE: " << config["NGROK_URL_BASE"] << std::endl;

}



