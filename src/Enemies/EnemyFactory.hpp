//src/Enemies/EnemyFactory.hpp
#pragma once
#include <Siv3D.hpp>
#include <unordered_map>
#include <functional>
#include <memory>
//ファクトリーパターンでEnemyKey作成

class EnemyBase;

using EnemyCreateFn = std::function<std::unique_ptr<EnemyBase>(const Vec2& pos)>;

inline std::unordered_map<String, EnemyCreateFn>& enemyRegistry() {
	static std::unordered_map<String, EnemyCreateFn> reg;
	return reg;
}

struct EnemyAutoRegister {
	EnemyAutoRegister(String key, EnemyCreateFn fn) {
		enemyRegistry().emplace(std::move(key), std::move(fn));
	}
};

inline std::unique_ptr<EnemyBase> spawnEnemy(const String& key, const Vec2& pos) {
	if (auto it = enemyRegistry().find(key); it != enemyRegistry().end()) {
		return it->second(pos);
	}
	throw std::runtime_error("Unknown enemy key");
}
