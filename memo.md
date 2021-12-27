# 最強のエンジニア
- RDS,KVS,OS,Web Server,Cloudとか目に見えないけど、世界を支えているソフトウェアを作っている人

# 誰がいるだろう？
- 古橋さん(Fluentd,embulkとか作った人。TreasureDataの創業メンバー。)
- kazuho.okuさん(h2o作った人。QUICの策定者の一人。)
- matzさん(ruby)
- jose valim(elixir)
とか。

# どうやったらなれるだろう？
- すでにあるソフトウェアに貢献する
- 新しくソフトウェアを実装する

# 何するのが良いだろう？
- 以下を繰り返す
  - 勉強する
  - すでにあるソフトウェアに貢献する
  - すでにあるソフトウェアで解決できない課題をカバーしたソフトウェアを作る

# 何を勉強すれば良いんだろう？
- どんなソフトウェアを開発するか？
  - いろいろなソフトウェアを使ってみる
- 言語
  - Go
  - C/C++
  - python
  - shell
  - Rust
- ネットワーク
  - マスタリングTCP/IP
  - 各RFC
- OS
- ハードウェア
- 数学
- アルゴリズムとデータ構造
  - 片っ端らAtCoder
  - 持っている入門書を読み切る

<img src="20210728_コンピュータシステムの理論と実装/コースマップ.png">

- 上の画像を見ると以下のようにコンピュータは成立している。各分野の学習ソースを張っていく。
  - 高水準言語/アプリケーション
    - OSSを使う
    - OSSをフォークしてデバッグする
    - OSSのコードをリーディングする
    - OSSを変更する
    - OSSにPRを出す
  - ネットワーク
  - OS
    - [ゼロからのOS自作入門 Kindle版](https://www.amazon.co.jp/dp/B08Z3MNR9J/)
    - [はじめてのOSコードリーディング　――UNIX V6で学ぶカーネルのしくみ](https://www.amazon.co.jp/dp/B0821XY1QJ/)
    - [［試して理解］Linuxのしくみ ～実験と図解で学ぶOSとハードウェアの基礎知識](https://www.amazon.co.jp/dp/B079YJS1J1/)
    - [詳解UNIXプログラミング 第3版](https://www.amazon.co.jp/dp/B00KRB9U8K/)a
    - [作って理解するOS x86系コンピュータを動かす理論と実装](https://www.amazon.co.jp/dp/B07YBQY75J/)
  - コンパイラ
    - [低レイヤを知りたい人のためのCコンパイラ作成入門](https://www.sigbus.info/compilerbook)
    - [ふつうのコンパイラをつくろう　言語処理系をつくりながら学ぶコンパイルと実行環境の仕組み](https://www.amazon.co.jp/dp/B06XZSH7Q9/)
    - [RubyでつくるRuby ゼロから学びなおすプログラミング言語入門](https://www.amazon.co.jp/dp/4908686017/)
    - [Go言語でつくるインタプリタ](https://www.amazon.co.jp/dp/4873118220/)
  - バーチャルマシーン
  - アセンブラ
    - https://www.amazon.co.jp/dp/B013JR60QS
  - 機械語
  - コンピュータ・アーキテクチャ
    - [32ビットコンピュータをやさしく語る　はじめて読む486](https://www.amazon.co.jp/dp/B00OCF5YUA/)
  - ALU/メモリ/
  - ブール演算/順序論理
  - ブール演算

# 貢献してみたいソフトウェア
- cncf
  - k8s
- redis
- mysql
- mariadb
- crystal
- nginx
- vm
  - firecracker
- terraform
- sdk
  - aws
  - twilio
  - stripe
  - とか
- supabase
  - https://github.com/supabase/supabase
  - OSS製のFirebase
  - 商用のサービスをOSSで作っていくっていうアイディアが良いなぁ
- Crystal
  - 将来性を感じる。
  - RubyのSyntaxで型がある。育成コスト、採用コストがめちゃくちゃ低い。
  - 1バイナリで実行できる。
  - コマンドが最近の言語に追従している。formatter,linter,compiler,package managerなどがデフォルトで決まっている。プロジェクトの雛形を作る機能もある。rust,elixirと同じ。これ好き。
  - オブジェクト指向である。
  - 並行性が考慮されている。
  - 以上の特徴から将来性を感じる。これらを満たす言語は今の所ない気がする。

# 作ってみたいソフトウェア
- Crystalを使った軽量ウェブフレームワーク
  - 以下を実現したい。
    - 脆弱性の設定がデフォルトでされている
    - 他はsinatraぐらいの軽量さで十分
- AB Test Manager
  - フロントエンドだけでABテストの設定ができるOSS
  - 利用者はサーバを用意する前提だけど。
  - サーバにはAPIが生えている。
  - optionalで公式の管理画面を使える。
  - マネージド版はトラフィック量によって料金体系を変えたい。マネージド版はサーバとクライアントセット。
  - OSS版はAMI,Docker Imageを提供すると良さげ。たぶん、Redis,MySQL,ElasticSearchに依存するソフトウェアになると思うから、そういうのを含んだイメージにするとか。

# お金になる言語/FW
- PHP/Laravel
- Python/Flask or Django
- TypeScript/Vue/React
- Go
- Ruby/Rails
