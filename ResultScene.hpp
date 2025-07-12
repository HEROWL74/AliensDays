#pragma once
#include <Siv3D.hpp>
#include "SceneBase.hpp"
#include "Stage.hpp"
#include "PlayerColor.hpp"

class ResultScene final : public SceneBase
{
private:
	// テクスチャ
	Texture m_backgroundTexture;
	Texture m_buttonTexture;
	Texture m_starFilledTexture;
	Texture m_starOutlineTexture;
	Texture m_coinTexture;

	// フォント
	Font m_titleFont;
	Font m_headerFont;
	Font m_dataFont;
	Font m_buttonFont;

	// リザルトデータ
	struct ResultData
	{
		StageNumber clearedStage;
		PlayerColor playerColor;
		int collectedStars;
		int totalStars;
		int collectedCoins;
		double clearTime;
		String stageName;
		String playerColorName;
	};

	ResultData m_resultData;

	// ボタン関連
	enum class ButtonAction
	{
		NextStage,
		Retry,
		BackToTitle
	};

	struct ButtonData
	{
		String text;
		RectF rect;
		ButtonAction action;
		bool enabled;
	};

	Array<ButtonData> m_buttons;
	int m_selectedButton;
	bool m_buttonHovered;

	// アニメーション
	double m_animationTimer;
	double m_starAnimationDelay;
	bool m_showStarAnimation;

	// パネル設定
	RectF m_panelRect;

	Optional<SceneType> m_nextScene;

public:
	ResultScene();
	~ResultScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;

	// リザルトデータ設定
	void setResultData(StageNumber stage, PlayerColor playerColor,
					   int stars, int totalStars, int coins, double time);

private:
	// 初期化メソッド
	void setupPanel();
	void setupButtons();
	void loadTextures();

	// 更新メソッド
	void updateInput();
	void updateAnimations();

	// 描画メソッド
	void drawBackground() const;
	void drawPanel() const;
	void drawTitle() const;
	void drawStageInfo() const;
	void drawStarResult() const;
	void drawGameStats() const;
	void drawButtons() const;
	void drawParticleEffects() const;

	// ユーティリティ
	void executeButton(int buttonIndex);
	String getPlayerColorName(PlayerColor color) const;
	String formatTime(double seconds) const;
	ColorF getStarColor(int starIndex) const;
	String getStarRating() const;
};
