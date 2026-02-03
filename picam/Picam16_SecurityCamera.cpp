
#define CPPHTTPLIB_OPENSSL_SUPPORT //cpp-httplibのHTTPS機能を有効にする

#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <map> // キー:値のデータ構造
#include "httplib.h"
#include <opencv2/opencv.hpp>
#include <chrono> // 時間計測用
#include <thread> // スレッドを使うために必要
#include <pigpio.h>
#include <unistd.h> // usleep()のために必要
#include "nlohmann/json.hpp" // nlohmann/jsonを使用
#include <atomic> // マルチスレッドで安全に使用できる変数の機能

using json = nlohmann::json;

// GPIOピン番号の定義（BCM番号）
#define LED_BLUE  17
#define LED_RED   27
#define BTN_GREEN 23
#define BTN_RED   24

// フラグ用の変数
std::atomic<bool> monitoring_enabled(true); // 監視状態、初期状態はON
std::atomic<bool> photo_request(false); // 写真要求、初期状態OFF
std::atomic<bool> program_end_request(false); // プログラム終了要求、初期状態OFF


// 設定ファイルを読み込んで、キーと値のmapを返す関数
std::map<std::string, std::string>load_config(const std::string& filename) {
    std::map<std::string, std::string>config;
    std::ifstream file(filename);
    std::string line;

    if(!file.is_open()) {
        std::cerr << "Error; " << filename << std::endl;
        return config;
    }

    while(std::getline(file, line)) {
        // 空行やコメント（#で始まる行）はスキップ
        if(line.empty() || line[0] == '#')
            continue;

        // =で分解し位置をposに格納、=が見つからずposがnposだったらスキップ
        size_t pos = line.find('=');
        if(pos == std::string::npos)
            continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        config[key] = value;
    }
    return config;
}


// LINEに送るエンドポイント
const std::string LINE_API_BASE_URL = "https://api.line.me";
const std::string LINE_PUSH_MESSAGE_ENDPOINT = "/v2/bot/message/push";
const std::string LINE_REPLY_MESSAGE_ENDPOINT = "/v2/bot/message/reply";

// LINE API 送信関数
// LINE APIにHTTP POSTリクエストを送信する関数
bool sendLineApiRequest(const std::string& endpoint, const std::string& body, const std::map<std::string, std::string>& config) {
    httplib::Client cli(LINE_API_BASE_URL.c_str()); // Clientオブジェクト作成、ClientはコンストラクタでURLをC言語文字列として受け取るため、.c_str()でURLをC言語文字列に変換

    // ネットワーク不安定な場合のフリーズ防止
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


// 画像メッセージを送信する関数
bool sendImageMessage(const std::string& to_user_id, const std::map<std::string, std::string>& config, std::string image_file) {
    std::string originalUrl = "https://" + config.at("NGROK_URL_BASE") + "/image?file=" + image_file;
    std::string previewUrl = originalUrl; // 簡略化のため同じURL

    // JSON形式のメッセージペイロード(送りたい文字列本体)を作成、生文字列リテラル「R"()"」でエスケープ文字が不要になる
    std::string body = R"({
        "to": ")" + to_user_id + R"(",
        "messages": [
            {
                "type": "image",
                "originalContentUrl": ")" + originalUrl + R"(",
                "previewImageUrl": ")" + previewUrl + R"("
            }
        ]
    })";
    
    // LINE APIのプッシュメッセージエンドポイントにリクエストを送信
    return sendLineApiRequest(LINE_PUSH_MESSAGE_ENDPOINT, body, config);
}

// テキストメッセージを送信する関数
bool sendTextMessage(const std::string& to_user_id, const std::string& text, const std::string& video_name, const std::map<std::string, std::string>& config) {
    std::string videoUrl = "https://" + config.at("NGROK_URL_BASE") + "/video?file=" + video_name;
    // video_nameが空ならURLを空にする
    if (video_name == "") {
        videoUrl = "";
    }

    // JSON形式のメッセージペイロード(送りたい文字列本体)を作成、生文字列リテラル「R"()"」でエスケープ文字が不要になる
    std::string body = R"({
        "to": ")" + to_user_id + R"(",
        "messages": [
            {
                "type": "text",
                "text": ")" + text + videoUrl + R"("
            }
        ]
    })";
    
    // LINE APIのプッシュメッセージエンドポイントにリクエストを送信
    return sendLineApiRequest(LINE_PUSH_MESSAGE_ENDPOINT, body, config);
}

