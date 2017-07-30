#include <iostream>

#include <Heart.h>
#include <thread>

using namespace std::chrono_literals;

uint32_t Random() {
	thread_local std::mt19937 engine(std::random_device{}());
	thread_local std::uniform_int_distribution<uint32_t> dist;
	return dist(engine);
}

struct Player {
	Heart<Player> heart{ this };

	std::string name;
	int hp;

	Player(const std::string& name_) : name(name_) {
		heart.Push(&Player::ChangeHpRandomly).SetDelta(2s, 2s); // every 0-4 sec
		heart.Push(&Player::LogInfo).SetDelta(2500ms);

		hp = Random() % 10 + 30; // [30, 40)

		std::cout << "New player: " << name << " with hp " << hp << std::endl;
	}

	~Player() {
		std::cout << "Player " << name << " has died.." << std::endl;
	}

	void Heartbeat() {
		heart.Beat();
	}

	void ChangeHpRandomly() {
		int delta = Random() % 10 - 6; // [-6, 4)
		hp += delta;
		if (delta > 0) {
			std::cout << "Heal " << name << " by " << delta << std::endl;
		}
		else if (delta < 0) {
			std::cout << "Shoot " << name << " by " << -delta << std::endl;
		}
	}

	void LogInfo() {
		std::cout << "Player: " << name << ", hp: " << hp << std::endl;
	}
};

struct PlayerManager {
	Heart<PlayerManager> heart{ this };

	std::vector<std::unique_ptr<Player>> players;

	PlayerManager() {
		heart.Push(&PlayerManager::LogSomething).SetDelta(1s);
		heart.Push(&PlayerManager::InfoAfterStart).SetOnce().SetDelta(5s);
		heart.Push([](){
			std::cout << "Lambda, that called every 8-12 sec" << std::endl;
		}).SetDelta(10s).SetRandom(2s); // or .SetDelta(10s, 2s);
		heart.Push(&PlayerManager::CheckDeadPlayers).SetDelta(1ms);
	}

	void Heartbeat() {
		// beat all hearts
		heart.Beat();
		for (auto& player : players) {
			player->Heartbeat();
		}
	}

	void LogSomething() {
		std::cout << "Total players: " << players.size() << std::endl;
	}

	void InfoAfterStart() {
		std::cout << "Player manager run correctly!" << std::endl;
	}

	void CheckDeadPlayers() {
		players.erase(std::remove_if(players.begin(), players.end(), [](const auto& player){
			return player->hp <= 0;
		}), players.end());
	}
};

struct Game {
	PlayerManager manager;

	Game() {
		manager.players.emplace_back(std::make_unique<Player>("Nagibator"));
		manager.players.emplace_back(std::make_unique<Player>("Biogen911"));
		manager.players.emplace_back(std::make_unique<Player>("Smertig"));
	}

	void Run() {
		while (!manager.players.empty()) {
			manager.Heartbeat();
			std::this_thread::sleep_for(1ms);
		}
		std::cout << "End of game :(" << std::endl;
	}
};

int main() {
	Game game;
	game.Run();
	return 0;
}