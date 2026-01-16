#!/bin/bash
find /home/pi/line_photo /home/pi/line_video -type f -mmin +360 -delete

#find 　/home/pi/line_photo    　-type f     　-mtime +1    -mmin +360  　　-delete
#検索 　ディレクトリの絶対パス 　f(ファイル) 　1日以上経過　360分以上経過　 削除