// リプライメッセージを送信する関数
bool sendReplyMessage(const std::string& reply_token, const std::string& text, const std::map<std::string, std::string>& config) {

    json reply_json = {
        {"replyToken", reply_token},
        {"messages", {
            {
                {"type", "text"},
                {"text", text}
            }
        }}
    };

    std::string post_data = reply_json.dump();
    
    // LINE APIのリプライエンドポイントに変身を送信
    return sendLineApiRequest(LINE_REPLY_MESSAGE_ENDPOINT, post_data, config);
}


httplib::Server svr; // グローバルで定義(svr.stop()をメインループ内で呼ぶため)

//Webサーバーを起動し、画像リクエストを処理する関数
void start_web_server(int port, const std::map<std::string, std::string>& config) {

    // -画像配信のエンドポイント
    // ngrokのURL + /imageにアクセスが来たら処理が実行される
    svr.Get("/image", [](const httplib::Request& req, httplib::Response& res) {
        if (!monitoring_enabled.load()) {
            res.status = 403;
            res.set_content("Monitoring stopped", "text/plain");
            return;
        }

        if (!req.has_param("file")) {
            res.status = 400;
            res.set_content("missing file parameter", "text/plain");
            return;
        }
        // HTTPリクエストのパラメーターから値を取り出して文字列として受け取る
        std::string filename = req.get_param_value("file");
        std::string image_path = "../line_photo/" + filename; 

        // ファイルをバイナリで読む
        std::ifstream ifs(image_path, std::ios::binary);
        if (!ifs) {
            res.set_content("Image not found", "text/plain");
            res.status = 404;
            return;
        }

        // ファイル内容をそのまま読み込む
        std::string data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            
        // HTTPレスポンスでバイナリとして送る
        res.set_content(data, "image/jpeg");
    });

    
    // -動画配信のエンドポイント
    // ngrokのURL + /video.mp4 にアクセスが来たらこの処理が実行される
    svr.Get("/video", [](const httplib::Request& req, httplib::Response& res) {

        if (!monitoring_enabled.load()) {
            res.status = 403;
            res.set_content("Monitoring stopped", "text/plain");
            return;
        }
        
        if (!req.has_param("file")) {
            res.status = 400;
            res.set_content("missing file parameter", "text/plain");
            return;
        }
        // HTTPリクエストのパラメーターから値を取り出して文字列として受け取る
        std::string filename = req.get_param_value("file");
        std::string video_path = "../line_video/" + filename; 

        std::ifstream ifs(video_path, std::ios::binary);

        if (!ifs) {
            res.set_content("Video not found", "text/plain");
            res.status = 404;
            return;
        }

        // ファイル全体をバッファに読み込み
        std::string video_data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());


        // レスポンスとして動画バイナリを返す
        res.set_content(video_data, "video/mp4");
        res.status = 200;
        std::cout << "[Server] ビデオが正常に送信されました。" << std::endl;
    });


    // -Webhookリクエストへの処理
    // LINE Developersに設定したWebhookURLにアクセスが来たらこの処理が実行される
    svr.Post("/webhook", [&](const httplib::Request& req, httplib::Response& res) {

        // リクエストボディの解析
        json req_json;
        try {
            req_json = json::parse(req.body);
        } catch (json::parse_error& e) {
            std::cerr << "JSON解析エラー: " << e.what() <<std::endl;
            res.set_content("Bad Request", "text/plain");
            res.status = 400;
            return;
        }

        std::cout << "Webhookリクエストを受信しました。処理を開始します。" << std::endl;

        // メッセージイベントの抽出と処理
        // リクエストに含まれる全てのイベントをループ
        for (const auto& event : req_json["events"]) {
            // イベントタイプが「メッセージ」かつ、メッセージタイプが「テキスト」であるか確認
            if (event.contains("type") && event["type"] == "message" && 
                event.contains("message") && event["message"].contains("type") && event["message"]["type"] == "text") {

                std::string user_message = event["message"]["text"];
                std::string reply_token  = event["replyToken"];

                std::cout << "  >ユーザーメッセージ: [" << user_message << "]" << std::endl;

                // -応答ロジックの実装
                // ！＝写真を送信
                if (user_message == "！") {
                    if (!monitoring_enabled.load()) {
                        sendReplyMessage(reply_token, "監視が停止中のため、写真は表示されません。", config);
                    }
                    photo_request.store(true);    

                // ？＝監視状態を通知
                } else if (user_message == "？") {
                    if (monitoring_enabled.load()) {
                        sendReplyMessage(reply_token, "現在、監視中です。", config);
                    } else {
                        sendReplyMessage(reply_token, "現在、監視は停止中です。", config);
                    }

                // 監視停止＝監視とWeb公開を停止
                } else if (user_message == "監視停止") {
                    if (!monitoring_enabled.load()) {
                        sendReplyMessage(reply_token, "すでに監視は停止しています。", config);
                    } else {
                        monitoring_enabled.store(false);
                        sendReplyMessage(reply_token, "監視を停止します。（停止中は写真や動画は確認できません）", config);
                    }
                
                // 監視再開＝監視とWeb公開を再開    
                } else if (user_message == "監視再開") {
                    if (monitoring_enabled.load()) {
                        sendReplyMessage(reply_token, "すでに監視中です。", config);
                    } else {
                        monitoring_enabled.store(true);
                        sendReplyMessage(reply_token, "監視を再開します。", config);
                    }

                // プログラム終了＝プログラムを終了    
                } else if (user_message == "プログラム終了") {
                    program_end_request.store(true);
                
                // それ以外はコマンドリストを送信
                } else {
                    sendReplyMessage(reply_token, 
                        "次の[コマンド]を送信できます。\n[コマンド]：説明\n\n[！]：写真を撮影し、LINEに送信します。\n[？]：監視状態を確認できます。\n[監視停止]：監視を停止し、再開まで待機します。\n[監視再開]：監視を再開します。\n[プログラム終了]：プログラムを終了します。", config);
                }
            }
        }
    res.set_content("OK", "text/plain");
    res.status = 200;

    });

    std::cout << "[Server] Listening on port " << port << "..." << std::endl;
    // listen() はブロッキング関数（処理がここで止まって待ち受ける）
    svr.listen("0.0.0.0", port);
    // listen() が返ってくるのは通常、サーバー停止時のみ
}



