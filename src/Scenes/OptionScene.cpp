#include "OptionScene.hpp"
#include "../Sound/SoundManager.hpp"

OptionScene::OptionScene()
	: m_selectedItem(0)
	, m_isDraggingSlider(false)
	, m_draggingSliderIndex(-1)
	, m_nextScene(none)
{
}

void OptionScene::init()
{
	// テクスチャの読み込み
	m_backgroundTexture = Texture(U"Sprites/Backgrounds/background_fade_mushrooms.png");
	m_sliderBarTexture = Texture(U"UI/PNG/Yellow/slide_horizontal_color.png");
	m_sliderHandleTexture = Texture(U"UI/PNG/Yellow/slide_hangle.png");
	m_buttonTexture = Texture(U"UI/PNG/Yellow/button_rectangle_depth_gradient.png");

	// フォントの初期化
	m_titleFont = Font(36, Typeface::Bold);
	m_labelFont = Font(20);
	m_buttonFont = Font(20, Typeface::Bold);

	// SoundManagerから現在の設定を取得
	SoundManager& soundManager = SoundManager::GetInstance();

	// タイトルBGMが再生されていない場合は開始
	if (!soundManager.isBGMPlaying(SoundManager::SoundType::BGM_TITLE))
	{
		soundManager.playBGM(SoundManager::SoundType::BGM_TITLE);
	}

	// UI要素の設定
	setupPanel();
	setupSliders();
	setupButtons();

	// 初期状態
	m_selectedItem = 0;
	m_isDraggingSlider = false;
	m_draggingSliderIndex = -1;
	m_nextScene = none;
}

void OptionScene::update()
{
	if (!m_isDraggingSlider)
	{
		updateKeyboardInput();
	}

	updateMouseInput();
	updateSliderDrag();
}

void OptionScene::draw() const
{
	drawBackground();
	drawPanel();
	drawTitle();
	drawSliders();
	drawButtons();

	// 操作説明
	const String instructions = U"↑↓: Select  ←→: Adjust  ENTER: Confirm  ESC: Back";
	m_labelFont(instructions).draw(20, Scene::Height() - 30, ColorF(0.8, 0.8, 0.8));
}

Optional<SceneType> OptionScene::getNextScene() const
{
	return m_nextScene;
}

void OptionScene::cleanup()
{
	// 設定を保存（メモリ内のみ）
	applySettings();
	// BGMは停止しない（タイトルBGMを継続）
}

void OptionScene::setupPanel()
{
	const double panelWidth = 600.0;
	const double panelHeight = 500.0;
	m_panelRect = RectF(
		Scene::Center().x - panelWidth / 2,
		Scene::Center().y - panelHeight / 2,
		panelWidth,
		panelHeight
	);
}

void OptionScene::setupSliders()
{
	m_sliders.clear();

	SoundManager& soundManager = SoundManager::GetInstance();

	const double sliderWidth = 300.0;
	const double sliderHeight = 20.0;
	const double startY = m_panelRect.y + 80;
	const double spacing = 80.0;

	// マスター音量
	SliderData masterVolume;
	masterVolume.label = U"Master Volume";
	masterVolume.barRect = RectF(
		m_panelRect.center().x - sliderWidth / 2,
		startY,
		sliderWidth,
		sliderHeight
	);
	masterVolume.value = soundManager.getMasterVolume();
	masterVolume.minValue = 0.0;
	masterVolume.maxValue = 1.0;
	masterVolume.isDragging = false;
	masterVolume.handlePos = Vec2(
		masterVolume.barRect.x + masterVolume.barRect.w * masterVolume.value,
		masterVolume.barRect.center().y
	);
	m_sliders.push_back(masterVolume);

	// BGM音量
	SliderData bgmVolume;
	bgmVolume.label = U"BGM Volume";
	bgmVolume.barRect = RectF(
		m_panelRect.center().x - sliderWidth / 2,
		startY + spacing,
		sliderWidth,
		sliderHeight
	);
	bgmVolume.value = soundManager.getBGMVolume();
	bgmVolume.minValue = 0.0;
	bgmVolume.maxValue = 1.0;
	bgmVolume.isDragging = false;
	bgmVolume.handlePos = Vec2(
		bgmVolume.barRect.x + bgmVolume.barRect.w * bgmVolume.value,
		bgmVolume.barRect.center().y
	);
	m_sliders.push_back(bgmVolume);

	// SE音量
	SliderData seVolume;
	seVolume.label = U"SE Volume";
	seVolume.barRect = RectF(
		m_panelRect.center().x - sliderWidth / 2,
		startY + spacing * 2,
		sliderWidth,
		sliderHeight
	);
	seVolume.value = soundManager.getSEVolume();
	seVolume.minValue = 0.0;
	seVolume.maxValue = 1.0;
	seVolume.isDragging = false;
	seVolume.handlePos = Vec2(
		seVolume.barRect.x + seVolume.barRect.w * seVolume.value,
		seVolume.barRect.center().y
	);
	m_sliders.push_back(seVolume);
}

