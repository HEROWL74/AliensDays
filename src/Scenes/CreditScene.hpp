#pragma once
#include <Siv3D.hpp>
#include "../Core/SceneBase.hpp"

class CreditScene final : public SceneBase
{
private:
	// テクスチャ
	Texture m_backgroundTexture;
	Texture m_buttonTexture;

	// フォント
	Font m_titleFont;
	Font m_categoryFont;
	Font m_nameFont;
	Font m_buttonFont;

	// クレジット情報の構造
	struct CreditEntry
	{
		String category;
		Array<String> names;
		ColorF categoryColor;
		ColorF nameColor{};
	};

	// クレジットデータ
	Array<CreditEntry> m_credits;

	// スクロール関連
	double m_scrollOffset;
	double m_scrollSpeed;
	double m_totalHeight;
	bool m_autoScroll;

	// ボタン
	RectF m_backButtonRect;
	bool m_backButtonHovered;

	// アニメーション
	double m_animationTimer;

	Optional<SceneType> m_nextScene;

public:
	CreditScene();
	~CreditScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;

private:
	// 初期化メソッド
	void setupCredits();
	void setupButton();
	void calculateTotalHeight();

	// 更新メソッド
	void updateScroll();
	void updateInput();

	// 描画メソッド
	void drawBackground() const;
	void drawCredits() const;
	void drawBackButton() const;
	void drawParticleEffects() const;

	// ユーティリティ
	double getEntryHeight(const CreditEntry& entry) const;
	Vec2 getDrawPosition(double baseY) const;
	bool isVisible(double y, double height) const;
};
