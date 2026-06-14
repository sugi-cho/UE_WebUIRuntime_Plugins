# WebUI Plugin Usage

このリポジトリは Unreal Engine プロジェクト本体ではなく、`Plugins/` 配下の WebUI 関連 Plugin のみを管理します。

## Author

- Author: Hironori Sugino
- Website: https://sugi.cc
- Original Repository: https://github.com/sugi-cho/UE_WebUIRuntime_Plugins

## Plugin 構成

- `WebUIRuntime`
  - Core Runtime Plugin
  - Actor / Component の Web UI 化、schema API、property 変更、button event の基盤
- `WebUI_NDI`
  - `WebUIRuntime` の拡張 Plugin
  - NDI source picker など、NDI 連携用の追加機能

## 使い方

1. Unreal Engine プロジェクトの `Plugins/` にこのリポジトリの Plugin を配置します。
2. 対象プロジェクトの `.uproject` で `WebUIRuntime` と必要なら `WebUI_NDI` を有効化します。
3. Project Settings の `Plugins > Web UI Runtime` で `Port` を設定します。
4. 同じ画面の `Allow Remote Access` を有効にすると、LAN 内の端末からもアクセスできます。
5. `WebUIHostComponent` を持つ Actor を配置して Web UI を起動します。
6. UE 内に埋め込み表示したい場合は、`Web Browser Widget` プラグインも有効化します。

複数の Actor が `WebUIHostComponent` を持つ場合、Web UI は `WebUIId` ごとのタブで切り替えます。
タブ名は `WebUIId` を使います。
タブ下の説明には `WebUIHostComponent` の `Description` を表示します。
WebUI に出すボタンやパラメータは、名前の先頭に `00_` のようなプレフィックスを付けて管理します。
`WUI00_` のような互換プレフィックスも同じく並び順として扱えます。
数字部分は並び順に使われ、表示時はプレフィックスを除いた名前だけが UI に出ます。

## `WebUIHostComponent` について

- Web UI の起動と集約を担当します。
- Actor ごとの識別子として `WebUIId` を持ちます。
- タブ説明用の `Description` を持ちます。
- `WebUIId` が未設定なら Actor 名を使います。
- ポートは個別設定ではなく、Project Settings の 1 設定を使います。
- `Allow Remote Access` を有効にすると、HTTPServer の bind を `any` にして LAN 内から見えるようにします。

## `WebUIHostActor` について

- `WebUIHostComponent` を内包した Actor です。
- まずは `WebUIHostActor` を置く運用を推奨します。
- Actor 側の `WebUIId` / `Description` / `bAutoStartServer` を使い、内部で `WebUIHostComponent` を同期します。
- 既存 Actor に後付けしたい場合は、従来どおり `WebUIHostComponent` 単体でも使えます。
- ボタンは `WebUIHostComponent` 側を上段コントロールとして使います。

## `WebUIComponent` について

- 実際の UI 要素は `WebUIComponentBase` か、その派生クラスに置きます。
- `WebUIComponentBase` はそのまま使ってもよいですし、独自の Component を作って継承しても使えます。
- たとえば NDI 用の `WebUINDIComponent` のように、用途ごとに派生 Component を追加できます。
- `WebUIHostComponent` を持つ Actor には、Actor 自身の `WebUI` 変数も上段に表示され、その下に Component ごとのセクションが並びます。
- `WebUIHostComponent` のボタンは、Actor セクション内の共通コントロールとして表示されます。
- WebUI に出す対象は `WUI00_` などの名前規則で判定します。

## 画像表示

`WebUIImageComponent` を追加すると、Web UI で画像を表示できます。

### 設定方法

1. Actor に `WebUIImageComponent` を追加します。
2. `SourceTexture` に表示したい `Texture2D` か `RenderTarget` を設定します。
3. `WebUIImageSlot` で表示先を選びます。
4. `bForceOpaqueRenderTargetImage` は、RenderTarget の alpha を不透明にしたいときだけ使います。

### `WebUIImageSlot`

- `Preview`
  - タブ上段のプレビュー領域に表示します。
- `Icon`
  - タブ見出しのアイコンとして表示します。
- `Inline`
  - コンポーネントのプロパティ欄の直後に表示します。

### 配信方式

- `Texture2D`
  - HTTP 経由で静的画像として配信します。
- `RenderTarget`
  - WebSocket 経由でフレーム配信します。
- `MediaTexture`
  - HTTP 経由で定期更新します。
  - 再生前は透明画像を返し、準備ができたら自動的に表示します。