void OptionScene::setupButtons()
{
	m_buttons.clear();

	const double buttonWidth = 120.0;
	const double buttonHeight = 50.0;
	const double buttonSpacing = 20.0;
	const double startY = m_panelRect.y + m_panelRect.h - 80;

	// 3つのボタンの総幅を計算
	const double totalWidth = buttonWidth * 3 + buttonSpacing * 2;
	const double startX = m_panelRect.center().x - totalWidth / 2;

	// Applyボタン
	ButtonData applyButton;
	applyButton.text = U"APPLY";
	applyButton.rect = RectF(
		startX,
		startY,
		buttonWidth,
		buttonHeight
	);
	applyButton.action = ButtonData::Action::Apply;
	m_buttons.push_back(applyButton);

	// Resetボタン
	ButtonData resetButton;
	resetButton.text = U"RESET";
	resetButton.rect = RectF(
		startX + buttonWidth + buttonSpacing,
		startY,
		buttonWidth,
		buttonHeight
	);
	resetButton.action = ButtonData::Action::Reset;
	m_buttons.push_back(resetButton);

	// Backボタン
	ButtonData backButton;
	backButton.text = U"BACK";
	backButton.rect = RectF(
		startX + (buttonWidth + buttonSpacing) * 2,
		startY,
		buttonWidth,
		buttonHeight
	);
	backButton.action = ButtonData::Action::Back;
	m_buttons.push_back(backButton);
}

void OptionScene::updateKeyboardInput()
{
	// アイテム選択
	if (KeyUp.down() || KeyW.down())
	{
		m_selectedItem = (m_selectedItem - 1 + getTotalItemCount()) % getTotalItemCount();
		SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_SELECT);
	}
	if (KeyDown.down() || KeyS.down())
	{
		m_selectedItem = (m_selectedItem + 1) % getTotalItemCount();
		SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_SELECT);
	}

	// スライダー調整
	if (isSliderIndex(m_selectedItem))
	{
		const int sliderIndex = getSliderIndex(m_selectedItem);
		SliderData& slider = m_sliders[sliderIndex];

		if (KeyLeft.down() || KeyA.down())
		{
			slider.value = Math::Clamp(slider.value - 0.1, 0.0, 1.0);
			slider.handlePos.x = slider.barRect.x + slider.barRect.w * slider.value;
			updateSliderValue(sliderIndex, slider.value);
		}
		if (KeyRight.down() || KeyD.down())
		{
			slider.value = Math::Clamp(slider.value + 0.1, 0.0, 1.0);
			slider.handlePos.x = slider.barRect.x + slider.barRect.w * slider.value;
			updateSliderValue(sliderIndex, slider.value);
		}
	}

	// ボタン実行
	if (KeyEnter.down() || KeySpace.down())
	{
		if (!isSliderIndex(m_selectedItem))
		{
			const int buttonIndex = getButtonIndex(m_selectedItem);
			executeButton(buttonIndex);
		}
	}

	// ESCで戻る
	if (KeyEscape.down())
	{
		m_nextScene = SceneType::Title;
	}
}

