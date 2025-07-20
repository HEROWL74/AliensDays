#pragma once
#include <Siv3D.hpp>
#include "SceneBase.hpp"
#include "Stage.hpp"
#include "PlayerColor.hpp"

class GameOverScene final : public SceneBase
{
private:
	// テクスチャ
	Texture m_backgroundTexture;
	Texture m_buttonTexture;

	// フォント
	Font m_titleFont;
	Font m_messageFont;
	Font m_buttonFont;

	// ボタン関連
	enum class ButtonAction
	{
		Retry,
		CharacterSelect,
		Title
	};

	struct ButtonData
	{
		String text;
		RectF rect;
		ButtonAction action;
	};

	Array<ButtonData> m_buttons;
	int m_selectedButton;

	// アニメーション
	double m_animationTimer;
	double m_fadeAlpha;

	// ゲームオーバー情報
	StageNumber m_currentStage;
	PlayerColor m_playerColor;

	Optional<SceneType> m_nextScene;

	bool m_sePlayedOnce; //SE再生フラグを追加

public:
	GameOverScene();
	~GameOverScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;

	// ゲームオーバー情報設定
	void setGameOverData(StageNumber stage, PlayerColor playerColor);

private:
	// 初期化メソッド
	void setupButtons();
	void loadTextures();

	// 更新メソッド
	void updateInput();
	void updateAnimations();

	// 描画メソッド
	void drawBackground() const;
	void drawTitle() const;
	void drawButtons() const;
	void drawStageInfo() const;
	void drawParticleEffects() const;

	// ユーティリティ
	void executeButton(int buttonIndex);
	String getPlayerColorName(PlayerColor color) const;
};
