# 1章 ドメイン駆動設計とは何か
- ソフトウェアを適用する領域をドメインと呼ぶ。
- ドメインの知識を活かした設計をすることをドメイン駆動設計と呼ぶ。
- また。ドメイン知識を活かすためのオブジェクト指向言語におけるパターンとかがよく取り上げられる。
- ソフトウェアの設計、実装にドメイン知識を役立てるためにドメインモデリングが行われ、生成されるドメインモデルをもとに設計、実装が行われる。
- モデルとは具象を抽象化したものであり、モデリングとはこれを行う作業のことである。すなわち、ドメインモデルとはソフトウェアを適用する領域を抽象化したものである。
- ドメインモデルがソフトウェア開発において必要なのは、現実世界を全てコードで実装するのは不可能であり、ドメインを明確にすることで、どこまでソフトウェアで表現するのか決めるためにも重要である。また、表現をどの深さまで行うのか決める上でも重要である。

# 2章 システム固有の値を表現する「値オブジェクト」
- システム固有の値を表したオブジェクトのこと。仕事ナビなら、Tierが当てはまりそう。
- 値は3つの性質を持つ
  - 不変である
  ```crystal
  class FullName
    property :first_name, :last_name
    def initialize(@first_name : String, @last_name : String)
    end
  end
  full_name = FullName.new(first_name: 'yamaguchi', last_name: 'kohei')
  # ↓はfull_nameを値オブジェクトとするなら、やってはいけない。値オブジェクトは不変であるべきだから。
  full_name.first_name = 'yamada'
  ```
  - 交換が可能である
  不変であるがゆえに、値を変更するには、値を作り直す必要がある。↓の具合に。
  ```crystal
  full_name = FullName.new(first_name: 'yamaguchi', last_name: 'kohei')
  full_name = FullName.new(first_name: 'yamada', last_name: 'taro')
  ```
  - 等価性によって比較される
  ```crystal
  1 === 1 # true
  ```
  みたいに比較することができることを値は期待される。
  なので、
  ```crystal
  class FullName
    def ===(comp: self)
      self.first_name === comp.first_name && self.last_name === comp.last_name
    end
  end
  ```
  みたいなメソッドが求められる。
- 値オブジェクトに切り分けるルールは成瀬さん的には
  - ルールが存在しているか
    氏名には名と姓で構成されるルールがある。なので、氏名は値オブジェクトとなる。
  - それ単体で取り扱いたいか
    仮に名と姓を値オブジェクトとする場合、それらを単体で取り扱うことがあるか？を問う。あるとして、それは名前の構成要素として扱えれば良いのか、どうなのか。yesならば、Nameみたいなクラスを用意すれば良いのでは？っていう考え方。
- 値オブジェクトはオブジェクトなので、データと振る舞いを持つことができる。
- 値オブジェクトを使うモチベーションは以下。
  - 表現力を増す
    ただの文字列、数値などと比較すると、値オブジェクトは多くの表現を持つことができる。
    full_nameに関しても姓と名の組み合わせであり、それぞれ制約が存在しており、、、みたいなことをクラスを見ると分かる。
    プリミティブな型だとそれが何を表しているのか分かりづらいときがあるが、それを防げる。
  - 不正な値を存在させない
    値オブジェクトを生成する時に、コンストラクタにvalidationを付けておけば、不正な値が存在しないことを保証できる。
  - 誤った代入を防ぐ
  - ロジックの散在を防ぐ    
    値オブジェクトに関するロジックは全てカプセル化できるので、便利。DRY原則に則る上でも。

# 3章 ライフサイクルのあるオブジェクト「エンティティ」
- エンティティは値オブジェクトと同じくドメインオブジェクトの一種である。値オブジェクトとの違いは属性によって識別されるのではなく、同一性(ID)によって識別されるのがエンティティである。
多くのシステムのユーザなどがそれに当たる。ユーザの情報は変更可能だが、情報を変えたからと言ってユーザが変わるわけではない。これと同じ。
- エンティティの性質は以下の通り。
  - 可変である
  - 同じ属性であっても区別される
  - 同一性により区別される
- ドメインオブジェクトを定義する意味は以下の通り。
  - コードのドキュメント性が高まる
    ドメインオブジェクトを読めば、オブジェクトがどういうデータ、振る舞い、制約を持っているのかが分かる。
  - ドメインにおける変更をコードに伝えやすくする
    ドメインモデルに変更があった場合に、それを反映する場所はドメインオブジェクトとなる。だから、変更しやすい。

