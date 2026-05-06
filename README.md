
# Raspberry Pi 4B 監視カメラシステム（LINE連携）

## ◇ 概要

Raspberry Piを用いて構築した、**リアルタイム監視カメラシステム**です。
顔検知をトリガーに録画・通知を行い、LINEを通じて遠隔から状況確認・操作が可能です。

→ 自宅の簡易セキュリティシステムとして実運用を想定しています。

 **「組み込み × 画像処理 × ネットワーク」** を統合したシステムとして開発しました。

---

## ◇ 目的

* 組み込みエンジニアとして必要な
  **ハードウェア制御 / 並行処理 / ネットワーク連携**の実装力を証明する
* 実運用を想定した**常時稼働システム設計**の経験を得る

---

## ◇ システムアーキテクチャ

<img width="3888" height="2704" alt="image" src="https://github.com/user-attachments/assets/402f1726-8089-40cf-9e33-b41f05f97cec" />


### ■ コンポーネント構成

- カメラ入力：映像取得
- 顔検知：画像処理（OpenCV）
- 録画制御：検知イベントに基づく録画管理
- ファイル保存：画像・動画データの保存
- Webサーバー：cpp-httplibを用いたHTTP通信処理（Webhook受信・画像/動画配信）
- LINE送信：Push / Reply APIによる通知・操作（WebhookイベントはWebサーバーで受信し、応答は本コンポーネントで処理）
- 状態管理：atomic変数による監視状態の制御
- GPIO制御：LEDおよびボタンによる入出力制御

---

### ■ データフロー
1. カメラからフレームを取得
2. OpenCVで顔検知を実行
3. 顔検知時に録画開始イベントを発行
4. 動画ファイルを保存
5. 保存した画像・動画のURLをLINEへ送信
6. ユーザーからのメッセージをWebhookで受信
7. コマンドに応じて撮影や監視状態を制御
8. Webサーバー経由で画像・動画データを外部公開（ngrok経由）

---

### ■ 並行処理構成
本システムでは処理の遅延を防ぐため、マルチスレッド構成を採用しています。

- スレッド1：カメラ監視・顔検知処理
- スレッド2：HTTPサーバー（外部リクエスト処理）

スレッド間の状態共有には `std::atomic` を使用し、
安全に録画状態や監視状態を管理しています。

---

## ◇ ハードウェア

* Raspberry Pi 4B
* Raspberry Pi Camera Module Rev 1.3
* LED（状態表示）
* ボタン（手動操作）
* ブレッドボード / ジャンパーワイヤ

---

### ■ 配線図