void OptionScene::updateMouseInput()
{
	const Vec2 mousePos = Cursor::Pos();

	// マウスホバー検出
	for (size_t i = 0; i < m_sliders.size(); ++i)
	{
		if (m_sliders[i].barRect.contains(mousePos))
		{
			m_selectedItem = static_cast<int>(i);
			break;
		}
	}

	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		if (m_buttons[i].rect.contains(mousePos))
		{
			m_selectedItem = static_cast<int>(m_sliders.size() + i);
			break;
		}
	}

	// スライダードラッグ開始
	if (MouseL.down())
	{
		for (size_t i = 0; i < m_sliders.size(); ++i)
		{
			if (m_sliders[i].barRect.contains(mousePos))
			{
				m_isDraggingSlider = true;
				m_draggingSliderIndex = static_cast<int>(i);
				m_dragStartPos = mousePos;
				break;
			}
		}

		// ボタンクリック
		for (size_t i = 0; i < m_buttons.size(); ++i)
		{
			if (m_buttons[i].rect.contains(mousePos))
			{
				executeButton(static_cast<int>(i));
				break;
			}
		}
	}

	// ドラッグ終了
	if (MouseL.up())
	{
		m_isDraggingSlider = false;
		m_draggingSliderIndex = -1;
	}
}

void OptionScene::updateSliderDrag()
{
	if (!m_isDraggingSlider || m_draggingSliderIndex < 0)
		return;

	SliderData& slider = m_sliders[m_draggingSliderIndex];
	const Vec2 mousePos = Cursor::Pos();

	// ハンドル位置を更新
	const double relativeX = mousePos.x - slider.barRect.x;
	const double normalizedX = Math::Clamp(relativeX / slider.barRect.w, 0.0, 1.0);

	slider.value = normalizedX;
	slider.handlePos.x = slider.barRect.x + slider.barRect.w * slider.value;

	// リアルタイムでサウンド設定を更新
	updateSliderValue(m_draggingSliderIndex, slider.value);
}

void OptionScene::updateSliderValue(int sliderIndex, double value)
{
	SoundManager& soundManager = SoundManager::GetInstance();

	switch (sliderIndex)
	{
	case 0: // Master Volume
		soundManager.setMasterVolume(value);
		break;
	case 1: // BGM Volume
		soundManager.setBGMVolume(value);
		break;
	case 2: // SE Volume
		soundManager.setSEVolume(value);
		// テスト用にSEを再生
		soundManager.playSE(SoundManager::SoundType::SFX_SELECT);
		break;
	}
}

void OptionScene::drawBackground() const
{
	if (m_backgroundTexture)
	{
		m_backgroundTexture.resized(Scene::Size()).draw(ColorF(0.7, 0.7, 0.7));
	}
	else
	{
		Scene::Rect().draw(ColorF(0.2, 0.2, 0.3));
	}
}

void OptionScene::drawPanel() const
{
	// パネル背景
	m_panelRect.draw(ColorF(0.1, 0.1, 0.2, 0.9));
	m_panelRect.drawFrame(3.0, ColorF(0.8, 0.8, 0.6));

	// パネル内側の装飾
	const RectF innerRect = m_panelRect.stretched(-10);
	innerRect.drawFrame(1.0, ColorF(0.6, 0.6, 0.4, 0.5));
}

void OptionScene::drawTitle() const
{
	const String title = U"OPTIONS";
	const Vec2 titlePos = Vec2(m_panelRect.center().x, m_panelRect.y + 30);

	// タイトルの影
	m_titleFont(title).drawAt(titlePos + Vec2(2, 2), ColorF(0.0, 0.0, 0.0, 0.5));
	// タイトル本体
	m_titleFont(title).drawAt(titlePos, ColorF(1.0, 1.0, 0.8));
}

void OptionScene::drawSliders() const
{
	for (size_t i = 0; i < m_sliders.size(); ++i)
	{
		const bool isSelected = (m_selectedItem == static_cast<int>(i));
		drawSlider(m_sliders[i], isSelected);
	}
}

