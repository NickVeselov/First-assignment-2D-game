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
	public:
		sprite() {
			texture = 0;
			enabled = true;
		}

		void init(int _texture, float x, float y, float w, float h) {
			modelToWorld.loadIdentity();
			modelToWorld.translate(x, y, 0);
			halfWidth = w * 0.5f;
			halfHeight = h * 0.5f;
			texture = _texture;
			enabled = true;
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
		}

		// position the object relative to another.
		void set_relative(sprite &rhs, float x, float y) {
			modelToWorld = rhs.modelToWorld;
			modelToWorld.translate(x, y, 0);
		}

		// return true if this sprite collides with another.
		// note the "const"s which say we do not modify either sprite
		bool collides_with(const sprite &rhs) const {
			float dx = rhs.modelToWorld[3][0] - modelToWorld[3][0];
			float dy = rhs.modelToWorld[3][1] - modelToWorld[3][1];

			// both distances have to be under the sum of the halfwidths
			// for a collision
			return
				(fabsf(dx) < halfWidth + rhs.halfWidth) &&
				(fabsf(dy) < halfHeight + rhs.halfHeight)
				;
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
	};

	class labyrinth_app : public octet::app {
		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;
		// shader to draw a textured triangle
		texture_shader texture_shader_;

		Labyrinth lab;

		enum {
			num_sound_sources = 8,
			num_borders = 4,

			//labyrinth parameters
			num_walls = 500,

			border_width = 1,
			
			// sprite definitions

			first_border_sprite = 0,
			last_border_sprite = first_border_sprite + num_borders - 1,

			first_wall_sprite,
			last_wall_sprite = first_wall_sprite + num_walls - 1,

			num_sprites,

		};

		// timers for missiles and bombs
		int missiles_disabled;
		int bombs_disabled;

		// accounting for bad guys
		int live_invaderers;
		int num_lives;

		// game state
		bool game_over;
		int score;

		// speed of enemy
		float invader_velocity;

		// sounds
		ALuint whoosh;
		ALuint background;
		ALuint bang;
		unsigned cur_source;
		ALuint sources[num_sound_sources];

		// big array of sprites
		sprite sprites[num_sprites];

		// random number generator
		class random randomizer;

		// a texture for our text
		GLuint font_texture;

		// information for our text
		bitmap_font font;

		ALuint get_sound_source() { return sources[cur_source++ % num_sound_sources]; }

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

		//dynamic behaviour

		// use the keyboard to move the player
		void move_ship() {
			const float ship_speed = 0.05f;
			// left and right arrows
			if (is_key_down(key_left)) {
				sprites[player_sprite].translate(-ship_speed, 0);
				//if (sprites[player_sprite].collides_with(sprites[first_border_sprite + 2])) {
				//	sprites[player_sprite].translate(+ship_speed, 0);
				//}
			}
			else if (is_key_down(key_right)) {
				sprites[player_sprite].translate(+ship_speed, 0);
				//if (sprites[player_sprite].collides_with(sprites[first_border_sprite + 3])) {
				//	sprites[player_sprite].translate(-ship_speed, 0);
				//}
			}
			else if (is_key_down(key_up)) {
				sprites[player_sprite].translate(0, +ship_speed);
				//if (sprites[player_sprite].collides_with(sprites[first_border_sprite + 3])) {
				//	sprites[player_sprite].translate(-ship_speed, 0);
				//}
			}
			else if (is_key_down(key_down)) {
				sprites[player_sprite].translate(0, -ship_speed);
				//if (sprites[player_sprite].collides_with(sprites[first_border_sprite + 3])) {
				//	sprites[player_sprite].translate(-ship_speed, 0);
				//}
			}
		}


		int current_sprite;
		int player_sprite;
	public:

		// this is called when we construct the class
		labyrinth_app(int argc, char **argv) : app(argc, argv), font(512, 256, "assets/big.fnt") {
		}

		// this is called once OpenGL is initialized
		void app_init() {
			// set up the shader
			texture_shader_.init();

			// set up the matrices with a camera 5 units from the origin
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(0, 0, lab.map_size);

			//font_texture = resource_dict::get_texture_handle(GL_RGBA, "assets/big_0.gif");

			// set the border to white for clarity
			draw_map();
			// sundry counters and game state.

			num_lives = 5;
			game_over = false;
			score = 0;
		}

		void draw_map()
		{
			current_sprite = 1;
			lab.construct_labyrinth();
			draw_walls();

			int x0 = -lab.map_size + lab.alignment_left,
				x1 = lab.map_size - lab.alignment_right,
				y0 = -lab.map_size + lab.alignment_top,
				y1 = lab.map_size - lab.alignment_bottom,
				step = lab.step;

			float	net_width = 0.35f;

			player_sprite = current_sprite;
			GLuint player = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/ship.gif");
			sprites[current_sprite++].init(player, x0 + lab.entrance_index*step + step / 2., y0 + step / 2., step - 2 * net_width, step - 2 * net_width);

		}

		void draw_walls()
		{
			Cell **walls = lab.cells;

			GLuint wall = resource_dict::get_texture_handle(GL_RGB, "#ffffff");
			GLuint empty = resource_dict::get_texture_handle(GL_RGB, "#000000");
			GLuint gray = resource_dict::get_texture_handle(GL_RGB, "#111111");
			GLuint exit = resource_dict::get_texture_handle(GL_RGBA, "assets/invaderers/invaderer.gif");



			gray = wall;

			int x0 = -lab.map_size + lab.alignment_left,
				x1 = lab.map_size - lab.alignment_right,
				y0 = -lab.map_size + lab.alignment_top,
				y1 = lab.map_size - lab.alignment_bottom,
				step = lab.step;

			float border_width = 1.0f,
				wall_width = 0.5f,
				net_width = 0.35f;

			//outer walls
			sprites[current_sprite++].init(wall, 0, y0, x1 - x0 + wall_width, wall_width);
			sprites[current_sprite++].init(wall, 0, y1, x1 - x0 + wall_width, wall_width);
			sprites[current_sprite++].init(wall, x0, 0, wall_width, y1 - y0 + wall_width);
			sprites[current_sprite++].init(wall, x1, 0, wall_width, y1 - y0 + wall_width);

			//entrance
				sprites[current_sprite++].init(empty, x0 + lab.entrance_index*step + step / 2., y0 , step - net_width, border_width);

			//inner walls

			for (int i = 0; i<lab.cells_number; i++)
				for (int j = 0; j < lab.cells_number; j++)
				{
					if (walls[i][j].left_wall)
						sprites[current_sprite++].init(gray, x0 + j*step, y0 + i*step + step / 2., net_width, step + net_width);

					if (walls[i][j].top_wall)
						sprites[current_sprite++].init(gray, x0 + j*step + step / 2., y0 + (i + 1)*step, step + net_width, net_width);

					if (walls[i][j].right_wall)
						sprites[current_sprite++].init(gray, x0 + (j + 1)*step, y0 + i*step + step / 2., net_width, step + net_width);

					if (walls[i][j].bottom_wall)
						sprites[current_sprite++].init(gray, x0 + j*step + step / 2., y0 + i*step, step + net_width, net_width);
				}

			//exit

			sprites[current_sprite++].init(exit, x0 + lab.exit.x()*step + step / 2., y0 + lab.exit.y()*step + step / 2., step - 2*net_width, step - 2*net_width);
		}

		// this is called to draw the world
		void draw_world(int x, int y, int w, int h) {

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
			for (int i = 0; i != num_sprites; ++i) {
				sprites[i].render(texture_shader_, cameraToWorld);
			}

			char score_text[32];
			sprintf(score_text, "score: %d   lives: %d\n", score, num_lives);
			draw_text(texture_shader_, -1.75f, 2, 1.0f / 256, score_text);

			// move the listener with the camera
			vec4 &cpos = cameraToWorld.w();
			alListener3f(AL_POSITION, cpos.x(), cpos.y(), cpos.z());
		}


		void simulate() {
			if (game_over) {
				return;
			}

			move_ship();

		}
	};
}
