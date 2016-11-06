#pragma once
namespace octet {
	enum direction { left, right, top, bottom };

	enum Loot { double_value, bonus, exit, fake_exit, none };

	struct character
	{
		int x;
		int y;
		//movement
		float distance_left_to_move;
		float speed;
		bool pointed_left = false;
		bool moving;
		direction moving_direction;
		float fading_point;
	};

	struct GameLevel
	{
		int steps = 0;
		int initial_steps;
		int reserve = 0;
		int id = 0;
	};
}
