#pragma once
#include <Siv3D.hpp>
#include "../Core/SceneBase.hpp"

class OptionScene final : public SceneBase
{
private:
	// テクスチャ
	Texture m_backgroundTexture;
	Texture m_sliderBarTexture;    // スライダーバー
	Texture m_sliderHandleTexture; // スライダーハンドル
	Texture m_buttonTexture;       // ボタン用

	// フォント
	Font m_titleFont;
	Font m_labelFont;
	Font m_buttonFont;

	// スライダーデータ構造
	struct SliderData
	{
		String label;
		RectF barRect;
		Vec2 handlePos;
		double value;        // 0.0 - 1.0
		double minValue;
		double maxValue;
		bool isDragging;
	};

	// オプション設定
	Array<SliderData> m_sliders;

	// ボタンデータ
	struct ButtonData
	{
		String text;
		RectF rect;
		enum class Action { Back, Apply, Reset } action;
	};
	Array<ButtonData> m_buttons;

	// UI状態
	int m_selectedItem;          // 選択中のアイテム（スライダー + ボタン）
	bool m_isDraggingSlider;
	int m_draggingSliderIndex;
	Vec2 m_dragStartPos;

	// パネル設定
	RectF m_panelRect;

	Optional<SceneType> m_nextScene;

public:
	OptionScene();
	~OptionScene() override = default;

	void init() override;
	void update() override;
	void draw() const override;
	Optional<SceneType> getNextScene() const override;
	void cleanup() override;

private:
	// 初期化メソッド
	void setupSliders();
	void setupButtons();
	void setupPanel();

	// 更新メソッド
	void updateKeyboardInput();
	void updateMouseInput();
	void updateSliderDrag();
	void updateSliderValue(int sliderIndex, double value);  // 新規追加

	// 描画メソッド
	void drawBackground() const;
	void drawPanel() const;
	void drawSliders() const;
	void drawSlider(const SliderData& slider, bool isSelected) const;
	void drawButtons() const;
	void drawTitle() const;

	// ユーティリティ
	int getTotalItemCount() const;
	bool isSliderIndex(int index) const;
	int getSliderIndex(int itemIndex) const;
	int getButtonIndex(int itemIndex) const;
	void executeButton(int buttonIndex);
	void resetToDefaults();
	void applySettings();
};
