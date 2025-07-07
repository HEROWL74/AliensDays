#include <Siv3D.hpp>
#include "Game.hpp"

void Main()
{
	// ゲームインスタンスの作成
	Game game;

	// ゲームの初期化
	if (!game.init())
	{
		Print << U"Failed to initialize game";
		return;
	}

	// ゲームの実行
	game.run();

	// ゲームの終了処理
	game.shutdown();
}
