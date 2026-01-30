#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

int main() {

    // 1. 顔検出器の初期化
    CascadeClassifier face_detector;
    string cascade_path = "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml"; 
    
    if (!face_detector.load(cascade_path)) {
        cerr << "エラー: 顔カスケードファイルを読み込めませんでした:" << cascade_path << endl;
        cerr << "'haarcascade_frontalface_default.xml' が実行可能ファイルと同じディレクトリにあること、または絶対パスがパスが合っていることを確認してください。" << endl;
        return -1;
    }
    cout << "顔カスケードファイルが正常に読み込まれました。" << endl;


    // 2. VideoCaptureオブジェクトの初期化 
    string pipeline = "libcamerasrc ! video/x-raw, width=800, height=600, framerate=15/1 ! videoconvert ! videoscale ! appsink";
    VideoCapture cap(pipeline, CAP_GSTREAMER);

    if (!cap.isOpened()) {
        cerr << "エラー: カメラを開けませんでした。" << endl;
        return -1;
    }
    
    // 3. 動画設定の取得 (解像度、FPS)
    int frame_width = static_cast<int>(cap.get(CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(CAP_PROP_FRAME_HEIGHT));
    Size frame_size(frame_width, frame_height);
    double fps = 15.0; // 録画FPS


    // 4. VideoWriterオブジェクトの初期化 
    VideoWriter writer("faces.avi", 
                       VideoWriter::fourcc('M', 'J', 'P', 'G'), 
                       fps, 
                       frame_size);

    if (!writer.isOpened()) {
        cerr << "エラー: Video Writerを開けませんでした。" << endl;
        return -1;
    }
    
    cout << "Video Writerを初期化しました。顔検出機能を使って10秒間録画を開始します..." << endl;

    Mat frame;
    Mat gray_frame; // 顔検出はグレースケール画像で行うため
    vector<Rect> current_faces; // 今回検出された顔の矩形を格納するベクトル
    vector<Rect> last_faces; // 前回検出された顔（描画用）
    
    int frame_count = 0;
    const int max_frames = static_cast<int>(fps * 10); // 10秒間の録画
    const int detection_interval = 5; // 5フレームに一度検出する

    cout << "最適化された顔検出で録画を開始します..." << endl;

  
    // 5. メインループ: フレーム取得 -> 検出 -> 描画 -> 録画
    while (frame_count < max_frames) {
        
        if (!cap.read(frame)) {
            cerr << "警告: ビデオ ストリームからフレームを読み取ることができません。終了します。" << endl;
            break;
        }
    
        // --- 顔検出処理 ---
        
        // 検出処理の間引き（5フレームに一度）
        if (frame_count % detection_interval == 0) {


            // 解像度を半分に縮小
            Mat small_frame;
            resize(frame, small_frame, Size(), 0.5, 0.5);
            // BGRフレームをグレースケールに変換 (Haar Cascadeはグレースケールで動作)
            // 縮小フレームで検出を実行 
            cvtColor(small_frame, gray_frame, COLOR_BGR2GRAY);
            // 顔検出の実行
            face_detector.detectMultiScale(gray_frame, current_faces, 1.1, 7, 0, Size(30, 30)); 
            // パラメータ:
            //  1.1: スケールファクター。画像をどれだけ縮小して検出を行うか (1.05～1.4程度)小さいほど精度が高いが処理速度が長くなる
            //  7: minNeighbors。矩形が何回検出されたら顔とみなすか (3～6程度)高いほど誤検出は減るが、検出漏れが増える可能性がある。
            //  0: flags。古いOpenCVとの互換性用
            //  Size(30, 30): minSize。検出する最小の顔サイズ (小さすぎると誤検出が増える)大きくすると小さすぎるノイズを顔と検出する誤検知が防げる。 // 解像度の縮小に伴い半分に変更する
            
            // 検出結果の座標を元のサイズに戻す
            last_faces.clear();
            for (auto& face : current_faces) {
                face.x *= 2;
                face.y *= 2;
                face.width *= 2;
                face.height *= 2;
                last_faces.push_back(face); // 結果をlast_facesの末尾に保存
            }
        }

        // 検出された顔に緑色の枠を描画
        for (const auto& face : last_faces) {
            rectangle(frame, face, Scalar(0, 255, 0), 2); // 緑色の2ピクセル幅の枠
        }
        
        // 描画されたフレームをファイルに書き込む 
        writer.write(frame);
        
        cout << "Frame: " << frame_count + 1 << " | Detection: " 
             << (frame_count % detection_interval == 0 ? "YES" : "NO") 
             << " | Faces: " << last_faces.size() << endl;
        
        frame_count++;
    }

    // 6. ループ終了。オブジェクトの解放
    cap.release();
    writer.release(); 
    
    cout << "録画完了：ファイルはfaces.aviとして保存されました。" << endl;

    return 0;
}

