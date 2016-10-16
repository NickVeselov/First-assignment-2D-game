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
			shader.render(modelToProjection, 0);

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

		vec2 camera_path_remaining;
		vec2 camera_speed;
		vec2 basic_camera_speed;
		vec2 camera_position;
		int camera_distance;
	#pragma endregion

	#pragma region private functions
		//smoothly move camera
		void move_camera()
		{
			if (camera_path_remaining.x() != 0 || camera_path_remaining.y() != 0)
			{
				//adjust speed
				int dx = camera_path_remaining.x(), dy = camera_path_remaining.y(),
					a_dx = abs(dx), a_dy = abs(dy);
				vec2 distance_left = vec2(a_dx / lab.cell_size, a_dy / lab.cell_size);
				camera_speed = vec2(0, 0);

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

				cameraToWorld.translate(camera_speed.x(), camera_speed.y(), 0);
				camera_position += camera_speed;
				sprites[board_sprite].translate(camera_speed.x(), camera_speed.y());
				camera_path_remaining -= camera_speed;
			}
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
					camera_path_remaining += vec2((character.x - x)*character.speed, (character.y - y)*character.speed);
					character.actual_position += vec2((character.x - x)*character.speed, (character.y - y)*character.speed);
					character_moving = true;
					character.steps--;
					if (character.steps == 0)
						game_over = true;
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

			shader.render(modelToProjection, 0);

			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].x);
			glEnableVertexAttribArray(attribute_pos);
			glVertexAttribPointer(attribute_uv, 3, GL_FLOAT, GL_FALSE, sizeof(bitmap_font::vertex), (void*)&vertices[0].u);
			glEnableVertexAttribArray(attribute_uv);

			glDrawElements(GL_TRIANGLES, num_quads * 6, GL_UNSIGNED_INT, indices);
		}

		ALuint get_sound_source() { return sources[cur_source++ % 8]; }

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
			camera_distance = lab.absolute_size*0.3f;
			basic_camera_speed = vec2(lab.cell_size / 40.f, lab.cell_size / 40.f);
			camera_path_remaining = vec2(0, 0);
			movement_frames_threshold = 2;
			movement_frames_counter = 0;
			character_moving = false;

			draw_map();

			//center camera on the character
			camera_position = vec2(sprites[character_sprite].get_position().x(), lab.absolute_size / 6.f);
			cameraToWorld.translate(camera_position.x(),camera_position.y(), camera_distance);
			//cameraToWorld.translate(lab.half_size, lab.half_size, camera_distance);

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
			status_bar.height = camera_distance / 8.f;
			sprites[current_sprite++].init(Board, sprites[character_sprite].get_position().x(), lab.absolute_size / 6.f - (camera_distance - status_bar.height),
				1.9f * camera_distance, status_bar.height);
			sprites[board_sprite].is_enabled() = true;
		}

		void draw_map()
		{
			float border_width = 1,
				wall_width = 0.4f;
				

			current_sprite = 0;
			lab.construct_labyrinth();
			draw_walls(border_width,wall_width);

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
			character.steps = lab.path_length*1.1f;

			GLuint GameOver = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/game over.gif");
			game_over_sprite = current_sprite;
			sprites[current_sprite++].init(GameOver, sprites[exit_sprite].get_position().x(), sprites[exit_sprite].get_position().y(), 2 * camera_distance, 2 * camera_distance);
			sprites[game_over_sprite].is_enabled() = false;


		}

		void draw_walls(float border_width, float wall_width)
		{
			GLuint border = resource_dict::get_texture_handle(GL_RGB, "#FFFFFF");
			GLuint wall = resource_dict::get_texture_handle(GL_RGB, "#888888");
			GLuint void_space = resource_dict::get_texture_handle(GL_RGB, "#000000");
			GLuint exit_staircase = resource_dict::get_texture_handle(GL_RGBA, "assets/labyrinth/staircase 2.gif");

			//entrance
			sprites[current_sprite++].init(void_space, lab.entrance_index*lab.cell_size + lab.half_cell, 0 , lab.cell_size - wall_width, border_width);

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
				}

			//outer walls
			sprites[current_sprite++].init(border, lab.half_size, 0, lab.absolute_size + border_width, border_width);
			sprites[current_sprite++].init(border, lab.half_size, lab.absolute_size, lab.absolute_size + border_width, border_width);
			sprites[current_sprite++].init(border, 0, lab.half_size, border_width, lab.absolute_size + border_width);
			sprites[current_sprite++].init(border, lab.absolute_size, lab.half_size, border_width, lab.absolute_size + border_width);

			//exit
			exit_sprite = current_sprite;
			sprites[current_sprite++].init(exit_staircase, lab.exit.x()*lab.cell_size + lab.half_cell, lab.exit.y()*lab.cell_size + lab.half_cell,
				lab.cell_size - 2*wall_width, lab.cell_size - 2*wall_width);
		}
		
		void generate_new_level()
		{
			current_sprite = 0;
			draw_map();

			camera_path_remaining = vec2(0, 0);
			cameraToWorld.translate(sprites[character_sprite].get_position().x() - camera_position.x(), lab.absolute_size/4.f - camera_position.y(), 0);
			camera_position += vec2(sprites[character_sprite].get_position().x() - camera_position.x(), lab.absolute_size / 4.f - camera_position.y());
			level_complete = false;

			GLuint Board = resource_dict::get_texture_handle(GL_RGBA, "#800080");
			board_sprite = current_sprite;
			status_bar.height = camera_distance / 8.f;
			sprites[current_sprite++].init(Board, camera_position.x(), camera_position.y() - (camera_distance - status_bar.height),
				1.9f * camera_distance, status_bar.height);
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
			
			char steps[32];// = "Steps:" + character.steps;
			sprintf(steps,"Steps:%d", character.steps);
			draw_text(texture_shader_, camera_position.x() - 0.55f*camera_distance,
				camera_position.y() - 1.15f*camera_distance,
				1.f/16, steps);

			// move the listener with the camera
			vec4 &cpos = cameraToWorld.w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
		}

		void simulate() {	

			if (level_complete) {

				if (is_key_down(key_enter))
					generate_new_level();
			}
			else
			{
				if (game_over)
				{
					//cameraToWorld.translate((int)camera_path_remaining.x(), (int)camera_path_remaining.y(), 0);
					return;
				}

				move_player();

				move_camera();

				//check end
				if ((character.x == (int)lab.exit.x()) && (character.y == (int)lab.exit.y()))
				{
					//cameraToWorld.translate(camera_path_remaining.x(), camera_path_remaining.y(), 0);
					//camera_position += camera_path_remaining;
					sprites[board_sprite].is_enabled() = false;

					level_complete = true;

					int old_x = sprites[game_over_sprite].get_position().x(),
						old_y = sprites[game_over_sprite].get_position().y();

					sprites[game_over_sprite].translate(camera_position.x() - old_x, camera_position.y() - old_y);
					sprites[game_over_sprite].is_enabled() = true;
				}
			}
		}
	};
}
