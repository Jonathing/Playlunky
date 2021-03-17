#pragma once

#include <Windows.h>
#include <memory>

class PlaylunkySettings;

class Playlunky {
public:
	static Playlunky& Get();
	
	static void Create(HMODULE game_module);
	static void Destroy();

	void Init();
	void PostGameInit();

	const PlaylunkySettings& GetSettings() const;

private:
	Playlunky(HMODULE game_module);
	~Playlunky();

	Playlunky() = delete;
	Playlunky(const Playlunky&) = delete;
	Playlunky(Playlunky&&) = delete;
	Playlunky& operator=(const Playlunky&) = delete;
	Playlunky& operator=(Playlunky&&) = delete;

	struct PlaylunkyImpl;
	friend struct PlaylunkyDeleter;
	std::unique_ptr<PlaylunkyImpl> mImpl;
};