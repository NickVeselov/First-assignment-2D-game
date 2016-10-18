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
		int x;
		int y;
		
	public:

		float transparency;

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
		}

		void render(texture_shader &shader, mat4t &cameraToWorld) {
			// invisible sprite... used for gameplay.
			if (!texture) return;

			// build a projection matrix: model -> world -> camera -> projection
			// the projection space is the cube -1 <= x/w, y/w, z/w <= 1
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

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
		void translate(float x, float y) {
			modelToWorld.translate(x, y, 0);
			x += x;
			y += y;
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

		//get the position of the sprite
		vec2 get_position()
		{
			return vec2(x, y);
		}
	};

	class labyrinth_app : public octet::app {

	#pragma region private variables
		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

		// shader to draw a textured triangle
		texture_shader texture_shader_;

		//labyrinth class (labyrinth.h)
		Labyrinth lab;
		//character data (position, loot, etc.)
		character character;
		//status bar with text
		StatusBar status_bar;

		// big array of sprites
		sprite sprites[2000];

		//special sprites
		int current_sprite;
		int character_sprite;
		int game_over_sprite;
		int board_sprite;
		int exit_sprite;
		int level_complete_sprite;

		// game state
		bool game_over;
		bool level_complete;
		int score;
		bool character_moving;
		int movement_frames_counter;
		int movement_frames_threshold;

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

		vec3 camera_path_remaining;
		vec3 camera_speed;
		vec3 basic_camera_speed;
		vec2 camera_position;
		int camera_initial_distance;
		int camera_max_distance;

		std::vector<Cell> distant_cells;

	#pragma endregion

	#pragma region private functions
		//smoothly move camera
		void move_camera()
		{
			//XY movement
			if (camera_path_remaining.x() != 0 || camera_path_remaining.y() != 0)
			{
				//adjust speed
				int dx = camera_path_remaining.x(), dy = camera_path_remaining.y(),
					a_dx = abs(dx), a_dy = abs(dy);
				vec3 distance_left = vec3(a_dx / lab.cell_size, a_dy / lab.cell_size, camera_path_remaining.z());
				camera_speed = vec3(0, 0, 0);

				if (a_dx > 0 && a_dx < lab.cell_size)
					distance_left.x()++;
				if (a_dy > 0 && a_dy < lab.cell_size)
					distance_left.y()++;

				distance_left.x() = pow(2.f, distance_left.x() - 1);
				distance_left.y() = pow(2.f, distance_left.y() - 1);

				camera_speed += basic_camera_speed * distance_left;

				if (a_dx < camera_speed.x())
					camera_speed.x() = a_dx;
				if (a_dy < camera_speed.y())
					camera_speed.y() = a_dy;

				if (dx < 0)
					camera_speed.x() *= -1;
				if (dy < 0)
					camera_speed.y() *= -1;

				cameraToWorld.translate(camera_speed.x(), camera_speed.y(), camera_speed.z());
				camera_position += vec2(camera_speed.x(), camera_speed.y());
				sprites[board_sprite].translate(camera_speed.x(), camera_speed.y());
				camera_path_remaining -= camera_speed;
			}

			//Z movement and character transparency - disabled
			//if ((character.initial_steps - character.steps) / character.fading_point > (1 - character.transparency)/0.1f + 1)
			//{
			//	camera_path_remaining.z() = (camera_max_distance - camera_initial_distance)*0.1f;
			//	character.transparency -= 0.1f;

			//	sprites[board_sprite].translate(0, -camera_path_remaining.z()/1.1f);
			//	//sprites[board_sprite].resize(camera_path_remaining.z() * 1.41f / , camera_path_remaining.z() / 1.41f);
			//	//sprites[board_sprite]
			//}
		}
		
		// use the keyboard to move the character
		void move_player() {

			if (!character_moving)
			{
				const float movespeed = lab.cell_size;
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
						sprites[character_sprite].translate(+movespeed, 0);
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
						sprites[character_sprite].translate(+movespeed, 0);
						character.x++;
					}
				}
				else if ((is_key_down(key_up)) && (character.y < lab.cells_number)) {
					if (!lab.cells[character.y][character.x].top_wall)
					{
						sprites[character_sprite].translate(0, +movespeed);
						character.y++;
					}
				}
				else if ((is_key_down(key_down)) && (character.y > 0)) {
					if (!lab.cells[character.y][character.x].bottom_wall)
					{
						sprites[character_sprite].translate(0, -movespeed);
						character.y--;
					}
				}

				//if movement occured
				if (character.x != x || character.y != y)
				{
					camera_path_remaining += vec3((character.x - x)*character.speed, (character.y - y)*character.speed, 0);
					character.actual_position += vec2((character.x - x)*character.speed, (character.y - y)*character.speed);
					character_moving = true;
					character.steps--;
				}
			}
			else
			{
				movement_frames_counter++;
				if (movement_frames_counter == movement_frames_threshold)
				{
					character_moving = false;
					movement_frames_counter = 0;
				}
			}
			
		}

		//draw text on the screen
		void draw_text(texture_shader &shader, float x, float y, float scale, const char *text) {
			mat4t modelToWorld;
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			modelToWorld.scale(scale, scale, 1);
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

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

		ALuint get_sound_source() { return sources[cur_source++ % 8]; }

		void draw_map(int steps_collection)
		{
			float border_width = 1,
				wall_width = 0.4f;


			current_sprite = 0;
			lab.construct_labyrinth();
			draw_walls(border_width, wall_width);

			//test - increased hall width
			int hall_width = 1;

			//character
			GLuint character_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/character.gif");
			character_sprite = current_sprite;
			sprites[current_sprite++].init(character_texture, lab.entrance_index*lab.cell_size + lab.half_cell, lab.half_cell,
				lab.cell_size - border_width - wall_width, lab.cell_size - border_width - wall_width);

			character.x = lab.entrance_index;
			character.y = 0;
			character.actual_position = vec2(lab.entrance_index*lab.cell_size, 0);
			character.initial_steps = character.steps = lab.path_length + steps_collection;
			character.initial_steps = character.steps += rand() % character.steps / 2.f;
			character.transparency = 1.0f;
			character.fading_point = character.steps / 10.f;

			GLuint LevelComplete = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/level complete.gif");
			level_complete_sprite = current_sprite;
			sprites[current_sprite++].init(LevelComplete, sprites[exit_sprite].get_position().x(), sprites[exit_sprite].get_position().y(), 2 * camera_initial_distance, 2 * camera_initial_distance);
			sprites[level_complete_sprite].is_enabled() = false;

			GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/game over.gif");
			game_over_sprite = current_sprite;
			sprites[current_sprite++].init(GameOver, sprites[exit_sprite].get_position().x(), sprites[exit_sprite].get_position().y(), 2 * camera_initial_distance, 2 * camera_initial_distance);
			sprites[game_over_sprite].is_enabled() = false;
		}

		void draw_walls(float border_width, float wall_width)
		{
			GLuint border = resource_dict::get_texture_handle(GL_RGB, "#FFFFFF");
			GLuint wall = resource_dict::get_texture_handle(GL_RGB, "#888888");
			GLuint void_space = resource_dict::get_texture_handle(GL_RGB, "#000000");

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
						distant_cells.push_back(lab.cells[i][j]);
						
				}

			//outer walls
			sprites[current_sprite++].init(border, lab.half_size, 0, lab.absolute_size + border_width, border_width);
			sprites[current_sprite++].init(border, lab.half_size, lab.absolute_size, lab.absolute_size + border_width, border_width);
			sprites[current_sprite++].init(border, 0, lab.half_size, border_width, lab.absolute_size + border_width);
			sprites[current_sprite++].init(border, lab.absolute_size, lab.half_size, border_width, lab.absolute_size + border_width);
			
			add_labyrinth_content(lab.cell_size - border_width - wall_width);
		}

		void add_labyrinth_content(float cell_size)
		{
			GLuint exit_staircase = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/staircase.gif");
			GLuint soul_loot = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/loot.gif");

			//exit
			exit_sprite = current_sprite;
			sprites[current_sprite++].init(exit_staircase, lab.exit.x()*lab.cell_size + lab.half_cell, lab.exit.y()*lab.cell_size + lab.half_cell,
				cell_size, cell_size);

			std::sort(distant_cells.begin(), distant_cells.end());
			for (int i = distant_cells.size() - 2; i >= distant_cells.size() - 5; i--)
				sprites[current_sprite++].init(soul_loot, distant_cells[i].x*lab.cell_size + lab.half_cell, distant_cells[i].y*lab.cell_size + lab.half_cell,
					cell_size*0.7f, cell_size*0.7f);
		}

		void generate_new_level()
		{
			current_sprite = 0;
			draw_map(character.steps);

			camera_path_remaining = vec3(0, 0, 0);
			cameraToWorld.translate(sprites[character_sprite].get_position().x() - camera_position.x(), lab.absolute_size / 4.f - camera_position.y(), 0);
			camera_position += vec2(sprites[character_sprite].get_position().x() - camera_position.x(), lab.absolute_size / 4.f - camera_position.y());
			level_complete = false;

			GLuint Board = resource_dict::get_texture_handle(GL_RGBA, "#800080");
			board_sprite = current_sprite;
			status_bar.height = camera_initial_distance / 8.f;
			sprites[current_sprite++].init(Board, camera_position.x(), camera_position.y() - (camera_initial_distance - status_bar.height),
				1.9f * camera_initial_distance, status_bar.height);
		}

