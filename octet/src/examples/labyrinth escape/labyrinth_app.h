#include "labyrinth.h"
#include "camera.h"

#pragma once
////////////////////////////////////////////////////////////////////////////////
//
// (C) Nikita Veselov 2016
//
// Introduction to the programming
// First assignment
// Goldsmiths University

namespace octet {

	
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
		int energy_bar_sprite;
		int reserve_bar_sprite;

		GLuint Void;
		GLuint Reserve;
		vec2 energy_bar_size;
		vec2 energy_bar_position;

		// game state
		bool game_over;
		bool level_complete;
		bool character_killed;
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

		void game_initialization()
		{
			game_over = false;
			character_killed = false;
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

			//GLuint Energy = resource_dict::get_texture_handle(GL_RGBA, "#6cabbd");
			////ene = current_sprite;
			//sprites[current_sprite].init(Energy, -0.5f, -0.8f, 0.55f, 0.10f);
			//sprites[current_sprite].static_position = true;
			//sprites[current_sprite++].is_enabled() = true;

			GLuint Energy = resource_dict::get_texture_handle(GL_RGBA, "#12115e");
			Reserve = resource_dict::get_texture_handle(GL_RGBA, "#FF0000");
			Void = resource_dict::get_texture_handle(GL_RGBA, "#000000");
			energy_bar_position = vec2(-0.26f, -0.6f);
			energy_bar_size = vec2(0.4f, 0.13f);

			energy_bar_sprite = current_sprite;
			sprites[current_sprite++].init(Void, energy_bar_position.x(), energy_bar_position.y(), 0, 0);
			sprites[energy_bar_sprite].static_position = true;

			sprites[current_sprite].init(Energy, energy_bar_position.x(), energy_bar_position.y(), energy_bar_size.x(), energy_bar_size.y());
			sprites[current_sprite++].static_position = true;
			
			reserve_bar_sprite = current_sprite;
			sprites[current_sprite++].init(Reserve, energy_bar_position.x(), energy_bar_position.y(), energy_bar_size.x(), energy_bar_size.y());
			sprites[energy_bar_sprite].static_position = true;
			
			GLuint Board = resource_dict::get_texture_handle(GL_RGBA, "#808080");
			board_sprite = current_sprite;
			sprites[current_sprite++].init(Board, 0, -0.7f, 1.8f, 0.4f);
			sprites[board_sprite].static_position = true;

			GLint evil_face = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/evil ghost.gif");
			scary_image_sprite = current_sprite;
			sprites[current_sprite++].init(evil_face, 0, 0, 2.f, 2.f);
			sprites[scary_image_sprite].static_position = true;
			sprites[scary_image_sprite].is_enabled() = false;
			sprites[scary_image_sprite].transparency = 0.5f;

			GLuint character_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/character.gif");
			character_sprite = current_sprite;
			sprites[current_sprite++].init(character_texture, 0, 0,
				lab.cell_size - border_width - wall_width, lab.cell_size - border_width - wall_width);

			static_sprites_number = current_sprite;

			character.speed = lab.cell_size / 3.f;
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

			level.reserve += level.steps;
			int steps = 0.5f*lab.path_length + rand() % lab.path_length;
			level.steps = level.initial_steps = steps;


			character.fading_point = level.steps / 10.f;
			sprites[character_sprite].transparency = 1;

			steps_alteration = none;
			steps_alteration_duration = 0;
			bonus_value = 25;
			scary_face_timeout = 0;
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
			int double_bonuses_number = 0;
			for (int i = content_cells.size() - exits_number - 1; i >= stop_index; i--)
			{
				GLuint soul_loot;
				content_cells[i].bouncing = true;
				content_cells[i].bounce_max_value = lab.cell_size / 20.f;
				float r = rand() % 2 * content_cells[i].bounce_max_value;
				content_cells[i].current_bounce_position = -content_cells[i].bounce_max_value + r;
				r = rand() % 2;
				if (r == 0)
					content_cells[i].direction = true;
				else
					content_cells[i].direction = false;

				int type = rand() % 2;
				if (double_bonuses_number == 1)
					type = 0;
				
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
					double_bonuses_number++;
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
				pickups_number = std::min(10,(int)content_cells.size() - 1 - 3);
			create_exits(3);
			create_pickups(4, std::max((int)content_cells.size() - (exits_number + pickups_number), 0));
		}

		void generate_new_level()
		{
			level.id++;
			sprites[level_complete_sprite].is_enabled() = false;
			//turn character
			if (character.pointed_left)
			{
				sprites[character_sprite].rotateY180();
				character.pointed_left = false;
			}

			//disable sprites
			for (int i = static_sprites_number; i <= current_sprite; i++)
				sprites[i].is_enabled() = false;
			current_sprite = static_sprites_number;

			//reconstruct labyrinth
			content_cells.clear();
			lab.construct_labyrinth();
			draw_walls();

			//move character to the entrance
			sprites[character_sprite].move_to_the_cell(0, lab.entrance_index, lab.cell_size);

			set_variables();

			add_labyrinth_content();

			//center camera on the character
			camera.go_to(sprites[character_sprite].get_position().x(), sprites[character_sprite].get_position().y(), camera.init_distance);

			level_complete = false;
			character.moving = false;
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
					if (level.steps < level.initial_steps / 10)
						level.steps = level.steps;
					camera.set_translation((character.x - x)*lab.cell_size, (character.y - y)*lab.cell_size, 0);
					level.steps--;
					character.moving = true;
					//check for looting/escaping
					check_collision();

					update_energy_bar();
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
				show_evil_face(10 + 2.f*level.initial_steps / level.steps);

			for (int i = 0; i < content_cells.size(); i++)
			{
				if (content_cells[i].x == character.x && content_cells[i].y == character.y)
				{
					switch (content_cells[i].loot)
					{
					case bonus:
						level.steps += level.initial_steps*bonus_value/100;
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
						level.steps -= level.initial_steps* bonus_value/100;
						if (level.steps <= 0)
						{
							if (level.reserve > 0)
								use_reserves();
							else
								character_killed = true;
						}
						handle_looting_event();
						sprites[content_cells[i].sprite_index].is_enabled() = false;
						steps_alteration = content_cells[i].loot;
						steps_alteration_duration = 100;
						content_cells[i].loot = none;
						show_evil_face(60,true);
						break;
					case exit:
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
			camera.translate(vec3(0, 0, camera.min_distance + current_pos*(camera.init_distance - camera.min_distance) - camera.get_z()));
		}

		void show_evil_face(int duration, bool mandatory = false)
		{
			if (previous_scary_face_time_interval > 1000 || mandatory)
			{
				previous_scary_face_time_interval = 0;
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
						max_pos = content_cells[i].bounce_max_value,
						speed = max_pos / 20;
					if (content_cells[i].direction)
					{
						content_cells[i].current_bounce_position += speed;
						sprites[content_cells[i].sprite_index].translate(0, +speed);
					}
					else
					{
						content_cells[i].current_bounce_position -= speed;
						sprites[content_cells[i].sprite_index].translate(0, -speed);
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

		void update_energy_bar()
		{
			int percentage = 100 - level.steps * 100 / level.initial_steps;
			if (percentage < 0)
				percentage = 0;
			vec2 bar_width = vec2(percentage*energy_bar_size.x()/100.f, energy_bar_size.y())*0.95f;
			float location_x = (energy_bar_position.x() + energy_bar_size.x() / 2.f) - bar_width.x()/ 2.f;
			sprites[energy_bar_sprite].init(Void, location_x, energy_bar_position.y(), bar_width.x(), bar_width.y());
			sprites[energy_bar_sprite].static_position = true;

			if (level.reserve > 0)
			{
				sprites[reserve_bar_sprite].init(Reserve, energy_bar_position.x() + energy_bar_size.x() + 0.52f, energy_bar_position.y(), 
					energy_bar_size.x()*0.8f, energy_bar_size.y());
				sprites[reserve_bar_sprite].static_position = true;
			}
		}

		void use_reserves()
		{
			level.steps = std::min(level.initial_steps, level.reserve);
			level.reserve -= level.steps;
			if (level.reserve <= 0)
				sprites[reserve_bar_sprite].is_enabled() = false;

			handle_looting_event();
		}

		void draw_text_lines()
		{
			if (!game_over && !level_complete)
			{
				float top_level = -0.95f,
					size = 1.f / 600,
					left_line = -0.25f,
					right_line = 0.65f;

				//Energy
				int percent = level.steps * 100 / level.initial_steps;
				char energy_text[32];
				sprintf(energy_text, "Energy:      %d%%", percent);
				draw_text(texture_shader_, left_line, top_level, size, energy_text);
				//Reserve
				if (level.reserve > 0)
				{
					int percent = level.reserve * 100 / level.initial_steps;
					char reserve_text[32];
					sprintf(reserve_text, "Reserve:      %d%%", percent);
					draw_text(texture_shader_, right_line, top_level, size, reserve_text);
				}


				if (steps_alteration_duration > 0)
				{
					char bonus_message[40];
					steps_alteration_duration--;
					switch (steps_alteration)
					{
					case double_value:
						sprintf(bonus_message, "Energy has been doubled!");
						break;
					case bonus:
						sprintf(bonus_message, "+%d%% bonus received", bonus_value);
						break;
					case fake_exit:
						sprintf(bonus_message, "It's a trap! -%d energy", bonus_value);
						break;
					default:
						break;
					}
					draw_text(texture_shader_, 0, -1.13f, size, bonus_message);
				}
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
				aabb bb(vec3(0, 0, 0), vec3(350, 256, 0));

				unsigned num_quads = font.build_mesh(bb, vertices, indices, max_quads, text,0);
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
			camera.init(lab.cell_size,lab.absolute_size);
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
			
			draw_text_lines();

			// move the listener with the camera
			vec4 &cpos = camera.get_camera().w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
		}

		void simulate() {

			if (game_over)
				return;
			else if (character_killed)
			{
				if (scary_face_timeout != 0)
				{
					scary_face_timeout--;
					if (scary_face_timeout == 0)
					{
						sprites[scary_image_sprite].is_enabled() = false;
						sprites[game_over_sprite].is_enabled() = true;
						game_over = true;
					}
				}
			}
			//if the steps = 0 -> game over
			else if (level.steps <= 0)
			{
				if (level.reserve <= 0)
				{
					sprites[game_over_sprite].is_enabled() = true;
					game_over = true;
				}
				else
					use_reserves();
			}
			//if the level completed
			else if (level_complete)
			{
				if (is_key_down(key_enter))
				{
					generate_new_level();
					update_energy_bar();
				}
			}
			//if the image of a ghost is shown
			else
			{
				//test only
				if (is_key_down(key_enter))
					generate_new_level();

				previous_scary_face_time_interval++;

				move_character();

				animate_pickups();

				animate_sprites();

				//XY movement
				if (camera.is_moving())
				{
					camera.move();
				}

				//Z movement and character transparency
				if ((level.initial_steps - level.steps) / character.fading_point > (1 - sprites[character_sprite].transparency) / 0.1f + 1)
				{
					camera.set_translation(0, 0, (camera.min_distance - camera.init_distance) / 10.f);
					sprites[character_sprite].transparency -= 0.1f;
				}
			}
		}
	};
}