※配線図は[Wokwi](https://wokwi.com/)で作成しており、Raspberry Pi 4Bが選択できないため
Raspberry Pi Picoで代用しています。

GPIO構成（LED・ボタン・抵抗回路）は実機と同様です。

ボタンの回路は、外部プルアップを使用しています。
Raspberry Pi 4Bは内部プルアップも利用可能ですが、電気的な動作理解を深めるために外部プルアップで構成しています。

<img width="728" height="476" alt="image" src="https://github.com/user-attachments/assets/c96c560f-9c31-432d-a244-a8ac2ef2eff3" />

---

### ■ GPIO割り当て：
- 青LED：GPIO17
- 赤LED：GPIO27
- 緑ボタン：GPIO22
- 赤ボタン：GPIO23


---

## ◇ ソフトウェア　＆　使用技術

### 言語

* C++（C++17）

### ライブラリー

* OpenCV（画像処理・顔検知）
* cpp-httplib（軽量HTTPサーバー）
* pigpio（GPIO制御）
* nlohmann/json（JSON処理）

### ツール / サービス

* LINE Messaging API（通知・操作）
* ngrok（外部公開）
* cron（定期処理）
* tmux（常時稼働管理）

---

## ◇ 主な機能

### 1. リアルタイム顔検知 × 自動録画

* Haar Cascade Classifierを用いて顔を検知
* 検知時のみ録画を開始し、不要なデータを削減
* 一定時間顔が検出されなければ自動停止

---

### 2. LINEによる即時通知

* 顔検知時に以下を自動送信

  * 静止画
  * 動画URL
* 外出先からリアルタイムで状況確認が可能

---

### 3. ngrokによるリモートアクセス

* ローカル環境を安全に外部公開
* ブラウザやLINEから動画・画像へアクセス可能

---

### 4. Webhookによる双方向制御

LINE Messaging APIのWebhookにより、
ユーザーからのメッセージをトリガーに動作：

* 撮影コマンド → 即時写真送信
* 監視ON / OFF切り替え
* システムの終了
---

### 5. マルチスレッド設計

以下の処理を分離：

* 画像取得・顔検知スレッド
* HTTPサーバースレッド

→ これにより

* 検知遅延の抑制
* 外部アクセスのレスポンス向上
  を実現

---
### 6. GPIOによるデバイス制御（組み込み機能）

本システムでは、LEDとボタンを用いた直感的な操作・状態表示を実装しています。

#### ■ LEDによる状態表示
- 赤LED：監視モード中に常時点灯
- 青LED：顔検知時に点灯（イベント発生通知）

#### ■ ボタンによる操作
- 緑ボタン：監視モードのON/OFF切り替え
- 赤ボタン：プログラムの終了

これにより、ディスプレイがない環境でも
システムの状態把握および操作が可能となっています。

---

## ◇ 動作イメージ

### ■ 顔検知時の動作

- 顔を検知すると自動で録画開始
- 同時にLINEへ画像を送信
- 顔の最終検知後、5秒で録画を停止
- 動画URLをLINEへ送信

※実際のスマートフォン画面の様子です。
※顔とURLは隠しています。

<img width="300" height="550" alt="image" src="https://github.com/user-attachments/assets/53e146e1-812c-4e1c-8a3f-7e6b1366f355" />

---

### ■ LINE操作

- 「！」・・・写真を撮影
- 「？」・・・監視状態を返信
- 「監視停止」・・・監視を停止
- 「監視再開」・・・監視を再開
- 「プログラム終了」・・・メインプログラムを終了

その他の文字列の場合、LINEコマンドの説明が返信されます。

[動作動画](https://drive.google.com/file/d/1BBzG_szHbPuQdoSehjQq6ycj4AeqUDqQ/view?usp=sharing)　※実際のスマートフォン画面およびシステム動作の様子です。

---

### ■ ボタン操作とLED表示

- 緑ボタン：監視の停止/再開
- 赤ボタン：メインプログラムを終了
- 青LED：顔検知時に点灯
- 赤LED：監視中に点灯、終了時に5秒間点滅

[動作動画](https://drive.google.com/file/d/1bfLe2dcWyjpfJEdVI0TzId5PjSbYHyfx/view?usp=sharing)　※実際のスマートフォン画面およびシステム動作の様子です。

---


## ◇ デザイン／技術的特長

### ■ 並行処理の最適化

- `std::thread` による処理分離（監視処理とHTTP処理を分離）
- `std::atomic` を使用したスレッド間状態共有
  → データ競合を防止し、安定した動作を実現

---

### ■ イベント駆動設計

* 「顔検知」や「LINEメッセージ」をトリガーとした処理設計
*  イベント発生時のみ処理を行う設計とし、CPU負荷を抑制

---
### ■ 処理負荷の軽減

- 顔検知を毎フレームではなく、一定間隔（5フレームごと）で実行することでCPU負荷を削減
- フレームを縮小してから顔検知を行い、処理速度を向上

---

### ■ 安定動作の工夫

- HTTP通信にタイムアウトを設定し、ネットワーク遅延時のフリーズを防止
- ボタン入力に対してチャタリング対策（一定時間待機）を実装

---

### ■ スレッド間通信の工夫

- 状態管理フラグに `std::atomic` を使用し、データ競合を防止
- Webサーバーと監視処理間で安全に状態共有を実現

---

### ■ アクセス制御の工夫

- 監視停止中でもWebサーバー自体は稼働し続けるが、
  状態フラグ（std::atomic）により画像・動画データのレスポンスを制御
- これにより、外部からのアクセスを遮断しつつ、システム全体の停止を回避

---

### ■ 組み込み視点の工夫

* LEDで状態可視化（監視中 / 顔検知中）
* 軽量OS（Raspberry Pi OS Lite）で動作
* tmuxによる長時間安定稼働

---

### ■ 運用を意識した設計

* cronで古いファイルを自動削除
* 録画時間の制御によるストレージ最適化

---

### ■ ハードウェア制御との統合
GPIOを用いてLEDおよびボタンを制御し、
ソフトウェア処理とハードウェア操作を連携させています。

視覚的なフィードバック（LED）と物理操作（ボタン）を組み合わせることで、
組み込みシステムとしての実用性を高めました。

---

## ◇ 課題と解決策

### 課題1：検知と通信の処理遅延

* 問題：顔検知中にHTTP処理がブロッキング
* 解決：スレッド分離により並行処理化

---

### 課題2：スレッド間の状態管理

* 問題：録画状態の不整合
* 解決：`atomic`変数で安全に管理

---

### 課題3：外部アクセスの実現

* 問題：ローカル環境の公開
* 解決：ngrokを利用し簡易的に実装

---

### 課題4：LINEで動画を直接再生できない

* 問題：動画ファイルをそのまま送信すると再生が不安定
* 解決：動画URLを送信し、Webサーバー経由で再生する構成に変更

---

## ◇ 実行方法

### 動作環境
本システムは以下の環境で動作確認を行っています。
※ Raspberry Pi 4B以外の環境では動作未確認です。

- Raspberry Pi 4B
- Raspberry Pi OS Lite (64-bit)　※Raspberry Pi OS(64-bit)でも動作確認済み
- インターネット接続環境（LINE / ngrok使用のため）
  
---

### 1.リポジトリの取得
```bash
git clone https://github.com/MasanaoY/Pi4-SecurityCamera.git
cd Pi4-SecurityCamera
```
---

### 2.ディレクトリ構成

```
.
├- delete_old_files.sh　＃古いファイルを削除するスクリプト #ユーザーに合わせて絶対パスの更新が必要
├- line_video/　　　　　　＃動画を保存する場所
├- line_photo/　　　　　　＃写真を保存する場所
├- main.cpp　　　　　　　　＃メインプログラム
├- config.txt　　     　 ＃設定ファイル（チャネルトークン・ユーザーID、ngrok URL）
├- CMakeLists.txt     　＃ビルド用設定ファイル
├- httplib.h　　　     　＃cpp-httplibのヘッダーファイル
├- picam/　　　　   
├- nlohmann/　　　     　＃nlohmann/jsonを使用するためのファイル
└- build/　　　　　     　＃ビルドディレクトリ
```

---

### 3.事前準備（LINE/ngrok）

本システムを動作させるために、以下の設定が必要です。

### ■ ngrok の準備
　　※[こちら](https://qiita.com/Masanao_00/items/1d27bd52a040dd36f89f)が参考になるかもしれません。
- ngrokアカウントを作成
- ngrokのインストール
- 認証トークンを設定

```bash
ngrok config add-authtoken <YOUR_TOKEN>
```

- ngrok URLの確認

```bash
ngrok http 8080
```

---

### ■ LINE Messaging API の設定
　　※[こちら](https://qiita.com/Masanao_00/items/a5592c43eaed5a5baa0f)が参考になるかもしれません。
- LINE Developersでチャネルを作成
- Messaging APIを有効化
- 以下の情報を取得

  - Channel Access Token
  - ユーザーID
---

　　※[こちら](https://qiita.com/Masanao_00/items/2d6db1efa6225a9e2191)が参考になるかもしれません。
- Webhookの利用　有効化
- Webhook URLに ngrok のURLを設定

---

### 4.必要パッケージのインストール
　　※[こちら](https://qiita.com/Masanao_00/items/71934d368d0ad36cbc51)が参考になるかもしれません。

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config git
sudo apt install libssl-dev
sudo apt install libopencv-dev python3-opencv
sudo apt install pigpio
```

---

### 5. pigpioの設定

本システムは `pigpio` を使用しています。

本プログラムは `sudo` 権限で実行するため、
pigpioデーモン（pigpiod）を停止してください。

```bash
sudo systemctl stop pigpiod
```

---

### 6.顔検出用カスケードファイルのパス確認

- メインプログラム（main.cpp）の386行目`haarcascade_frontalface_default.xml`の絶対パスを適切に変更して下さい。

```main.cpp
386    std::string cascade_path = "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml";
```


OpenCVを`apt`でインストールした場合、共有ディレクトリに配置されているので、次の`ls`コマンドで確認して下さい。
```bash
ls /usr/share/opencv*/haarcascades/haarcascade_frontalface_default.xml
```

無い場合は、ダウンロードして下さい。
```bash
wget https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_frontalface_default.xml
```

---

### 7.設定ファイルの作成

- sample_config.txt のファイル名を config.txt に変更
- ファイル内の`=`の右側に **「チャネルのトークン、ユーザーID、ngrok URL」** を追記

```
- sample_config.txt を config.txt にリネーム

以下を設定：

CHANNEL_ACCESS_TOKEN=xxxx
USER_ID_TO_SEND=xxxx
NGROK_URL_BASE=xxxx
```
---

### 8.ビルド（Make）
```bash
cd build
cmake ..
make
```

---

### 9.実行
- ターミナルを２つ開き、それぞれ実行します。
```bash
ngrok http 8080
sudo ./main_app
```

---

## ◇　トラブルシューティング

### ■ カメラが起動しない

症状

- カメラを開けませんでした と表示される

対処法

- カメラが正しく接続されているか確認
- カメラが有効化されているか確認
- sudo raspi-config

→ Interface Options → Camera を有効化

※参考までに
[コマンドで撮影してみる](https://qiita.com/Masanao_00/items/0ff648260ed55dd873f4)

---

### ■ LINEにメッセージが送信されない

症状

- 何も送信されない / エラーが出る

対処法

- config.txt の設定を確認
```
CHANNEL_ACCESS_TOKEN
USER_ID_TO_SEND
NGROK_URL_BASE
```
- トークンの有効期限切れに注意

※ 1チャネルの通知上限は200通/月です。

---

### ■ ngrokのURLでアクセスできない

症状

- 画像や動画が表示されない

対処法

- ngrok http 8080
- 表示されたURLを config.txt に正しく設定
- ngrokは再起動するとURLが変わる可能性があるため注意

---

### ■ 顔を検知しない

症状

- 顔を検知したり、しなかったりする

対処法

- メインプログラム（main.cpp）内の503行目のパラメータを変更する

  ※現在のパラメータは私の環境下での最適値となっています。
  ※検出はグレースケールで行われるため、明るさや光の当たり方で精度はバラつきます。

```main.cpp

503     face_detector.detectMultiScale(gray_frame, current_faces, 1.1, 7, 0, cv::Size(30, 30));
 //                                                              ↑~~~~~~~~~~~~~~~~~~~~~~~~~~~↑
 //                                                             パラメータ:ここで検出精度をざっくり調整する

 //  1.1: スケールファクター。画像をどれだけ縮小して検出を行うか (1.05～1.4程度)小さいほど精度が高いが処理速度が長くなる。
 //  7: minNeighbors。矩形が何回検出されたら顔とみなすか (3～6程度)高いほど誤検出は減るが、検出漏れが増える可能性がある。
 //  0: flags。古いOpenCVとの互換性用。変更しない。
 //  Size(30, 30): minSize。検出する最小の顔サイズ (小さすぎると誤検出が増える)大きくすると小さすぎるノイズを顔と検出する誤検知が防げる。
```

--- 

## ◇ 今後の改善

### ■ セキュリティ強化
- 認証機能の追加（トークン認証など）
- Web公開時間の最小化（認証したLINEからのアクセス後の1分間のみWeb公開）

### ■ 機能拡張
- 顔認識（個人識別）への対応
- 個人識別に反応する挨拶（スピーカー追加）
- 動体検知の追加
- 顔認識と動体検知のモード切替

---

## ◇ 運用方法

### ■ cronによる定期削除

保存された画像・動画の肥大化を防ぐため、cronを用いて古いファイルを定期削除しています。
　　※[こちら](https://qiita.com/Masanao_00/items/40e92d12ce76632fa811)が参考になるかもしれません。
  
- シェルスクリプトを準備

　　スクリプトファイル（delete_old_files.sh）の`line_photo`と`line_video`のパスを環境に合わせて変更

- cronを実行
```bash
crontab -e
```
　以下を末尾に追加（※毎時0分に実行）
```
0 * * * * /home/pi/projects/Pi4-SecurityCamera/delete_old_files.sh
# パスは環境に合わせて変更して下さい
```


---

### ■ tmuxによる常時稼働

SSH切断後もプログラムを継続実行するため、tmuxを使用しています。
　　※[こちら](https://qiita.com/Masanao_00/items/a3e3342a73910e44cc8b)が参考になるかもしれません。

1.セッション作成
```bash
tmux new -s camera
```

2.ngrok起動
```bash
ngrok http 8080
```

3.ウィンドウ追加

　　Ctrl + b → c

4.メインプログラム稼働
```bash
sudo ./main_app
```

5.セッションから離脱

　　Ctrl + b → d

6.再接続
```bash
tmux attach -t camera
```
---

## ◇ 工夫した点（まとめ）

- イベント駆動設計により無駄な処理を削減
- std::atomic による安全なスレッド間通信
- 顔検知の間引きと解像度縮小による負荷軽減
- Webサーバーと監視処理の分離による応答性向上
- 監視停止中のアクセス制御による安全性向上


