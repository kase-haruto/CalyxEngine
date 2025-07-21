#pragma once

class Player;

class PlayerInputHandler{
public:
	void Update(Player& player,float dt);

private:
	void HandleMove(Player& player);
	void HandleReticle(Player& player, float dt);
	void HandleShoot(Player& player);
};