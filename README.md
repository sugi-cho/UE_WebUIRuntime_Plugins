# WebUI Plugin Usage

このリポジトリは Unreal Engine プロジェクト本体ではなく、`Plugins/` 配下の WebUI 関連 Plugin のみを管理します。

## Plugin 構成

- `WebUIRuntime`
  - Core Runtime Plugin
  - Actor / Component の Web UI 化、schema API、property 変更、button event の基盤
- `WebUI_NDI`
  - `WebUIRuntime` の拡張 Plugin
  - NDI source picker など、NDI 連携用の追加機能

## 使い方

1. Unreal Engine プロジェクトの `Plugins/` にこのリポジトリの Plugin を配置します。
2. `UE_WebUIRuntime.uproject` で `WebUIRuntime` と必要なら `WebUI_NDI` を有効化します。
3. Project Settings の `Plugins > Web UI Runtime` で `Port` を設定します。
4. `WebUIHostComponent` を持つ Actor を配置して Web UI を起動します。

複数の Actor が `WebUIHostComponent` を持つ場合、Web UI は `WebUIId` ごとのタブで切り替えます。
タブ名は `WebUIId` を使います。

## `WebUIHostComponent` について

- Web UI の起動と集約を担当します。
- Actor ごとの識別子として `WebUIId` を持ちます。
- `WebUIId` が未設定なら Actor 名を使います。
- ポートは個別設定ではなく、Project Settings の 1 設定を使います。

## `WebUIComponent` について

- 実際の UI 要素は `WebUIComponentBase` か、その派生クラスに置きます。
- `WebUIComponentBase` はそのまま使ってもよいですし、独自の Component を作って継承しても使えます。
- たとえば NDI 用の `WebUINDIComponent` のように、用途ごとに派生 Component を追加できます。

## Property を WebUI に出す方法

1. 対象の `UPROPERTY` を `Category="WebUI"` にします。
2. もしくは `meta=(WebUI)` を付けます。
3. 対応型の値だけが WebUI に出ます。

```cpp
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="WebUI")
float Brightness;

UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(WebUI))
FLinearColor Tint;
```

WebUI 上で値を変更すると、UE 側の変数が更新され、その後 `OnWebUI...Changed` 系イベントが呼ばれます。

## Button の作り方

1. `WebUIComponentBase` か派生 Component で `RegisterWebUIButton("ButtonName")` を呼びます。
2. `OnWebUIButtonClicked(ButtonId)` を実装して押下処理を書きます。

```cpp
RegisterWebUIButton(TEXT("Apply"));
RegisterWebUIButton(TEXT("Reset"));
```

Button は Property と同じく WebUI に表示され、クリックで UE 側のイベントに戻ります。

## `WebUI_NDI` について

- `WebUI_NDI` は `NDIIOPlugin` を前提にします。
- `NDIIOPlugin` はこのリポジトリでは管理しません。
- 運用時は Unreal Engine プロジェクトの `Plugins/NDIIO` に配置してください。
- 公式 SDK は Project `Plugins/` と Engine `Plugins/` の両方に入れられますが、このリポジトリでは Project `Plugins/` 前提で扱います。
- 公式 SDK はこちらです: [NDI Unreal Engine SDK](https://ndi.video/for-developers/ndi-unreal-engine-sdk/)
- このリポジトリでは UE 5.7 向けの `NDIIOPlugin` を想定しています。
- 5.7 以外の Unreal Engine では、そのままでは動作しない可能性があります。
