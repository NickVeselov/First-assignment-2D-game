#include "labyrinth.h"

#pragma once
////////////////////////////////////////////////////////////////////////////////
//
// (C) Nikita Veselov 2016
//
// Introduction to the programming
// First assignment
// Goldsmiths University

namespace octet {

	class Camera
	{
		float x;
		float y;
		float z;
		mat4t cameraToWorld;
		vec3 initial_position;
		vec3 path_remaining;		
		vec3 basic_speed;
		vec3 current_speed;

		float min_distance;
		bool moving;
		int cell_size;

		float d1_shift(float path, float speed, bool horizontal)
		{
			float abs = std::abs(path);
			int cells_to_go = 1;
						
			if (horizontal)
			{
				//if camera has to pass multiple cells - accelerate
				if (abs > cell_size)
					cells_to_go = pow(2.f, abs / cell_size - 1);
			}
			//vertical movement
			else if (abs == 0)
				cells_to_go = 0;
			
			//increase speed
			float adjusted_speed = speed*cells_to_go;

			//if the distance remaining is lesser than one movement step
			if (abs < adjusted_speed)
				adjusted_speed = abs;

			//set movement direction
			if (path < 0)
				adjusted_speed *= -1;

			return (adjusted_speed);
		}

		void adjust_speed()
		{ 
			//	1/8 screen per 1 sec 0.2 - 0.6; 0.3; 0.75
			float current = (z - min_distance) / (initial_position.z() - min_distance), //4 - 14; 14/10 = 1.4;
				modifier = 18 * (current - 0.1);

			current_speed.x() = current_speed.y() = (2.f*z) / (modifier * 30);
		}
	public:		

		void init( float X, float Y, float Z, float Min_distance, float Vx, float Vy, float Vz, int Cell_size)
		{
			// set up the matrices with a camera 5 units from the origin
			cameraToWorld.loadIdentity();

			x = X; y = Y; z = Z;
			cameraToWorld.translate(x, y, z);

			initial_position = vec3(x, y, z);
			min_distance = Min_distance;
			path_remaining = vec3(0, 0, 0);
			basic_speed = vec3(Vx, Vy, Vz);
			current_speed = basic_speed;
			moving = false;
			cell_size = Cell_size;
		}

		void set_translation(float dx, float dy, float dz)
		{
			if (dx != 0 || dy != 0 || dz != 0)
			{
				moving = true;
				path_remaining += vec3(dx, dy, dz);
			}
		}

		void translate(vec3 &dv)
		{
			x += dv.x();
			y += dv.y();
			z += dv.z();
			cameraToWorld.translate(dv.x(), dv.y(), dv.z());

			adjust_speed();
		}

		bool is_moving()
		{
			return moving;
		}

		void move()
		{
			vec3 translation = vec3(0, 0, 0);
			if (path_remaining.x() != 0)
				translation.x() = d1_shift(path_remaining.x(), current_speed.x(), true);
			if (path_remaining.y() != 0)
				translation.y() = d1_shift(path_remaining.y(), current_speed.y(), true);
			if (path_remaining.z() != 0)
				translation.z() = d1_shift(path_remaining.z(), current_speed.z(), false);

			if (translation.x() != 0 || translation.y() != 0 || translation.z() != 0)
			{
				translate(translation);
				path_remaining -= translation;
			}
			else
				moving = false;
		}

		mat4t get_camera()
		{
			return cameraToWorld;
		}

		float get_z()
		{
			return z;
		}

		void go_to(float x1, float y1, float z1)
		{
			while (moving)
				move();
			translate(vec3(x1 - x , y1 - y, z1 - z));
		}
	};

	class sprite {
		// where is our sprite (overkill for a 2D game!)
		mat4t modelToWorld;

		// half the width of the sprite
		float halfWidth;

		// half the height of the sprite
		float halfHeight;

		// what texture is on our sprite
		int texture;

		// true if this sprite is enabled.
		bool enabled;



		//position
		float x;
		float y;
		
	public:

		float transparency;
		//static
		bool static_position;

		sprite() {
			texture = 0;
			enabled = true;
		}
		
