
#include <iostream>
#include <opencv2/opencv.hpp> // opencvの全てのモジュール
#include <unistd.h> // Linuxシステムコール用（sleep用）

using namespace std;
using namespace cv;

int main(){
    
    // VideoCaptureオブジェクトの初期化
    // GStreamerパイプライン（libcameraからのデータをopencvが扱える形式に変換するため必要）
    string pipeline = "libcamerasrc ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert ! videoscale ! appsink";
    VideoCapture cap(pipeline, CAP_GSTREAMER);    
    
    // カメラが正常にオープンできたか確認
    if(!cap.isOpened()) {
        cerr << "エラー：カメラを開けませんでした。" << endl;
        return -1;
    }
    Mat frame;

    // カメラのウォームアップ（最初の数フレームが不安定な場合があるので、最初の数フレームを読み捨てる）
    cout << "カメラを準備中..." << endl;
    for (int i = 0; i < 5; i++) {
        cap >> frame; // フレームを取得
    }
    sleep(2); // 2秒待機
    
    // フレームのキャプチャ
    if (!cap.read(frame)) {
        cerr << "エラー：フレームを読み取ることができませんでした。" << endl;
        // 完全にカメラが開いているかの最終チェック
        if (!cap.isOpened()) {
            cerr << "VideoCapture オブジェクトは開いていません。" << endl;
        }
        return -1;
     }
     
     // 画像をファイルに保存
     string filename = "image.jpg";
     if (imwrite(filename, frame)) {
         cout << "画像を正常に保存しました: " << filename << endl;
     } else {
         cerr << "エラー: 画像をファイルに保存できませんでした。" << endl;
     return -1;
     }

     // カメラの開放（基本はオブジェクトのデストラクタで自動的に解放されるが一応）
     cap.release();


     return 0;
}

