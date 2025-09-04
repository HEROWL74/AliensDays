# Aliens_Days

**横スクロールアクションゲーム**  
― バンダイナムコスタジオ杯 | Siv3Dゲームジャム2025 応募作品 ―  

---

## 🕹️ 概要

エイリアスたちの惑星での日々を想像して作り上げた緩くてちょっぴり危険なゲームです。

---

## ✨ 特徴（ゲーム性と技術ポイント）

- ステージ1～3、ミニゲーム、チュートリアル、オプション画面を搭載
- プレイヤー・敵・ギミックなどをJSONによる**データ駆動**で管理
- **スプライトアニメーション、エフェクト（Glow/Wave）**など演出強化
- **ゲームパッド対応（DS4）＋解像度調整・FPS表示機能（実装予定）**
- **マルチシーン構成：Title → Select → GameScene → Result**

---

## 🎮 操作方法

| 入力         | 内容             |
|--------------|------------------|
| ← →  A D     | 移動             |
| ↑ W          | ジャンプ         |
| F            | ファイアボール攻撃 |
| Click        | ボタン選択 |

---

## 💻 動作環境

- OS：Windows 10 / 11
- 開発言語：C++20 / Siv3D 0.6.14
- 実行方法：Visual Studio 2022 / `.exe` による実行

---

## 🖼️ スクリーンショット

### 🎬 タイトルシーン
![Title Scene](https://github.com/user-attachments/assets/a74af4ca-0b01-4d32-b540-c71ca20537a2)

ゲーム起動後のタイトルシーンでは、キャラクターが浮遊しながらロゴが表示され、ボタンにアニメーションが付与されています。  
背景・UI・エフェクトすべて自作素材で構成されており、**スタート・オプションの選択UIもマウス対応**です。

---

### 🌱 ステージ1（草原エリア）
![Stage 1 Gameplay](https://github.com/user-attachments/assets/e5ca6be7-5ea1-430a-beab-a30aee99743c)

---

## 🧠 使用技術・設計パターン

- **C++20**：`noexcept`, `[[nodiscard]]`, `explicit`, `const auto&` などモダン記法
- **Siv3D**：シーン管理・音声・UI・エフェクトなどを統合活用
- **設計パターン**：
  - Singleton：SceneManager, AudioManager
  - Factory：EnemyFactory, EffectFactory
  - Observer：イベント通知
  - Registry：ギミック動的登録

---

## 🧪 Git運用方針

- Git Flowスタイルを採用
  - `feature/〇〇`：機能追加
  - `fix/〇〇`：不具合修正
  - `refactor/〇〇`：内部構造整理
- `develop`：デバッグ・試遊用ブランチ
- `main`：Release版、タグ `v1.0.0` などでバージョン管理予定
- Pull Request運用 → 一人レビューも実施予定（細かくコメントを記載する）

---

## 👤 作者

- 名前：砥川展明（とがわ ひろあき）（高校2年生 / C++ゲーム開発者）
- GitHub：[https://github.com/yourname](https://github.com/HEROWL74)
- 使用エンジン：Siv3D / DirectX12 自作エンジン
- 活動記録：九州学生ゲーム大祭 2025 出展 / Bit Summit 13th 出展