		sprite(int _texture, float x0, float y0, float w, float h)
		{
			modelToWorld.loadIdentity();
			modelToWorld.translate(x0, y0, 0);
			halfWidth = w * 0.5f;
			halfHeight = h * 0.5f;
			texture = _texture;
			enabled = true;
			x = x0;
			y = y0;
		}

		void init(int _texture, float x0, float y0, float w, float h) {
			modelToWorld.loadIdentity();
			modelToWorld.translate(x0, y0, 0);
			halfWidth = w * 0.5f;
			halfHeight = h * 0.5f;
			texture = _texture;
			enabled = true;
			transparency = 1.0f;
			x = x0;
			y = y0;
			static_position = false;
		}

		void change_texture(int _texture)
		{
			texture = _texture;
		}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			// invisible sprite... used for gameplay.
			if (!texture) return;

			// build a projection matrix: model -> world -> camera -> projection
			// the projection space is the cube -1 <= x/w, y/w, z/w <= 1
			mat4t modelToProjection;
			if (static_position)
				modelToProjection = modelToWorld;
			else
				modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

			// set up opengl to draw textured triangles using sampler 0 (GL_TEXTURE0)
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture);

			// use "old skool" rendering
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			shader.render(modelToProjection, 0, transparency);

			// this is an array of the positions of the corners of the sprite in 3D
			// a straight "float" here means this array is being generated here at runtime.
			float vertices[] = {
				-halfWidth, -halfHeight, 0,
				halfWidth, -halfHeight, 0,
				halfWidth,  halfHeight, 0,
				-halfWidth,  halfHeight, 0,
			};

