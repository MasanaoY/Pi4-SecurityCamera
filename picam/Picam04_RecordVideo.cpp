#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main() {
    // 1. VideoCaptureオブジェクトの初期化
    string pipeline = "libcamerasrc ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert ! videoscale ! appsink";
    VideoCapture cap(pipeline, CAP_GSTREAMER);  

    if (!cap.isOpened()) {
        cerr << "エラー: カメラを開けませんでした。" << endl;
        return -1;
    }
    
    // 2. 録画設定の取得と定義
    // カメラのフレーム幅と高さを取得（録画ファイルの設定に必要）
    int frame_width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
    Size frame_size(frame_width, frame_height);
    
    // フレームレートの設定 (FPS)
    double fps = 30.0; 

    // 3. VideoWriterオブジェクトの初期化
    // output.aviは保存するファイル名
    // MJPGはラズパイで広くサポートされているコーデック
    VideoWriter writer("output.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, frame_size);
    

    if (!writer.isOpened()) {
        cerr << "エラー: Video Writerを開けませんでした。" << endl;
        return -1;
    }
    
    cout << "Video Writerを初期化しました。10秒間の録画を開始します。" << endl;

    Mat frame;
    int frame_count = 0;
    // 10秒間の録画 (30 FPS * 10秒 = 300フレーム)
    const int max_frames = static_cast<int>(fps * 10); 

    // 4. フレーム処理と書き込みのループ
    while (frame_count < max_frames) {
        
        if (!cap.read(frame)) {
            cerr << "ビデオストリームからフレームを読み取ることができません。終了します。" << endl;
            break;
        }

        // フレームをファイルに書き込む (録画処理)
        writer.write(frame);
        
        cout << "Recording Frame: " << frame_count + 1 << "/" << max_frames << endl;
        
        frame_count++;
    }

    // 5. ループ終了。オブジェクトの解放
    cap.release();
    writer.release(); // 録画を終了し、ファイルをクローズする
    
    cout << "録画が完了しました。ファイルは output.avi として保存されました。" << endl;

    return 0;
}