# 4章 不自然さを解決する「ドメインサービス」
- DDDの中にはドメインサービスとアプリケーションサービスの2つが存在する。
- ドメインサービスとはドメインモデルに含まれるデータ、振る舞いなのだが、エンティティ、値オブジェクトに定義すると、オブジェクトが不自然になってしまう場合があり、そのような振る舞いとデータを定義するオブジェクトのことである。
- 例えば、以下のコードは不自然さがある。
```crystal
class User
  def exists?(other : User)
    # userはすでに存在する？
  end
end
```
これだと呼び出すときには
```crystal
user = User.new
user.exists?(user)
```
とか
```crystal
user = User.new
other_user = User.new
user.exists?(other_user)
```
みたいな呼び方になる。これは不自然さがすごい。こういうときに
```crystal
class UserService
  def exists?(user : User)
    # userはすでに存在するか確認
  end
end

user_service = UserService.new
user = User.new
user_service.exists?(user)
```
みたいにドメインサービスを導入できる。
しかし、ドメインサービスを乱用して、エンティティや値オブジェクトにあるべき振る舞いをドメインサービスに移行すると、ドメインオブジェクトが貧血症になる(ドメイン貧血症)ので、気をつけましょう。
- ドメインサービスもあくまでドメインモデルをソフトウェアで表現するためのオブジェクトである。なので、DBへのアクセス方法や入出力のようなシステム世界のロジックは書かれるべきではない。それはリポジトリやアプリケーションサービスなどに書かれるべきである。

# 5章 データにまつわる処理を分離する「リポジトリ」
- システムでオブジェクトを永続化し、再構築するには何かしらのデータストアが必要になる。リポジトリはデータを永続化し再構築するといった処理を抽象的に扱うためのオブジェクトである。
- リポジトリはドメインモデルに由来するオブジェクトではない。これはドメインオブジェクトをそれたらしめるために必要なオブジェクトである。ドメインモデルを表すオブジェクトだけではシステムは作れない。だから、ドメインオブジェクトにシステム特有のコードをおいてしまうと、ドメインオブジェクトではなくなってしまう。だから、そのようなコードはドメインオブジェクト以外のオブジェクトに実装しようっていう話。
- リポジトリの裏側にあるデータベースは一切気にしない。

# 6章 ユースケースを実現する「アプリケーションサービス」
- ドメインオブジェクトを操作して、アプリケーションを実現するサービスオブジェクトのこと。
- アプリケーションサービスにドメインルールは記述しない。記述すると、ドメイン貧血症を招き、変更コストの高いコードとなる。
- アプリケーションサービス(に関わらず)を作る時に凝集度が大切である。凝集度を高めることで単一責任の法則などを満たしやすい。堅牢性、信頼性、再利用性、可読性を上げられる。

# 7章 柔軟性をもたらす依存関係のコントロール
- DI Container
- Service Locator
- Clean Architectureとオブジェクト指向設計ガイドを読めば分かる

# 8章 ソフトウェア・システムを組み立てる
- あんま面白くない

# 9章 複雑な生成処理を行う「ファクトリ」
- コンストラクタの中で他のオブジェクトを生成する必要があるとき、ファクトリの導入を検討する。

# 10章 データの整合性を保つ
- RDBなどのユニークキー制約を使うのはいい考えだが、ユニーク制約をDBの仕組みに依存するのは良くない。
なぜならば、
- 重複してはならないというのはドメインルールであり、ドメインオブジェクトで表されるべきものであるから。
- 特定の技術基盤に依存するから。

# 11章 ユースケースを組み立てる
- 色々な実践

# 12章 ドメインルールを守る「集約」
- ルートオブジェクトからのみ、集約されたオブジェクトの変更ができる。
```crystal
class Circle
  getter :circle_id
  property :owner, :members

  def join(member: User)
    raise Error if (!user)
    raise Error if members.count >= 29

    members.add(user)
  end
end

circle.join(user)
```

# 13章 仕様

# 14章 アーキテクチャ
- ヘキサゴルアーキテクチャ、クリーンアーキテクチャ、レイヤードアーキテクチャ。

# 15章 ドメイン駆動設計のとびらを開こう

