#include <Siv3D.hpp>
#include "Core/Game.hpp"

void Main()
{
	Profiler::EnableAssetCreationWarning(false);
	FontAsset::Register(U"Menu", 20, Typeface::Bold);
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
