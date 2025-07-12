#pragma once
#include <Siv3D.hpp>
#include "SceneBase.hpp"

class TitleScene final : public SceneBase
{
private:
	// テクスチャ
	Texture m_backgroundTexture;
	Texture m_gameLogoTexture;
	Texture m_buttonTexture;  // ボタン用テクスチャ

	// フォント
	Font m_titleFont;
	Font m_messageFont;
	Font m_buttonFont;  // ボタン用フォント

	// ボタンの種類
	enum class ButtonAction
	{
		Start,
		Option,
		Credits,
		Exit
	};

	// ボタンデータ構造
	struct ButtonData
	{
		String text;
		RectF rect;
		ButtonAction action;
	};

	// ボタン関連
	Array<ButtonData> m_buttons;
	int m_selectedButton;      // 現在選択されているボタン
	double m_buttonHoverTimer; // ホバー効果用タイマー

	Optional<SceneType> m_nextScene;

public:
	TitleScene();
	~TitleScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;

private:
	// ボタン関連メソッド
	void setupButtons();
	void drawButtons() const;
	void executeButton(int buttonIndex);

	// 描画メソッド
	void drawAnimatedLogo() const;
	void drawBackgroundParticles() const;
	void drawControlInstructions() const;
};
