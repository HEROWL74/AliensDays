#pragma once
#include <Siv3D.hpp>
#include "../Core/SceneBase.hpp"
#include "../Player/PlayerColor.hpp"  // 共通のPlayerColorヘッダーをインクルード

class CharacterSelectScene final : public SceneBase
{
private:
	// テクスチャ
	Texture m_backgroundTexture;
	Texture m_buttonTexture;
	Array<Texture> m_playerTextures;  // プレイヤーテクスチャ配列

	// フォント
	Font m_titleFont;
	Font m_labelFont;
	Font m_buttonFont;

	// キャラクター選択データ
	struct CharacterData
	{
		String name;
		PlayerColor color;
		Texture texture;
		RectF rect;
		Vec2 displayPos;
	};

	Array<CharacterData> m_characters;
	int m_selectedCharacter;
	double m_selectionTimer;

	// ボタン
	RectF m_selectButtonRect;
	RectF m_backButtonRect;
	bool m_selectButtonHovered;
	bool m_backButtonHovered;

	// アニメーション
	double m_animationTimer;
	Array<Vec2> m_sparklePositions;
	Array<double> m_sparkleTimers;

	// ステータスバーアニメーション用
	mutable Array<double> m_currentStatValues;  // 現在表示中の値（アニメーション用）
	mutable Array<double> m_targetStatValues;   // 目標値
	static constexpr double STAT_ANIMATION_SPEED = 8.0;  // アニメーション速度

	Optional<SceneType> m_nextScene;

public:
	CharacterSelectScene();
	~CharacterSelectScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;

	// 静的メソッド - 選択されたキャラクターの取得
	static PlayerColor getSelectedPlayerColor();
	static void setSelectedPlayerColor(PlayerColor color);

private:
	// 初期化メソッド
	void setupCharacters();
	void setupButtons();
	void loadPlayerTextures();

	// 更新メソッド
	void updateInput();
	void updateAnimations();
	void updateStatAnimations();      // ステータスバーアニメーション更新
	void updateTargetStatValues();    // 目標ステータス値更新

	// 描画メソッド
	void drawBackground() const;
	void drawTitle() const;
	void drawCharacters() const;
	void drawCharacter(const CharacterData& character, bool isSelected) const;
	void drawCharacterStats() const;  // 新しいメソッド：特性表示
	void drawButtons() const;
	void drawSparkles() const;
	void drawInstructions() const;

	// ユーティリティ
	String getColorName(PlayerColor color) const;
	ColorF getColorTint(PlayerColor color) const;
	void createSparkleEffect(const Vec2& position);

	// 特性表示用ヘルパーメソッド
	void drawStatBar(const String& label, int value, int maxValue, double centerX, double y, const ColorF& color) const;
	void drawAnimatedStatBar(const String& label, double normalizedValue, double centerX, double y, const ColorF& color) const;

	// 五角形アニメーション関連
	Vec2 getPentagonTraceMovement(double time) const;
	void drawPentagonTracePath() const;

	// 静的変数
	static PlayerColor s_selectedPlayerColor;
};
