#pragma once
#include <Siv3D.hpp>
#include "SceneBase.hpp"
#include "PlayerColor.hpp"  // 共通のPlayerColorヘッダーをインクルード

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

	// 描画メソッド
	void drawBackground() const;
	void drawTitle() const;
	void drawCharacters() const;
	void drawCharacter(const CharacterData& character, bool isSelected) const;
	void drawButtons() const;
	void drawSparkles() const;
	void drawInstructions() const;

	// ユーティリティ
	String getColorName(PlayerColor color) const;
	ColorF getColorTint(PlayerColor color) const;
	void createSparkleEffect(const Vec2& position);

	// 五角形アニメーション関連
	Vec2 getPentagonTraceMovement(double time) const;
	void drawPentagonTracePath() const;

	// 静的変数
	static PlayerColor s_selectedPlayerColor;
};
