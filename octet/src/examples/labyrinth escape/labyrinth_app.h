#include <cstdlib>
#include <ctime>

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


	class StackNode
	{
	public:
		int x;
		int y;
		StackNode* next;
	};

	class Stack
	{
	private:
		StackNode *head;
		int count = 1;
	public:
		vec2 pop()
		{
			if (head == NULL)
				return (vec2(-1, -1));
			else
			{
				StackNode *popped = head;
				head = head->next;
				int x = popped->x,
					y = popped->y;
				delete popped;
				count--;
				return (vec2(x, y));
			}
		}

		void push(int x, int y)
		{
			StackNode *pushed = new StackNode;
			pushed->next = head;
			pushed->x = x;
			pushed->y = y;
			head = pushed;
			count++;
		}

		Stack(int x, int y)
		{
			head = new StackNode;
			head->x = x;
			head->y = y;
			head->next = NULL;
		}

		Stack()
		{
			head = NULL;
			count = 0;
		}

		bool isEmpty()
		{
			if (head == NULL)
				return true;
			else
				return false;
		}
	};


	class labyrinth_app : public octet::app {
		// Matrix to transform points in our camera space to the world.
		// This lets us move our camera
		mat4t cameraToWorld;

		// shader to draw a textured triangle
		texture_shader texture_shader_;

		enum {
			num_sound_sources = 8,
			num_borders = 4,

			//labyrinth parameters
			map_size = 30,
			num_walls = 500,
			alignment_top = 2,
			alignment_bottom = alignment_top,
			alignment_left = alignment_top,
			alignment_right = alignment_top,

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


		//labyrinth generator
		int cells_number;
		int step;
		bool visited[map_size][map_size];
		int current_sprite;
		float net_width;
		Stack labyrinth_stack;
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
			cameraToWorld.translate(0, 0, map_size);

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
			current_sprite = first_border_sprite;

			float border_width = 1.0f,
				wall_width = 0.5f;

			GLuint white = resource_dict::get_texture_handle(GL_RGB, "#ffffff");
			GLuint black = resource_dict::get_texture_handle(GL_RGB, "#000000");
			GLuint gray = resource_dict::get_texture_handle(GL_RGB, "#808080");

			#pragma region Draw Borders

			net_width = 0.35f;

			int x0 = -map_size + alignment_left,
				x1 = map_size - alignment_right,
				y0 = -map_size + alignment_top,
				y1 = map_size - alignment_bottom;
			step = 8;

			float space_width = 2.f,
				space_height = 2.f;
			for (int i = x0; i <= x1; i += step)
				sprites[current_sprite++].init(gray, 0,	i,	y1 - y0 + net_width, net_width);
			for (int i = y0; i <= y1; i += step)
				sprites[current_sprite++].init(gray, i, 0, net_width, x1 - x0 + net_width);

			sprites[current_sprite++].init(white, y0, 0, net_width, x1 - x0 + net_width);
			sprites[current_sprite++].init(white, y1, 0, net_width, x1 - x0 + net_width);
			sprites[current_sprite++].init(white, 0, x0, x1 - x0 + net_width, net_width);
			sprites[current_sprite++].init(white, 0, x1, x1 - x0 + net_width, net_width);

			#pragma endregion

			cells_number = (x1 - x0) / step;

			srand(time(NULL));

			int entrance_index = rand() % cells_number,
				exit_index = rand() % cells_number;
			#pragma region Draw Labyrinth

			//entrance

			sprites[current_sprite++].init(black, x0 + entrance_index*step + step/2., y0, step - net_width, net_width);

			#pragma endregion

			draw_labyrinth(entrance_index);
		}

		//Recursive backtracker algorithm: https://en.wikipedia.org/wiki/Maze_generation_algorithm
		void draw_labyrinth(int start_index)
		{
			for (int i = 0; i < cells_number; i++)
				for (int j = 0; j < cells_number; j++)
					visited[i][j] = false;

			vec2 current_cell(start_index, 0);
			Stack stack(start_index, 0);
			visited[start_index][0] = true;
			bool complete = false;
			GLuint black = resource_dict::get_texture_handle(GL_RGB, "#000000");
			GLuint white = resource_dict::get_texture_handle(GL_RGB, "#FF0000");
			int x0 = -map_size + alignment_left,
				x1 = map_size - alignment_right,
				y0 = -map_size + alignment_top,
				y1 = map_size - alignment_bottom;
			int it = 0;
			while (!complete)
			{
				vec2 next_cell = find_unvisited_cell((int)current_cell.x(), (int)current_cell.y());
				if (next_cell.x() == -1)
					complete = true;
				else
				{
					int x = next_cell.x(), y = next_cell.y();
					stack.push(x, y);
					visited[x][y] = true;

					//remove wall
					if (current_cell.x() == next_cell.x())
						if (current_cell.y() > next_cell.y())
							sprites[current_sprite++].init(white, x0 + current_cell.x()*step + step / 2, y0 + (next_cell.y() + 1)*step, step - net_width, net_width);
						else
							sprites[current_sprite++].init(white, x0 + current_cell.x()*step + step / 2, y0 + (current_cell.y() + 1)*step, step - net_width, net_width);
					else if (current_cell.x() > next_cell.x())
						sprites[current_sprite++].init(white, x0 + step * current_cell.x(), y0 + current_cell.y()*step + step / 2, net_width, step - net_width);
					else
						sprites[current_sprite++].init(white, x0 + step*next_cell.x(), y0 + current_cell.y()*step + step / 2, net_width, step - net_width);


					current_cell = next_cell;
				}
			}
		}

		vec2 find_unvisited_cell(int cur_x, int cur_y)
		{
			int dir[4] = { -1, -1, -1, -1 };
			int cur = 0;

			//check directions for emptiness
			if (check_cell(cur_x - 1, cur_y))
				dir[cur++] = 1;
			if (check_cell(cur_x, cur_y + 1))
				dir[cur++] = 2;
			if (check_cell(cur_x + 1, cur_y))
				dir[cur++] = 3;
			if (check_cell(cur_x, cur_y - 1))
				dir[cur++] = 4;

			//if some unvisited neighbour exists
			if (cur > 0)
			{
				int random_direction = dir[rand() % cur];
				if (random_direction == -1)
					int a = 5;
				switch (random_direction)
				{
					//left
				case 1:
					printf_s("left\n");
					return (vec2(cur_x - 1, cur_y));
					//top
				case 2:
					printf_s("top\n");
					return (vec2(cur_x, cur_y + 1));
					//right
				case 3:
					printf_s("right\n");
					return (vec2(cur_x + 1, cur_y));
					//bottom
				case 4:
					printf_s("bottom\n");
					return (vec2(cur_x, cur_y - 1));
				}
			}
			//no unvisited neighbours
			else
			{
				vec2 previous = labyrinth_stack.pop();
				if (previous.x() == -1)
					return previous;
				find_unvisited_cell((int)previous.x(), (int)previous.y());				
			}
		}
		
		bool check_cell(int x, int y)
		{
			if ((x >= 0) && (x < cells_number)	&& (y >= 0) && (y < cells_number))
			{
				if (visited[x][y])
					return false;
				else
					return true;
			}
			return false;
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
	};
}
