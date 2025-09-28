#pragma once
#include <Siv3D.hpp>

namespace Pad {

	struct Mapping {
		int  lx = 0;
		int  ly = 1;
		bool invertLy = false;

		// face buttons (×〇□△) の buttons[] インデックス
		int cross = 1;           // ×
		int circle = 2;          // 〇
		int square = 0;          // □
		int triangle = 3;        // △

		// D-Pad（十字キー）
		int dpadUp = 14;      // ↑
		int dpadRight = 15;      // →
		int dpadDown = 16;      // ↓
		int dpadLeft = 17;      // ←
	};

	inline Mapping DefaultMapping() {
		Mapping m;
		m.lx = 0; m.ly = 1; m.invertLy = false;
		m.cross = 1; m.circle = 2; m.square = 0; m.triangle = 3;
		m.dpadUp = 14; m.dpadRight = 15; m.dpadDown = 16; m.dpadLeft = 17;
		return m;
	}

	inline Mapping DualShock4Mapping(const s3d::GamepadInfo& /*info*/) {
		// DS4 でも上記の既定で動く想定。必要ならここで個別調整
		return DefaultMapping();
	}

	// 4方向の離散方向（斜めなし）
	enum class Dir4 { None, Left, Right, Up, Down };

	struct PS4Pad {
		size_t  index = 0;
		double  dead = 0.25;
		Mapping map = DefaultMapping();

		inline PS4Pad(size_t idx = 0, double dz = 0.25) noexcept
			: index(idx), dead(dz) {
			if (const auto& gp = s3d::Gamepad(index)) {
				const auto& info = gp.getInfo();
				if (info.vendorID == 0x054c) {
					map = DualShock4Mapping(info);
				}
			}
		}

		inline bool connected() const noexcept {
			return s3d::Gamepad(index).isConnected();
		}


		inline double lx() const noexcept {
			const auto& a = s3d::Gamepad(index).axes;
			if (map.lx < a.size()) {
				double v = a[map.lx];
				return (s3d::Math::Abs(v) < dead) ? 0.0 : v;
			}
			return 0.0;
		}
		inline double ly() const noexcept {
			const auto& a = s3d::Gamepad(index).axes;
			if (map.ly < a.size()) {
				double v = a[map.ly];
				if (map.invertLy) v = -v;
				return (s3d::Math::Abs(v) < dead) ? 0.0 : v;
			}
			return 0.0;
		}

		// --- D-Pad（十字キー）
		inline bool dpadUpPressed()    const noexcept { return _btnPressed(map.dpadUp); }
		inline bool dpadRightPressed() const noexcept { return _btnPressed(map.dpadRight); }
		inline bool dpadDownPressed()  const noexcept { return _btnPressed(map.dpadDown); }
		inline bool dpadLeftPressed()  const noexcept { return _btnPressed(map.dpadLeft); }

		inline bool dpadUpDown()    const noexcept { return _btnDown(map.dpadUp); }
		inline bool dpadRightDown() const noexcept { return _btnDown(map.dpadRight); }
		inline bool dpadDownDown()  const noexcept { return _btnDown(map.dpadDown); }
		inline bool dpadLeftDown()  const noexcept { return _btnDown(map.dpadLeft); }

		// 移動系は D-Pad を使用 ===
		inline bool leftPressed()  const noexcept { return dpadLeftPressed(); }
		inline bool rightPressed() const noexcept { return dpadRightPressed(); }
		inline bool upPressed()    const noexcept { return dpadUpPressed(); }
		inline bool downPressed()  const noexcept { return dpadDownPressed(); }

		// 斜め無効の離散方向（D-Padなので元々斜めは無いが、優先順を定義）
		inline Dir4 dir4() const noexcept {
			if (dpadLeftPressed())  return Dir4::Left;
			if (dpadRightPressed()) return Dir4::Right;
			if (dpadUpPressed())    return Dir4::Up;
			if (dpadDownPressed())  return Dir4::Down;
			return Dir4::None;
		}
		inline bool digitalLeft()  const noexcept { return leftPressed(); }
		inline bool digitalRight() const noexcept { return rightPressed(); }
		inline bool digitalUp()    const noexcept { return upPressed(); }
		inline bool digitalDown()  const noexcept { return downPressed(); }

		// --- ×〇□△（そのまま）
		inline bool crossPressed()    const noexcept { return _btnPressed(map.cross); }
		inline bool circlePressed()   const noexcept { return _btnPressed(map.circle); }
		inline bool squarePressed()   const noexcept { return _btnPressed(map.square); }
		inline bool trianglePressed() const noexcept { return _btnPressed(map.triangle); }

		inline bool crossDown()    const noexcept { return _btnDown(map.cross); }
		inline bool circleDown()   const noexcept { return _btnDown(map.circle); }
		inline bool squareDown()   const noexcept { return _btnDown(map.square); }
		inline bool triangleDown() const noexcept { return _btnDown(map.triangle); }

		// 任意ボタン直接参照（D-Pad番号の差異がある環境向け）
		inline bool buttonPressed(int idx) const noexcept {
			const auto& btns = s3d::Gamepad(index).buttons;
			return (0 <= idx && idx < static_cast<int>(btns.size())) ? btns[idx].pressed() : false;
		}
		inline bool buttonDown(int idx) const noexcept {
			const auto& btns = s3d::Gamepad(index).buttons;
			return (0 <= idx && idx < static_cast<int>(btns.size())) ? btns[idx].down() : false;
		}

		// ×/〇/□/△ の簡易キャリブ
		inline bool captureFaceButton(const s3d::String& which) {
			const auto& gp = s3d::Gamepad(index);
			for (auto&& [i, b] : s3d::Indexed(gp.buttons)) {
				if (b.down()) {
					if (which == U"cross")         map.cross = static_cast<int>(i);
					else if (which == U"circle")   map.circle = static_cast<int>(i);
					else if (which == U"square")   map.square = static_cast<int>(i);
					else if (which == U"triangle") map.triangle = static_cast<int>(i);
					return true;
				}
			}
			return false;
		}

	private:
		inline bool _btnPressed(int i) const noexcept {
			const auto& btns = s3d::Gamepad(index).buttons;
			return (0 <= i && i < static_cast<int>(btns.size())) ? btns[i].pressed() : false;
		}
		inline bool _btnDown(int i) const noexcept {
			const auto& btns = s3d::Gamepad(index).buttons;
			return (0 <= i && i < static_cast<int>(btns.size())) ? btns[i].down() : false;
		}
	};

}
