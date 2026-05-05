
# Raspberry Pi 4B 監視カメラシステム（LINE連携）

## ◇ 概要

Raspberry Piを用いて構築した、**リアルタイム監視カメラシステム**です。
顔検知をトリガーに録画・通知を行い、LINEを通じて遠隔から状況確認・操作が可能です。

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
- カメラ入力：Raspberry Pi Camera Moduleから映像取得
- 画像処理：OpenCVによる顔検知
- 録画制御：顔検知イベントに応じて動画の開始・停止および保存を管理
- Webサーバー：cpp-httplibを用いたHTTP通信処理
- LINE連携：Messaging API（Push / Webhook）による通知・遠隔操作
- GPIO制御：LEDおよびボタンによる状態管理
- 
- カメラ入力：Raspberry Pi Camera Moduleから映像取得
- 顔検知・録画制御：OpenCVによる顔検知および録画制御
- ファイル保存：画像・動画データの保存
- Webサーバー：cpp-httplibを用いたHTTP通信処理
- LINE送信：Messaging API（Push / Webhook）による通知・遠隔操作
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
* 監視ON/OFF切り替え
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
- 緑ボタン：監視モードのON / OFF切り替え
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

※顔は隠しています。
<img width="300" height="550" alt="image" src="https://github.com/user-attachments/assets/53e146e1-812c-4e1c-8a3f-7e6b1366f355" />

---

### ■ LINE操作

- 「！」・・・写真を撮影
- 「？」・・・監視状態を返信
- 「監視停止」・・・監視を停止
- 「監視再開」・・・監視を再開
- 「プログラム終了」・・・メインプログラムを終了

その他の文字列の場合、LINEコマンドの説明が返信されます。

[動作動画](https://drive.google.com/file/d/1BBzG_szHbPuQdoSehjQq6ycj4AeqUDqQ/view?usp=sharing)

---

### ■ ボタン操作とLED表示

- 緑ボタン：監視の停止/再開
- 赤ボタン：メインプログラムを終了
- 青LED：顔検知時に点灯
- 赤LED：監視中に点灯、終了時に5秒間点滅

[動作動画](https://drive.google.com/file/d/1bfLe2dcWyjpfJEdVI0TzId5PjSbYHyfx/view?usp=sharing)

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
※ Raspberry Pi以外の環境では動作未確認です。

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
├- main.cpp　　　　　＃メインプログラム
├- config.txt　　　 ＃設定ファイル（チャネルトークン・ユーザーID、ngrok URL）
├- CMakeLists.txt　＃ビルド用設定ファイル
├- httplib.h　　　　＃cpp-httplibのヘッダーファイル
├- picam/　　　　   
├- nlohmann/　　　　＃nlohmann/jsonを使用するためのファイル
└- build/　　　　　　＃ビルドディレクトリ
```

---

### 3.事前準備（LINE/ngrok）

本システムを動作させるために、以下の設定が必要です。

### ■ ngrok の準備
　　※[こちら](https://qiita.com/Masanao_00/items/1d27bd52a040dd36f89f)が参考になるかもしれません。
- ngrokアカウントを作成
- 認証トークンを設定

```bash
ngrok config add-authtoken <YOUR_TOKEN>
```

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


Open CV`apt`でインストールした場合、共有ディレクトリに配置されているので、`ls`で確認できます。
```bash
ls /usr/share/opencv*/haarcascades/haarcascade_frontalface_default.xml
```


無い場合は、ダウンロードして下さい。
```bash
wget https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_frontalface_default.xml
```

---

### 7.設定ファイルの作成

- smple_config.txt のファイル名を config.txt に変更
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
```bash
ngrok http 8080
sudo ./main_app
```

## ◇　トラブルシューティング



## ◇ 運用方法











//残り
運用方法
トラブルシューティング

