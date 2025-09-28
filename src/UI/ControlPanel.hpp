#pragma once
#include <Siv3D.hpp>

class ControlPanel {
private:
	RectF m_rect;
	Font m_font;

	Texture m_iconCross;
	Texture m_iconCircle;
	Texture m_iconDpadVertical;
	Texture m_iconDpadHorizontal;

public:
	ControlPanel()
		: m_rect(Scene::Width() - 360, Scene::Height() - 160, 340, 140)
		, m_font(18, Typeface::Bold)
		, m_iconCross(U"UI/InputIcon/playstation_button_color_cross.png")
		, m_iconCircle(U"UI/InputIcon/playstation_button_color_circle.png")
		, m_iconDpadVertical(U"UI/InputIcon/playstation_dpad_vertical.png")
		, m_iconDpadHorizontal(U"UI/InputIcon/playstation_dpad_horizontal.png")
	{
	}

	void draw(bool showBack = true) const {
		// 背景
		m_rect.draw(ColorF(0.0, 0.0, 0.0, 0.6));
		m_rect.drawFrame(2, ColorF(1.0, 1.0, 0.8, 0.8));

		const Vec2 base = m_rect.pos.movedBy(20, 20);

		// 決定
		m_iconCross.resized(24).draw(base);
		m_font(U"Enter / × : 決定").draw(base.movedBy(34, 2), Palette::White);

		// 選択
		m_iconDpadVertical.resized(24).draw(base.movedBy(0, 32));
		m_font(U"↑↓ : 選択").draw(base.movedBy(34, 34), Palette::White);

		// 横操作も提示したい場合
		m_iconDpadHorizontal.resized(24).draw(base.movedBy(150, 32));
		m_font(U"←→ でも可").draw(base.movedBy(184, 34), Palette::Gray);

		// 戻る（タイトル以外のみ）
		if (showBack) {
			m_iconCircle.resized(24).draw(base.movedBy(0, 64));
			m_font(U"Esc / ○ : 戻る").draw(base.movedBy(34, 66), Palette::White);
		}
	}
};