- `NDI`
  - `WebUI_NDI` を有効にすると、`NDIMediaTexture2D` も `WebUIImageComponent` の `SourceTexture` として扱えます。
  - NDI テクスチャは内部的にフレーム読み出しして表示します。

### 補足

- `RenderTarget` は WebSocket 配信を使うため、`WebUIRuntime` の `WebSocketPort` が有効である必要があります。
- WebSocket の購読は接続ごとに管理されます。
- クライアントはタブ切替時に、現在の `WebUIId` と表示中の画像キーを `subscribe` で送り直します。
- 再接続直後は `snapshotRequest` を送って、最新フレームを再取得します。
- 購読していない `WebUIId` の RenderTarget フレームは送られません。
- NDI 画像を使う場合は、`WebUI_NDI` と `NDIIOPlugin` の両方を有効化してください。
- NDI 側の受信先は `WebUINDIComponent` の `TargetNDIMediaReceiver` で指定します。
- `WebUINDIComponent` の `SelectedNDISource` に表示したい Source 名を入れると、`UNDIMediaReceiver` の `ConnectionSetting` に反映されます。
- 初回接続時は `Initialize(ConnectionInfo, Standalone)`、既存接続がある場合は `ChangeConnection(ConnectionInfo)` を呼びます。
- `SetTargetNDIMediaReceiver(UNDIMediaReceiver)` を使うと、Blueprint から受信先を差し替えられます。
- 画像が表示されない場合は、`SourceTexture` が正しく設定されているか、`WebUIId` が一致しているかを確認してください。

## Property を WebUI に出す方法

1. 対象の `UPROPERTY` 名を `00_` のような形式にします。
2. `WUI00_` のような形式も互換として使えます。
3. 数字は並び順に使われます。
4. 表示名はプレフィックスを除いた部分になります。
5. 対応型の値だけが WebUI に出ます。
6. 数値型は `UIMin/UIMax` や `ClampMin/ClampMax` があればスライダーとして表示されます。
   Editor では `Sync WebUI Presentation For Blueprint` で保存済み presentation に同期できます。Packaged build では保存済み data を使います。
7. `enum` は選択肢のドロップダウン、`Vector` / `Rotator` は成分ごとの数値入力です。
8. `Color` はカラーピッカー、`LinearColor` はカラーピッカー＋RGBA 数値入力で表示されます。

### 対応型

`WebUIRuntime` が現在対応している `UPROPERTY` の型は次のとおりです。

- `bool`
- `int32`
- `float` / `double`
- `FString` / `FName` / `FText`
- `enum`
- `FVector`
- `FRotator`
- `FColor`
- `FLinearColor`

JSON での入力例:

```json
{
  "bool": true,
  "int32": 42,
  "float": 1.5,
  "string": "hello",
  "enum": "ValueName",
  "vector": { "x": 1, "y": 2, "z": 3 },
  "rotator": { "pitch": 10, "yaw": 20, "roll": 30 },
  "color": { "r": 1, "g": 0.5, "b": 0, "a": 1 },
  "linearColor": { "r": 0.2, "g": 0.4, "b": 0.6, "a": 1 }
}
```

`TArray` / `TMap` / `TSet` / 任意の `UStruct` / オブジェクト参照は、この実装では対象外です。

Blueprint 側の例:

- `00_Brightness`
- `01_Tint`

WebUI 上で値を変更すると、UE 側の変数が更新され、その後 `OnWebUI...Changed` 系イベントが呼ばれます。

## Button の作り方

1. `WebUIComponentBase` か派生 Component で `RegisterWebUIButton("00_ButtonName")` を呼びます。
2. `WUI00_` のような形式も互換として扱えます。
3. 数字は並び順に使われ、表示時は取り除かれます。
4. `WebUIHostActor` から呼んだ場合も、内部の `WebUIHostComponent` のボタンとして扱われます。
5. `OnWebUIButtonClicked(ButtonId)` を実装して押下処理を書きます。

```cpp
RegisterWebUIButton(TEXT("00_Apply"));
RegisterWebUIButton(TEXT("01_Reset"));
```

Button は Property と同じく WebUI に表示され、クリックで UE 側のイベントに戻ります。

## UE 内 Widget 表示

`UWebUIRuntimeBrowserWidget` を使うと、既存の WebUI を `WebBrowserWidget` 経由で UE 内の UMG として表示できます。

### 配置方法

1. Widget Blueprint を作成し、親クラスに `WebUIRuntimeBrowserWidget` を指定します。
2. そのまま配置しても動作します。必要なら BP 側で `WebBrowser` を配置して見た目を調整できます。
3. `bAutoLoadOnConstruct=true` の場合、Construct 後に自動で `LoadWebUI()` します。
4. 手動で再読込したい場合は `ReloadWebUI()` を呼びます。

