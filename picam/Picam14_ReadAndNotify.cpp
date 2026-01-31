
#define CPPHTTPLIB_OPENSSL_SUPPORT //cpp-httplibのHTTPS機能を有効にする

#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include "httplib.h"


// 設定ファイルを読み込んで、キーと値のmapを返す関数
std::map<std::string, std::string>load_config(const std::string& filename) {
    std::map<std::string, std::string>config;
    std::ifstream file(filename);
    std::string line;

    if(!file.is_open()) {
        std::cerr << "設定ファイルを開けません:" << filename << std::endl;
        return config;
    }

    while(std::getline(file, line)) {
        // 空行やコメント（#で始まる行）はスキップ
        if(line.empty() || line[0] == '#')
            continue;

        // 「キー=値」を分解
        size_t pos = line.find('=');
        if(pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        config[key] = value;
    }
    return config;
}

const std::string LINE_API_BASE_URL = "https://api.line.me";
const std::string LINE_PUSH_MESSAGE_ENDPOINT = "/v2/bot/message/push";

// LINE APIにHTTP POSTリクエストを送信する関数
bool sendLineApiRequest(const std::string& endpoint, const std::string& body, const std::map<std::string, std::string>& config) {
    httplib::Client cli(LINE_API_BASE_URL.c_str());
    cli.set_connection_timeout(std::chrono::seconds(5)); // 接続タイムアウト (5秒)
    cli.set_read_timeout(std::chrono::seconds(10));      // 読み込みタイムアウト (10秒)

    httplib::Headers headers = {
        {"Authorization", "Bearer " + config.at("CHANNEL_ACCESS_TOKEN")}
    };

    // LINE APIへPOSTリクエストを送信
    auto res = cli.Post(endpoint.c_str(), headers, body, "application/json");

    // レスポンスの確認
    if (res) {
        if (res->status == 200) {
            std::cout << "LINE APIへの送信に成功しました！ レスポンス: " << res->body << std::endl;
            return true;
        } else {
            std::cerr << "LINE APIエラーが発生しました。ステータスコード: " << res->status << ", レスポンスボディ: " << res->body << std::endl;
            return false;
        }
    } else {
        std::cerr << "LINE APIへの接続エラーが発生しました: " << httplib::to_string(res.error()) << std::endl;
        return false;
    }
}

// テキストメッセージを送信する関数
bool sendTextMessage(const std::string& to_user_id, const std::string& text, const std::map<std::string, std::string>& config) {
    // JSON形式のメッセージペイロードを作成
    std::string body = R"({
        "to": ")" + to_user_id + R"(",
        "messages": [
            {
                "type": "text",
                "text": ")" + text + R"("
            }
        ]
    })";
    
    // LINE APIのプッシュメッセージエンドポイントにリクエストを送信
    return sendLineApiRequest(LINE_PUSH_MESSAGE_ENDPOINT, body, config);
}

// メイン関数
int main() {
    
    // ファイルの読み込み
    auto config = load_config("../config.txt");

    if(config.empty()) {
        std::cerr << "設定ファイルの読み込みに失敗しました" << std::endl;
        return 1;
    }

    std::cout << "LINEへメッセージを送信します..." << std::endl;

    // テキストメッセージを送信
    std::string message_to_send = "おはようございます。";
    
    if (sendTextMessage(config.at("USER_ID_TO_SEND"), message_to_send, config)) {
        
        std::cout << "メッセージの送信が完了しました。" << std::endl;
    } else {
        std::cerr << "メッセージの送信に失敗しました。" << std::endl;
    }

    return 0;
}