// 時刻を文字列で取得する関数
std::string get_timestamp() {

    // 現在時刻の取得
    std::ostringstream oss; 
    auto t = std::time(nullptr);
    std::tm tm{}; // {}で構造体の初期化
    localtime_r(&t, &tm);

    // YYYYMMDD-HHMMSS 形式の文字列を作成
    oss << std::put_time(&tm, "%Y_%m_%d--%H_%M_%S");
    return oss.str();
}


// グローバルでVideoWriterを定義 (録画の開始/停止でオブジェクトを再生成するため)
cv::VideoWriter writer;

std::string video_filepath;
std::string photo_filepath;
std::string photo_filename;
std::string video_filename;
std::string message_to_send;
               
// メイン関数
int main() {
    
    // pigpioライブラリの初期化
    // gpioInitialise()は正常に初期化すれば0以上を、失敗すれば0未満を返す
    if (gpioInitialise() < 0) {
        std::cerr << "pigpioの初期化に失敗しました。" << std::endl;
        return 1;
    }

    // 出力ピンの設定
    gpioSetMode(LED_BLUE, PI_OUTPUT);
    gpioSetMode(LED_RED, PI_OUTPUT);

    // 入力ピンの設定
    gpioSetMode(BTN_GREEN, PI_INPUT);
    gpioSetMode(BTN_RED, PI_INPUT);

    const int SERVER_PORT = 8080;

    // 設定ファイルの読み込み
    auto config = load_config("../config.txt");

    if(config.empty()) {
        std::cerr << "設定ファイルの読み込みに失敗しました" << std::endl;
        return 1;
    }
    
    // Webサーバーを別スレッドで起動
    // std::thread::thread(関数名, 引数...)で新しいスレッドが生成され、関数が実行される
    std::thread server_thread(start_web_server, SERVER_PORT, std::cref(config));


    // 初期設定と検出機のロード
    cv::CascadeClassifier face_detector;
    std::string cascade_path = "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml"; 
     
    if (!face_detector.load(cascade_path)) {
    std::cerr << "顔カスケード分類機を読み込めませんでした" << cascade_path << std::endl;
        return -1;
    }

    // カメラの初期化
    std::string pipeline = "libcamerasrc ! video/x-raw, width=800, height=600, framerate=15/1 ! videoconvert ! videoscale ! appsink";
    cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
    
    if (!cap.isOpened()) { 
        std::cerr << "カメラを開けませんでした" << std::endl;
        return -1;
    }

    // 動画設定の取得
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    cv::Size frame_size(frame_width, frame_height);
    double fps = 15.0; // カメラFPS

    // 状態管理変数
    bool is_recording = false;
    auto last_detection_time = std::chrono::high_resolution_clock::now(); // 最後に顔を検知した時刻（初期値は現在時刻）

    const std::chrono::seconds RECORD_DURATION(5); // 録画を継続する時間（秒）

    // メインループ
    cv::Mat frame;
    std::vector<cv::Rect> current_faces;
    std::vector<cv::Rect> last_faces;
    int frame_count = 0;
    const int detection_interval = 5; // 5フレームに一度だけ検出

    std::cout << "モニターモードを開始：顔検出を待機しています。" << std::endl;

    while (true) { // 無限ループで監視を続ける
        
        // 赤ボタンが押されるか、LINEからリクエストがあればプログラム終了
        if (gpioRead(BTN_RED) == PI_LOW || program_end_request.load()) {
            svr.stop();
            break;
        }
        
        if (!cap.read(frame)) { break; }
        
        // LINEからリクエストがあれば、写真を保存し、LINEに送信
        if (photo_request.load()) { 
            photo_request.store(false);

            // 日時を取得
            std::string get_time2 = get_timestamp();

            // 写真を保存
            photo_filepath = "../line_photo/" + get_time2 + ".jpg";
            photo_filename = get_time2 + ".jpg";
            if (imwrite(photo_filepath, frame)) {
                std::cout << "画像を保存しました: " << photo_filepath << std::endl;
            } else {
                std::cerr << "画像を保存できませんでした" << std::endl;
            }
                
            // 写真をLINEに送信
            if (sendImageMessage(config.at("USER_ID_TO_SEND"), config, photo_filename)) {    
                std::cout << "メッセージの送信が完了しました。" << std::endl;
            } else {    
                std::cerr << "メッセージの送信に失敗しました。" << std::endl;
            }        
        }
        
        // 緑ボタンが押されたら、監視状態を切り替える（監視中 ⇄ 監視停止中）
        if (gpioRead(BTN_GREEN) == PI_LOW) {
            if (monitoring_enabled.load()) {
                monitoring_enabled.store(false);

                // LINEに変更を通知
                message_to_send = "監視を停止します。";
                video_filename = ""; 
                sendTextMessage(config.at("USER_ID_TO_SEND"), message_to_send, video_filename, config);
                // チャタリングを防ぐために少し待つ
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
           
            } else {
                monitoring_enabled.store(true);

                message_to_send = "監視を再開します。";
                video_filename = "";
                sendTextMessage(config.at("USER_ID_TO_SEND"), message_to_send, video_filename, config);

                // チャタリングを防ぐために少し待つ
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        }

        // 監視が停止中なら処理をスキップ、赤LEDは消灯
        if (!monitoring_enabled.load()) {
            gpioWrite(LED_RED, PI_LOW);
            // CPU負荷を下げるために少し待つ
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        // 監視が開始したら赤LEDを点灯
        gpioWrite(LED_RED, PI_HIGH);

        // 顔検出の間引き
        if (frame_count % detection_interval == 0) {

            // 解像度を半分に縮小
            cv::Mat small_frame;
            cv::resize(frame, small_frame, cv::Size(), 0.5, 0.5);

            cv::Mat gray_frame;
            cvtColor(small_frame, gray_frame, cv::COLOR_BGR2GRAY);

            // 検出のパラメータを厳しめに設定（minNeighbors=7, minSize=60x60）
            face_detector.detectMultiScale(gray_frame, current_faces, 1.1, 7, 0, cv::Size(30, 30));

            // 検出結果の座標を元画像サイズに戻す
            last_faces.clear();
            for (auto& face : current_faces) {
                face.x *= 2;
                face.y *= 2;
                face.width *= 2;
                face.height *= 2;
                last_faces.push_back(face);
            }
        }

        // 録画ロジックの核
        bool face_detected_this_frame = !last_faces.empty();
        
        // 顔を検知：タイマーをリセットし、録画を開始/継続
        if (face_detected_this_frame) {

            // 顔を検知したら青LED点灯
            gpioWrite(LED_BLUE, PI_HIGH);

            // 最後に検知した時刻を現在時刻に更新（タイマーリセット）
            last_detection_time = std::chrono::high_resolution_clock::now();

            // 録画が始まっていなければ新しく開始する
            if (!is_recording) {

                // 日時を取得
                std::string get_time = get_timestamp();

                video_filepath = "../line_video/" + get_time + ".mp4";
                video_filename = get_time + ".mp4";
                writer.open(video_filepath, cv::VideoWriter::fourcc('H', '2', '6', '4'), fps, frame_size);

                if (writer.isOpened()) {
                    is_recording = true;
                    std::cout << "[録画開始]顔検出！録画中:" << video_filepath << std::endl;
                }


                // 写真を保存
                photo_filepath = "../line_photo/" + get_time + ".jpg";
                photo_filename = get_time + ".jpg";
                if (imwrite(photo_filepath, frame)) {
                    std::cout << "画像を保存しました: " << photo_filepath << std::endl;
                } else {
                    std::cerr << "画像を保存できませんでした" << std::endl;
                }
                
                // 写真をLINEに送信
                if (sendImageMessage(config.at("USER_ID_TO_SEND"), config, photo_filename)) {    
                    std::cout << "メッセージの送信が完了しました。" << std::endl;
                } else {    
                    std::cerr << "メッセージの送信に失敗しました。" << std::endl;
                } 
            }
        } else {
            // 顔を検知していない時は青LEDを消灯
            gpioWrite(LED_BLUE, PI_LOW);
        }


        // 検知しなかった場合：タイマーをチャックし、録画を終了
        if (is_recording) {
            auto current_time = std::chrono::high_resolution_clock::now();
            auto time_elapsed = std::chrono::duration_cast<std::chrono::seconds>(current_time - last_detection_time);

            if (time_elapsed >= RECORD_DURATION) {
                // 5秒経過したら録画を終了
                writer.release();
                is_recording = false;
                std::cout << "録画停止：最後の検出から5秒経過" << std::endl;


                // テキストメッセージを送信
                message_to_send = "動画を撮影しました。";
                
                // テキストとvideoのURLを送信
                if (sendTextMessage(config.at("USER_ID_TO_SEND"), message_to_send, video_filename, config)) {    
                    std::cout << "メッセージの送信が完了しました。" << std::endl;
                } else {    
                    std::cerr << "メッセージの送信に失敗しました。" << std::endl;
                } 
            }
        }


        // 描画は常に実行
        cv::Scalar color = is_recording ? cv::Scalar(0, 0, 255) : cv::Scalar(0, 255, 0); // 録画中は赤、待機中は緑
        for (const auto& face : last_faces) {
            rectangle(frame, face, color, 2);
        }

        // 録画中の場合、フレームをファイルに書き込む
        if (is_recording) {
                writer.write(frame);
        }

        frame_count++;
        // 負荷軽減のため、わずかに待機時間を入れる
        std::this_thread::sleep_for(std::chrono::milliseconds(1));


    }

    // プログラム終了をLINEに通知
    message_to_send = "プログラムを終了します。";
    video_filename = "";
    sendTextMessage(config.at("USER_ID_TO_SEND"), message_to_send, video_filename, config);
    
    
    // サーバースレッドを終わらせる処理
    if (server_thread.joinable()) {
        server_thread.join();
        std::cout << "サーバースレッドを終了" << std::endl;
    }
    
    // 終了処理
    if (writer.isOpened()) { writer.release(); }
    cap.release();
    std::cout << "プログラム終了処理を実行" << std::endl;

    // プログラム終了前に赤LEDチカチカ
    for (int i = 0; i < 10; i++) {
        gpioWrite(LED_RED, PI_HIGH);
        usleep(250000); // 250ms待機
        gpioWrite(LED_RED, PI_LOW);
        usleep(250000); // 250ms待機
    }

    // プログラム終了時のgpioのクリーンアップ
    gpioTerminate();
    return 0;
}