void OptionScene::drawSlider(const SliderData& slider, bool isSelected) const
{
	const double time = Scene::Time();

	// ラベル描画
	const Vec2 labelPos = Vec2(slider.barRect.x, slider.barRect.y - 30);
	m_labelFont(slider.label).draw(labelPos, ColorF(0.9, 0.9, 0.9));

	// 値表示
	const String valueText = U"{:.0f}%"_fmt(slider.value * 100);
	const Vec2 valuePos = Vec2(slider.barRect.x + slider.barRect.w + 20, slider.barRect.y - 5);
	m_labelFont(valueText).draw(valuePos, ColorF(1.0, 1.0, 0.8));

	// スライダーバー描画
	if (m_sliderBarTexture)
	{
		ColorF barColor = isSelected ? ColorF(1.0, 1.0, 0.8) : ColorF(0.8, 0.8, 0.8);
		m_sliderBarTexture.resized(slider.barRect.size).drawAt(slider.barRect.center(), barColor);
	}
	else
	{
		// フォールバック
		slider.barRect.draw(ColorF(0.3, 0.3, 0.3));
		slider.barRect.drawFrame(2.0, isSelected ? ColorF(1.0, 1.0, 0.6) : ColorF(0.6, 0.6, 0.6));
	}

	// ハンドル描画
	if (m_sliderHandleTexture)
	{
		ColorF handleColor = ColorF(1.0, 1.0, 1.0);
		if (isSelected)
		{
			const double glow = 0.8 + 0.2 * std::sin(time * 6.0);
			handleColor = ColorF(1.0, 1.0, glow);
		}

		const double handleSize = 24.0;
		m_sliderHandleTexture.resized(handleSize, handleSize).drawAt(slider.handlePos, handleColor);
	}
	else
	{
		// フォールバック
		const double handleRadius = 12.0;
		Circle(slider.handlePos, handleRadius).draw(isSelected ? ColorF(1.0, 1.0, 0.6) : ColorF(0.8, 0.8, 0.8));
		Circle(slider.handlePos, handleRadius).drawFrame(2.0, ColorF(0.2, 0.2, 0.2));
	}
}

void OptionScene::drawButtons() const
{
	const double time = Scene::Time();

	for (size_t i = 0; i < m_buttons.size(); ++i)
	{
		const ButtonData& button = m_buttons[i];
		const int itemIndex = static_cast<int>(m_sliders.size() + i);
		const bool isSelected = (m_selectedItem == itemIndex);

		// ボタン描画
		ColorF buttonColor = isSelected ? ColorF(1.0, 1.0, 0.8) : ColorF(0.8, 0.8, 0.8);

		if (m_buttonTexture)
		{
			m_buttonTexture.resized(button.rect.size).drawAt(button.rect.center(), buttonColor);
		}
		else
		{
			button.rect.draw(ColorF(0.3, 0.3, 0.3));
			button.rect.drawFrame(2.0, buttonColor);
		}

		// ボタンテキスト
		ColorF textColor = isSelected ? ColorF(0.1, 0.1, 0.1) : ColorF(0.9, 0.9, 0.9);
		m_buttonFont(button.text).drawAt(button.rect.center(), textColor);
	}
}

int OptionScene::getTotalItemCount() const
{
	return static_cast<int>(m_sliders.size() + m_buttons.size());
}

bool OptionScene::isSliderIndex(int index) const
{
	return index >= 0 && index < static_cast<int>(m_sliders.size());
}

int OptionScene::getSliderIndex(int itemIndex) const
{
	return itemIndex;
}

int OptionScene::getButtonIndex(int itemIndex) const
{
	return itemIndex - static_cast<int>(m_sliders.size());
}

void OptionScene::executeButton(int buttonIndex)
{
	if (buttonIndex < 0 || buttonIndex >= static_cast<int>(m_buttons.size()))
		return;

	const auto action = m_buttons[buttonIndex].action;

	SoundManager::GetInstance().playSE(SoundManager::SoundType::SFX_SELECT);

	switch (action)
	{
	case ButtonData::Action::Apply:
		applySettings();
		break;
	case ButtonData::Action::Reset:
		resetToDefaults();
		break;
	case ButtonData::Action::Back:
		m_nextScene = SceneType::Title;
		break;
	}
}

void OptionScene::resetToDefaults()
{
	SoundManager& soundManager = SoundManager::GetInstance();

	// デフォルト値に戻す
	soundManager.setMasterVolume(0.5);
	soundManager.setBGMVolume(0.7);
	soundManager.setSEVolume(0.8);

	// スライダー値を更新
	m_sliders[0].value = 0.5;  // Master Volume
	m_sliders[1].value = 0.7;  // BGM Volume
	m_sliders[2].value = 0.8;  // SE Volume

	// ハンドル位置更新
	for (auto& slider : m_sliders)
	{
		slider.handlePos.x = slider.barRect.x + slider.barRect.w * slider.value;
	}

}

void OptionScene::applySettings()
{
	// 設定適用（既にリアルタイムで適用されているため、確認メッセージのみ）
	Print << U"Settings applied!";
}