#pragma endregion

	public:

		// this is called when we construct the class
		labyrinth_app(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
		}

		//create new sprite

		//void new_sprite(GLuint texture, int x, int y, int w, int h)
		//{			
		//	//sprite s(texture, x, y, w, h);
		//	sprites.push_back(sprite(texture,x,y,w,h));
		//}

		// this is called once OpenGL is initialized
		void app_init() {
			// set up the shader
			texture_shader_.init();

			// set up the matrices with a camera 5 units from the origin
			cameraToWorld.loadIdentity();

			font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

			//set up camera
			camera_initial_distance = lab.absolute_size*0.6f;
			camera_max_distance = lab.absolute_size*0.2f;
			basic_camera_speed = vec3(lab.cell_size / 40.f, lab.cell_size / 40.f, (camera_initial_distance - camera_max_distance)/800.f);
			camera_path_remaining = vec3(0, 0, 0);
			movement_frames_threshold = 2;
			movement_frames_counter = 0;
			character_moving = false;

			draw_map(0);

			//center camera on the character
			//camera_position = vec2(sprites[character_sprite].get_position().x(), lab.absolute_size / 6.f);
			camera_position = vec2(lab.half_size, lab.half_size);
			cameraToWorld.translate(camera_position.x(),camera_position.y(), camera_initial_distance);
			//cameraToWorld.translate(lab.half_size, lab.half_size, camera_initial_distance);

			// sounds
			main_music_theme = resource_dict::get_sound_handle(AL_FORMAT_MONO16, "assets/labyrinth/background.wav");
			cur_source = 0;
			alGenSources(num_sound_sources, sources);
			ALuint source = get_sound_source();
			alSourcei(source, AL_BUFFER, main_music_theme);
			alSourcePlay(source);

			game_over = false;
			level_complete = false;
			character.speed = lab.cell_size;
			score = 0;

			GLuint Board = resource_dict::get_texture_handle(GL_RGBA, "#800080");
			board_sprite = current_sprite;
			status_bar.height = camera_initial_distance / 8.f;
			sprites[current_sprite++].init(Board, sprites[character_sprite].get_position().x(), lab.absolute_size / 6.f - (camera_initial_distance - status_bar.height),
				1.9f * camera_initial_distance, status_bar.height);
			sprites[board_sprite].is_enabled() = true;
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
			for (int i = 0; i != current_sprite; ++i) {
				if (sprites[i].is_enabled())
				sprites[i].render(texture_shader_, cameraToWorld);
			}
			
			if (!game_over)
			{
				char steps[32];// = "Steps:" + character.steps;
				sprintf(steps, "Steps:%d", character.steps);
				draw_text(texture_shader_, camera_position.x() - 0.55f*camera_initial_distance,
					camera_position.y() - 1.15f*camera_initial_distance,
					1.f / 16, steps);
			}

			// move the listener with the camera
			vec4 &cpos = cameraToWorld.w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
		}

		void simulate() {	
			if (game_over)
				return;
			else if (character.steps == 0)
			{
				int old_x = sprites[game_over_sprite].get_position().x(),
					old_y = sprites[game_over_sprite].get_position().y();

				sprites[board_sprite].is_enabled() = false;
				sprites[game_over_sprite].translate(camera_position.x() - old_x, camera_position.y() - old_y);
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
				move_player();

				move_camera();

				//check end
				if ((character.x == (int)lab.exit.x()) && (character.y == (int)lab.exit.y()))
				{
					//cameraToWorld.translate(camera_path_remaining.x(), camera_path_remaining.y(), 0);
					//camera_position += camera_path_remaining;
					sprites[board_sprite].is_enabled() = false;

					level_complete = true;

					int old_x = sprites[level_complete_sprite].get_position().x(),
						old_y = sprites[level_complete_sprite].get_position().y();

					sprites[level_complete_sprite].translate(camera_position.x() - old_x, camera_position.y() - old_y);
					sprites[level_complete_sprite].is_enabled() = true;
				}
			}
		}
	};
}