			// attribute_pos (=0) is position of each corner
			// each corner has 3 floats (x, y, z)
			// there is no gap between the 3 floats and hence the stride is 3*sizeof(float)
			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)vertices);
			glEnableVertexAttribArray(attribute_pos);

			// this is an array of the positions of the corners of the texture in 2D
			static const float uvs[] = {
				0,  0,
				1,  0,
				1,  1,
				0,  1,
			};

			// attribute_uv is position in the texture of each corner
			// each corner (vertex) has 2 floats (x, y)
			// there is no gap between the 2 floats and hence the stride is 2*sizeof(float)
			glVertexAttribPointer(attribute_uv, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)uvs);
			glEnableVertexAttribArray(attribute_uv);

			// finally, draw the sprite (4 vertices)
			glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		}

		// move the object
		void translate(float X, float Y, bool inverted = false) {
			modelToWorld.translate(X, Y, 0);
			if (!inverted)
				x += X;
			else
				x -= X;
			y += Y;
		}

		void resize(float w, float h)
		{
			halfWidth *= w;
			halfHeight *= h;
		}

		void rotateY180()
		{
			//modelToWorld.rotate(angle, x, y, z);
			modelToWorld.rotateY180();
		}

		// position the object relative to another.
		void set_relative(sprite &rhs, float x, float y) {
			modelToWorld = rhs.modelToWorld;
			modelToWorld.translate(x, y, 0);
		}

		bool is_above(const sprite &rhs, float margin) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];

			return
				(fabsf(dx) < halfWidth + margin)
				;
		}

		bool &is_enabled() {
			return enabled;
		}

		void move_to_the_cell(int i, int j, int cell_size)
		{
			vec2 destination = vec2(cell_size*j + cell_size/2.f, cell_size*i + cell_size/2.f);
			translate(destination.x() - x, destination.y() - y);
		}

		//get the position of the sprite
		vec2 get_position()
		{
			return vec2(x, y);
		}
	};

	class labyrinth_app : public octet::app {

		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera

		// shader to draw a textured triangle
		texture_shader texture_shader_;

		//labyrinth class (labyrinth.h)
		Labyrinth lab;
		//character data (position, loot, etc.)
		character character;
		//status bar with text
		GameLevel level;

		// big array of sprites
		sprite sprites[2000];

		//special sprites
		int current_sprite;
		int character_sprite;
		int game_over_sprite;
		int board_sprite;
		int exit_sprite;
		int level_complete_sprite;
		int scary_image_sprite;
		int static_sprites_number;

		// game state
		bool game_over;
		bool level_complete;

		// random number generator
		class random randomizer;

		// a texture for our text
		GLuint font_texture;

		// information for our text
		bitmap_font font;

		// sounds
		ALuint main_music_theme;
		unsigned cur_source;
		enum { num_sound_sources = 44 };
		ALuint sources[num_sound_sources];

		Camera camera;

		Loot steps_alteration;
		int steps_alteration_duration;
		int blinking_sprites;
		int scary_face_timeout;
		int previous_scary_face_time_interval;

		int bonus_value;

		std::vector<Cell> content_cells;
		
	//for the game in general
	#pragma region once-initialization
		
		float border_width = 1;
		float wall_width = 0.4f;
		float camera_init_pos = lab.absolute_size*0.4f;
		float camera_fin_pos = lab.absolute_size*0.2f;

		void game_initialization()
		{
			game_over = false;
			current_sprite = 0;

			GLuint LevelComplete = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/level complete.gif");
			level_complete_sprite = current_sprite;
			sprites[current_sprite++].init(LevelComplete, 0, 0, 2, 2);
			sprites[level_complete_sprite].static_position = true;

			GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/game over.gif");
			game_over_sprite = current_sprite;
			sprites[current_sprite++].init(GameOver, 0, 0, 2, 2);
			sprites[game_over_sprite].is_enabled() = false;
			sprites[game_over_sprite].static_position = true;

			GLuint Board = resource_dict::get_texture_handle(GL_RGBA, "#800080");
			board_sprite = current_sprite;
			sprites[current_sprite++].init(Board, 0, -0.8f, 1.8f, 0.2f);
			sprites[board_sprite].static_position = true;
			sprites[board_sprite].is_enabled() = true;

			GLuint character_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/character.gif");
			character_sprite = current_sprite;
			sprites[current_sprite++].init(character_texture, 0, 0,
				lab.cell_size - border_width - wall_width, lab.cell_size - border_width - wall_width);

			static_sprites_number = current_sprite;

			character.speed = lab.cell_size / 6.f;
		}

	#pragma endregion

	//for every level
	#pragma region initialization-loop 

		void draw_walls()
		{
			GLuint border = resource_dict::get_texture_handle(GL_RGB, "#FFFFFF");
			GLuint wall = resource_dict::get_texture_handle(GL_RGB, "#888888");
			GLuint void_space = resource_dict::get_texture_handle(GL_RGB, "#000000");

			//outer walls
			sprites[current_sprite++].init(border, lab.half_size, 0, lab.absolute_size + border_width, border_width);
			sprites[current_sprite++].init(border, lab.half_size, lab.absolute_size, lab.absolute_size + border_width, border_width);
			sprites[current_sprite++].init(border, 0, lab.half_size, border_width, lab.absolute_size + border_width);
			sprites[current_sprite++].init(border, lab.absolute_size, lab.half_size, border_width, lab.absolute_size + border_width);

			//inner walls
			for (int i = 0; i<lab.cells_number; i++)
				for (int j = 0; j < lab.cells_number; j++)
				{
					if ((lab.cells[i][j].left_wall) && (j != 0))
						sprites[current_sprite++].init(wall, j*lab.cell_size, i*lab.cell_size + lab.half_cell, wall_width, lab.cell_size + wall_width);

					if ((lab.cells[i][j].top_wall) && (i != lab.cells_number - 1))
						sprites[current_sprite++].init(wall, j*lab.cell_size + lab.half_cell, (i + 1)*lab.cell_size, lab.cell_size + wall_width, wall_width);

					if ((lab.cells[i][j].right_wall) && (j != lab.cells_number - 1))
						sprites[current_sprite++].init(wall, (j + 1)*lab.cell_size, i*lab.cell_size + lab.half_cell, wall_width, lab.cell_size + wall_width);

					if ((lab.cells[i][j].bottom_wall) && (i != 0))
						sprites[current_sprite++].init(wall, j*lab.cell_size + lab.half_cell, i*lab.cell_size, lab.cell_size + wall_width, wall_width);

					if (lab.cells[i][j].distance > 0)
						content_cells.push_back(lab.cells[i][j]);

				}


		}
		
		void set_variables()
		{
			character.x = lab.entrance_index;
			character.y = 0;

			level.initial_steps = level.steps += 0.7f*lab.path_length;
			level.initial_steps = level.steps += rand() % level.steps / 2.f;

			character.fading_point = level.steps / 10.f;
			sprites[character_sprite].transparency = 1;

			steps_alteration = none;
			steps_alteration_duration = 0;
			bonus_value = level.initial_steps / 2;
			scary_face_timeout = 0;
			scary_image_sprite = -1;
			previous_scary_face_time_interval = 10000;
		}

		void create_exits(int exits_number)
		{
			//exits
			GLuint exit_staircase = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/staircase.gif");
			exit_sprite = current_sprite;
			int true_exit = rand() % exits_number + 1;
			for (int i = 1; i <= exits_number; i++)
			{
				content_cells[content_cells.size() - i].sprite_index = current_sprite;

				sprites[current_sprite++].init(exit_staircase, content_cells[content_cells.size() - i].x*lab.cell_size + lab.half_cell,
					content_cells[content_cells.size() - i].y*lab.cell_size + lab.half_cell, 0.9f*lab.cell_size, 0.9f*lab.cell_size);
				//true or fake?
				if (i == true_exit)
					content_cells[content_cells.size() - i].loot = exit;
				else
					content_cells[content_cells.size() - i].loot = fake_exit;
			}
		}

		void create_pickups(int exits_number, int stop_index)
		{
			for (int i = content_cells.size() - exits_number - 1; i >= stop_index; i--)
			{
				GLuint soul_loot;
				int type = rand() % 2;
				content_cells[i].bouncing = true;
				content_cells[i].bounce_max_value = lab.cell_size / 20;
				int r = rand() % 2 * content_cells[i].bounce_max_value;
				content_cells[i].current_bounce_position = -content_cells[i].current_bounce_position + r;
				r = rand() % 2;
				if (r == 0)
					content_cells[i].direction = true;
				else
					content_cells[i].direction = false;

				switch (type)
				{
					//blue (add)
				case 0:
					soul_loot = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/blue soul.gif");
					content_cells[i].loot = bonus;
					break;
					//violet (x2)
				case 1:
					soul_loot = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/violet soul.gif");
					content_cells[i].loot = double_value;
					break;
				}
				content_cells[i].sprite_index = current_sprite;
				sprites[current_sprite++].init(soul_loot, content_cells[i].x*lab.cell_size + lab.half_cell, content_cells[i].y*lab.cell_size + lab.half_cell,
					lab.cell_size*0.7f, lab.cell_size*0.7f);
			}
		}

		void add_labyrinth_content()
		{
			std::sort(content_cells.begin(), content_cells.end());

			int exits_number = 3,
				pickups_number = 4;
			create_exits(3);
			create_pickups(4, std::max((int)content_cells.size() - (exits_number + pickups_number), 0));
		}

		void generate_new_level()
		{
			level.id++;
			sprites[level_complete_sprite].is_enabled() = false;
			if (character.pointed_left)
			{
				sprites[character_sprite].rotateY180();
				character.pointed_left = false;
			}

			for (int i = static_sprites_number; i <= current_sprite; i++)
				sprites[i].is_enabled() = false;
			current_sprite = static_sprites_number;
			if (1 == 1)
			{
				content_cells.clear();
				lab.construct_labyrinth();
				draw_walls();

				sprites[character_sprite].move_to_the_cell(0, lab.entrance_index, lab.cell_size);

				set_variables();

				add_labyrinth_content();

				//center camera on the character
				if (level.id == 2)
					level.id = 2;
				camera.go_to(sprites[character_sprite].get_position().x(), sprites[character_sprite].get_position().y(), camera_init_pos);

				level_complete = false;
				character.moving = false;
			}
		}

	#pragma endregion

	//for every frame in a loop
	#pragma region level loops

		void set_character_translation()
		{
			if (!character.moving)
			{
				int x = character.x,
					y = character.y;

				if (is_key_down(key_left)) {
					if ((!lab.cells[character.y][character.x].left_wall) && (character.x > 0))
					{
						if (!character.pointed_left)
						{
							sprites[character_sprite].rotateY180();
							character.pointed_left = true;
						}
						character.moving_direction = left;
						character.x--;
					}
				}
				else if ((is_key_down(key_right)) && (character.x < lab.cells_number)) {
					if (!lab.cells[character.y][character.x].right_wall)
					{
						if (character.pointed_left)
						{
							sprites[character_sprite].rotateY180();
							character.pointed_left = false;
						}
						character.moving_direction = right;
						character.x++;
					}
				}
				else if ((is_key_down(key_up)) && (character.y < lab.cells_number)) {
					if (!lab.cells[character.y][character.x].top_wall)
					{
						character.moving_direction = top;
						character.y++;
					}
				}
				else if ((is_key_down(key_down)) && (character.y > 0)) {
					if (!lab.cells[character.y][character.x].bottom_wall)
					{
						character.moving_direction = bottom;
						character.y--;
					}
				}

				//if movement occured
				if (character.x != x || character.y != y)
				{
					character.distance_left_to_move = lab.cell_size;
					camera.set_translation((character.x - x)*lab.cell_size, (character.y - y)*lab.cell_size, 0);
					level.steps--;
					character.moving = true;
					//check for looting/escaping
					check_collision();
				}
			}
		}
		
		// use the keyboard to move the character
		void move_character() {

			set_character_translation();

			if (character.moving)
			{
				vec2 pos = sprites[character_sprite].get_position();
				if (character.distance_left_to_move > 0)
				{
					float translation = std::min(character.speed, character.distance_left_to_move);
					switch (character.moving_direction)
					{
					case left:
						sprites[character_sprite].translate(+translation, 0,true);
						break;
					case right:
						sprites[character_sprite].translate(+translation, 0);
						break;
					case top:
						sprites[character_sprite].translate(0, +translation);
						break;
					case bottom:
						sprites[character_sprite].translate(0, -translation);
						break;
					default:
						break;
					}
					character.distance_left_to_move -= character.speed;
				}
				else
				{
					character.moving = false;
					character.distance_left_to_move = 0;
					pos = sprites[character_sprite].get_position();
				}
			}
		}

		//check collision
		void check_collision()
		{
			//show scary face sometimes for suspense
			int rand_scary_face = rand() % 100;
			if (rand_scary_face == 0)
				show_evil_face(3 + 2.f*level.initial_steps / level.steps);

			for (int i = 0; i < content_cells.size(); i++)
			{
				if (content_cells[i].x == character.x && content_cells[i].y == character.y)
				{
					switch (content_cells[i].loot)
					{
					case bonus:
						level.steps += bonus_value;
						handle_looting_event();
						sprites[content_cells[i].sprite_index].is_enabled() = false;
						steps_alteration = content_cells[i].loot;
						steps_alteration_duration = 100;
						content_cells[i].loot = none;
						break;
					case double_value:
						level.steps *= 2;
						handle_looting_event();
						sprites[content_cells[i].sprite_index].is_enabled() = false;
						steps_alteration = content_cells[i].loot;
						steps_alteration_duration = 100;
						content_cells[i].loot = none;
						break;
					case fake_exit:
						level.steps -= bonus_value;
						if (level.steps <= 0)
							game_over = true;
						handle_looting_event();
						sprites[content_cells[i].sprite_index].is_enabled() = false;
						steps_alteration = content_cells[i].loot;
						steps_alteration_duration = 100;
						content_cells[i].loot = none;
						show_evil_face(30);
						break;
					case exit:
						//	//cameraToWorld.translate(camera_path_remaining.x(), camera_path_remaining.y(), 0);
						//	//camera_position += camera_path_remaining;
						sprites[board_sprite].is_enabled() = false;
						level_complete = true;
						sprites[level_complete_sprite].is_enabled() = true;
						break;
					default:
						break;
					}
				}
				else if (content_cells[i].loot == fake_exit)
				{
					//component from the path made by character
					int min_value = level.initial_steps - level.steps;
					//random number
					int random_component = rand() % level.initial_steps;
					//if they combined divided by some constant > initial steps - show fake staircase for the few frames
					if (((random_component + min_value) / 1.6f > level.initial_steps) || (min_value < random_component && rand() % 100 == 0))
					{
						sprites[content_cells[i].sprite_index].change_texture(resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/evil ghost.gif"));
						blinking_sprites++;
						content_cells[i].blinking_time = 30;
					}
				}
			}
		}

		void handle_looting_event()
		{
			float current_pos = (float)level.steps/ level.initial_steps;
			if (current_pos > 1 )
				current_pos = 1;
			sprites[character_sprite].transparency = current_pos;
			//1 - 9; 3 = 0.25; 1 + 0.25*(8) = 1 + 0.75*8 = 7;
			camera.translate(vec3(0, 0, camera_fin_pos + current_pos*(camera_init_pos - camera_fin_pos) - camera.get_z()));
		}

		void show_evil_face(int duration)
		{
			if (previous_scary_face_time_interval > 5000)
			{
				previous_scary_face_time_interval = 0;
				if (scary_image_sprite == -1)
				{
					GLint evil_face = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/evil ghost.gif");
					scary_image_sprite = current_sprite;
					sprites[current_sprite++].init(evil_face, 0, 0, 1.8f, 1.8f);
					sprites[scary_image_sprite].static_position = true;
				}
				else
					sprites[scary_image_sprite].is_enabled() = true;
				scary_face_timeout = duration;
			}
		}

		void animate_pickups()
		{
			for (int i = 0; i < content_cells.size(); i++)
			{
				if (content_cells[i].bouncing)
				{
					float current = content_cells[i].current_bounce_position,
						max_pos = content_cells[i].bounce_max_value;
					if (content_cells[i].direction)
					{
						content_cells[i].current_bounce_position += max_pos / 10;
						sprites[content_cells[i].sprite_index].translate(0, +max_pos / 10);
					}
					else
					{
						content_cells[i].current_bounce_position -= max_pos / 10;
						sprites[content_cells[i].sprite_index].translate(0, -max_pos / 10);
					}

					//change direction
					if (abs(content_cells[i].current_bounce_position) >= content_cells[i].bounce_max_value)
					{
						if (content_cells[i].direction)
							content_cells[i].direction = false;
						else
							content_cells[i].direction = true;
					}
				}
			}
		}

		void animate_sprites()
		{
			if (blinking_sprites != 0)
			{
				for (int i = 0; i<content_cells.size(); i++)
				{
					if (content_cells[i].blinking_time != 0)
					{
						content_cells[i].blinking_time--;
						if (content_cells[i].blinking_time == 0)
						{
							sprites[content_cells[i].sprite_index].change_texture(resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/staircase.gif"));
							blinking_sprites--;
						}
					}
				}
			}
			if (scary_face_timeout != 0)
			{
				scary_face_timeout--;
				if (scary_face_timeout == 0)
					sprites[scary_image_sprite].is_enabled() = false;
			}
		}

	#pragma endregion

	//"invaders" useful functions
	#pragma region basic functions
		ALuint get_sound_source() { return sources[cur_source++ % 8]; }

		//draw text on the screen
		void draw_text(texture_shader &shader, float x, float y, float scale, const char *text) {
				mat4t modelToWorld;
				modelToWorld.loadIdentity();
				modelToWorld.translate(x, y, 0);
				modelToWorld.scale(scale, scale, 1);
				mat4t modelToProjection = modelToWorld;//mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

													   /*mat4t tmp;
													   glLoadIdentity();
													   glTranslatef(x, y, 0);
													   glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);
													   glScalef(scale, scale, 1);
													   glGetFloatv(GL_MODELVIEW_MATRIX, (float*)&tmp);*/

				enum { max_quads = 32 };
				bitmap_font::vertex vertices[max_quads * 4];
				uint32_t indices[max_quads * 6];
				aabb bb(vec3(0, 0, 0), vec3(256, 256, 0));

				unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text, 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, font_texture);

				shader.render(modelToProjection, 0, 1.0f);

				glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x);
				glEnableVertexAttribArray(attribute_pos);
				glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u);
				glEnableVertexAttribArray(attribute_uv);

				glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
			}
	#pragma endregion

	public:

		//ToDo: when your soul is low - all doubles become bonuses

		// this is called when we construct the class
		labyrinth_app(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
		}

		// this is called once OpenGL is initialized
		void app_init() {
			// set up the shader
			texture_shader_.init();

			//ToDo: the horizontal speed should be relative to camera distance
			camera.init(0, 0, camera_init_pos, camera_fin_pos,
				lab.cell_size / 120.f, lab.cell_size / 120.f, (camera_init_pos - camera_fin_pos) / 240.f, lab.cell_size);
			font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

			game_initialization();
			generate_new_level();

			// sounds
			main_music_theme = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/labyrinth/background.wav");
			cur_source = 0;
			alGenSources(num_sound_sources, sources);
			ALuint source = get_sound_source();
			alSourcei(source, AL_BUFFER, main_music_theme);
			alSourcePlay(source);
		}

		// this is called to draw the world
		void draw_world(int x, int y, int w, int h) {
			simulate();
			// set a viewport - includes whole window area
			glViewport(x, y, w, h);

			// clear the background to black
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// don't allow Z buffer depth testing (closer objects are always drawn in front of far ones)
			glDisable(GL_DEPTH_TEST);

			// allow alpha blend (transparency when alpha channel is 0)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// draw all the sprites
			for (int i = current_sprite; i >= 0; --i) {
				if (sprites[i].is_enabled())
					sprites[i].render(texture_shader_, camera.get_camera());
			}
			
			if (!game_over && scary_face_timeout == 0)
			{
				char steps[32];
				sprintf(steps, "Steps:%d", level.steps);

				//draw_text(texture_shader_, camera_position.x() - 0.55f*camera_initial_distance,
				//	camera_position.y() - 1.15f*camera_initial_distance,
				//	1.f / 16, steps);
				draw_text(texture_shader_, -0.32f, -1.21f, 1.f / 512, steps);
				if (steps_alteration_duration != 0)
				{
					char bonus_message[40];
					steps_alteration_duration--;
					switch (steps_alteration)
					{
					case double_value:
						sprintf(bonus_message, "Soul has been doubled!");
						break;
					case bonus:
						sprintf(bonus_message, "+%d bonus received",bonus_value);
						break;
					case fake_exit:
						sprintf(bonus_message, "It's a trap! -%d soul essence",bonus_value);
						break;
					default:
						break;
					}
					draw_text(texture_shader_, 0.4f, -1.21f, 1.f / 512, bonus_message);
				}
			}

			// move the listener with the camera
			vec4 &cpos = camera.get_camera().w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
		}

		void simulate() {

			if (game_over)
				return;
			else if (level.steps == 0)
			{
				int old_x = sprites[game_over_sprite].get_position().x(),
					old_y = sprites[game_over_sprite].get_position().y();

				sprites[board_sprite].is_enabled() = false;
				sprites[game_over_sprite].is_enabled() = true;
				game_over = true;
			}
			else if (level_complete)
			{
				if (is_key_down(key_enter))
					generate_new_level();
			}
			else
			{
				if (is_key_down(key_enter))
				{
					generate_new_level();
				}


				previous_scary_face_time_interval++;

				move_character();

				animate_sprites();

				animate_pickups();
				
				//XY movement
				if (camera.is_moving())
				{
					camera.move();
				}

				//Z movement and character transparency
				if ((level.initial_steps - level.steps) / character.fading_point > (1 - sprites[character_sprite].transparency) / 0.1f + 1)
				{
					camera.set_translation(0, 0, (camera_fin_pos - camera_init_pos) / 10.f);
					sprites[character_sprite].transparency -= 0.1f;
				}
			}
		}
	};
}
