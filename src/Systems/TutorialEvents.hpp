#pragma once
#include <Siv3D.hpp>
#include <list>

//チュートリアルで使うイベント
enum class TutorialEvent
{
   MoveLeftRight, //左右に移動した
   Jump,         //ジャンプした
   StompEnemy,   //敵を踏んだ（Flattened）
   FireballKill //ファイアボールで敵を倒した
};

//Subject
class TutorialSubject
{
public:
	//型エイリアス
	using Handler = std::function<void(TutorialEvent, const Vec2&)>;

	int subscribe(Handler h) {
		m_handlers.emplace_back(++m_lastId, std::move(h));
		return m_lastId;
	}
	void unsubscribe(int id) {
		m_handlers.remove_if([&](auto& p) {return p.first == id; });
	}
	void notify(TutorialEvent ev, const Vec2& pos = Vec2::Zero()) {
		for (auto& [_, h] : m_handlers)
			h(ev, pos);
	}

	static TutorialSubject& instance() { static TutorialSubject s; return s; }

private:
	int m_lastId = 0;
	std::list<std::pair<int, Handler>> m_handlers;
};

//送信用ヘルパー
inline void TutorialEmit(TutorialEvent ev, const Vec2& pos = Vec2::Zero()) {
	TutorialSubject::instance().notify(ev, pos);
}
