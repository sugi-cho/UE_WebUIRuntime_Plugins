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
3. `WebUIRuntime` を持つ Actor を配置して Web UI を起動します。

## `WebUI_NDI` について

- `WebUI_NDI` は `NDIIO` Plugin を前提にします。
- `NDIIO` は NDI 公式の Unreal Engine SDK から別途インストールしてください。
- このリポジトリの `NDIIO` は Unreal Engine 5.7 で使うためにカスタムしたものです。
- 5.7 以外の Unreal Engine では、そのままでは動作しない可能性があります。

## 注意

- `UE_WebUIRuntime` 本体は Git 管理対象外です。
- `NDIIO` は移植確認用で、最終的には Unreal Engine 側の `Engine/Plugins` に置く想定です。