### 主な Blueprint API

- `LoadWebUI()`
- `ReloadWebUI()`
- `SetWebUIId(FString InWebUIId)`
- `SetUseEmbedMode(bool bInUseEmbedMode)`
- `SetOverrideURL(FString InOverrideURL)`
- `GetWebUIURL()`

### 主なプロパティ

- `bAutoLoadOnConstruct`
- `bUseEmbedMode`
- `bSupportsTransparency`
- `WebUIId`
- `AdditionalQueryString`
- `OverrideURL`
- `bPreferLocalhost`

### URL 生成

- `OverrideURL` が空でない場合はそれを優先します。
- 通常は `http://127.0.0.1:{Port}/webui/` を使います。
- `bUseEmbedMode=true` の場合、`embed=1` を付けます。
- `WebUIId` がある場合、`webuiId={WebUIId}` を付けます。
- `AdditionalQueryString` がある場合、そのまま追加します。

URL 例:

- `http://127.0.0.1:8080/webui/?embed=1`
- `http://127.0.0.1:8080/webui/?embed=1&webuiId=LightController`
- `http://127.0.0.1:8080/webui/?embed=1&webuiId=LightController&compact=1`

### `?embed=1` の意味

- UE 内埋め込み表示向けの軽量レイアウトに切り替えます。
- `html` / `body` の余白を抑え、背景を透明寄りにします。
- 外部ブラウザ向けの大きなヘッダーや余白を非表示にします。
- Property UI と Button UI はそのまま維持します。

### 透明背景

- `bSupportsTransparency` を有効にすると、`WebBrowserWidget` 側で透過表示を試みます。
- `embed=1` 時は HTML 側も透明背景寄りにします。
- ただし、プラットフォームやレンダラー設定によっては完全な透過にならない場合があります。

### サーバー起動順

- Widget の `NativeConstruct` 時点で HTTPServer が未起動でも、クラッシュしないようにしています。
- 未起動時はログを出し、数回だけ再試行します。
- 後から `ReloadWebUI()` を呼べば再接続できます。

### 外部ブラウザとの使い分け

- 外部ブラウザ向けの通常表示はそのまま残っています。
- `embed=1` は UMG 内表示専用の追加モードです。
- `webuiId` クエリを付けると、初期表示タブを指定できます。

## `WebUI_NDI` について

- `WebUI_NDI` は `NDIIOPlugin` を前提にします。
- `NDIIOPlugin` はこのリポジトリでは管理しません。
- 運用時は Unreal Engine プロジェクトの `Plugins/NDIIO` に配置してください。
- 公式 SDK は Project `Plugins/` と Engine `Plugins/` の両方に入れられますが、このリポジトリでは Project `Plugins/` 前提で扱います。
- 公式 SDK はこちらです: [NDI Unreal Engine SDK](https://ndi.video/for-developers/ndi-unreal-engine-sdk/)
- このリポジトリでは UE 5.7 向けの `NDIIOPlugin` を想定しています。
- `WebUI_NDI` を有効にすると、`NDIMediaTexture2D` を `WebUIImageComponent` で表示できます。
- `WebUINDIComponent` では `NDIMediaReceiver` を直接持たせて NDI Source を切り替えます。`TargetNDIMediaReceiver` が未設定なら runtime で transient な `NDIMediaReceiver` と `NDIMediaTexture2D` を自動生成します。`On WebUI NDI Source Selected` は Details の `Events` から受け取れます。`NDIReceiverComponent` は不要です。
- 5.7 以外の Unreal Engine では、そのままでは動作しない可能性があります。

## 注意事項

- `Web Browser Widget` プラグインが無効だと UE 内表示は利用できません。
- Shipping ビルドや各プラットフォームでの WebBrowserWidget の挙動は、別途検証が必要です。
- `Allow Remote Access` を有効にすると、LAN 内の端末から操作 API にアクセスできます。運用環境ではアクセス制御に注意してください。
- この変更では `WebUI_NDI` の既存挙動は変更していません。
- `WebUIRuntimeEditor` の `Sync WebUI Presentation For Blueprint` で、Blueprint や C++ `UPROPERTY` の `DisplayName` / `ToolTip` / `UIMin` / `UIMax` / `ClampMin` / `ClampMax` を保存済み presentation に同期できます。
- エディタ上部ツールバーの `WebUI Sync All` で、プロジェクト内の Blueprint をまとめて同期できます。
