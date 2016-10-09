#include <cstdlib>
#include <ctime>
#pragma once

namespace octet {

	struct Cell {
		int x;
		int y;
		bool left_wall = true;
		bool top_wall = true;
		bool right_wall = true;
		bool bottom_wall = true;
		Cell() { }
		Cell(int X, int Y)
		{
			x = X; y = Y;
		}
	};

	class Labyrinth {

		struct StackNode
		{
			int x;
			int y;
			StackNode* next;

			StackNode(int X, int Y, StackNode *Next)
			{
				x = X; y = Y; next = Next;
			}
		};
		
		class Stack
		{
		private:
			StackNode *head;
			int count = 1;
			int max_number = 1;
		public:
			vec2 distant_cell;
			vec2 pop()
			{
				if (head->next == NULL)
					return (vec2(-1, -1));
				else
				{
					StackNode *popped = head;
					StackNode *nexthead = head->next;
					delete popped;
					head = nexthead;
					count--;
					return (vec2(head->x, head->y));
				}
			}

			void push(int x, int y)
			{
				StackNode *pushed = new StackNode(x, y, head);
				head = pushed;
				count++;
				if (count > max_number)
				{
					max_number = count;
					distant_cell = vec2(x, y);
				}
			}

			vec2 get_head()
			{
				return(vec2(head->x, head->y));
			}

			Stack(int x, int y)
			{
				head = new StackNode(x, y, NULL);
			}

			bool isEmpty()
			{
				if (head == NULL)
					return true;
				else
					return false;
			}

			//ToDO: delete this function (test only)
			void showElements()
			{
				StackNode *current = head;
				std::cout << "Stack = ";
				while (current != NULL)
				{
					std::cout << "[" << current->x << ", " << current->y << "] - ";
					current = current->next;
				}
				std::cout << std::endl;

			}
		};
		
		int entrance_index;

		enum {

			//labyrinth parameters
			map_size = 40,
			alignment_top = 4,
			alignment_bottom = alignment_top,
			alignment_left = alignment_top,
			alignment_right = alignment_top,

			border_width = 1,

			step = 9,

			cells_number = (map_size - 2 * alignment_top) / step,
		};



		//labyrinth generator

		bool visited[cells_number][cells_number];
		Cell cells[cells_number][cells_number];
		Stack *labyrinth_stack;


		//Recursive backtracker algorithm: https://en.wikipedia.org/wiki/Maze_generation_algorithm
		void draw_labyrinth(int start_index)
		{
			//matrix with elements, which indicates whether the cell was visited by algorithm
			for (int i = 0; i < cells_number; i++)
				for (int j = 0; j < cells_number; j++)
					visited[i][j] = false;


			vec2 current_cell(start_index, 0);
			labyrinth_stack = new Stack(start_index, 0);
			visited[start_index][0] = true;
			bool complete = false;

			int it = 0;
			while (!complete)
			{
				vec2 next_cell = find_unvisited_cell((int)current_cell.x(), (int)current_cell.y());
				if (next_cell.x() == -1)
				{
					complete = true;
					printf("Stack is empty.\n");
				}
				else
				{
					current_cell = labyrinth_stack->get_head();

					int x = next_cell.x(), y = next_cell.y();
					labyrinth_stack->push(x, y);
					visited[x][y] = true;

					
					//remove wall
					if (current_cell.x() == next_cell.x())
						if (current_cell.y() > next_cell.y())
							cells[(int)current_cell.y()][(int)current_cell.x()].bottom_wall = false;
						else
							cells[(int)current_cell.y()][(int)current_cell.x()].top_wall = false;
					else if (current_cell.x() > next_cell.x())
						cells[(int)current_cell.y()][(int)current_cell.x()].left_wall = false;
					else
						cells[(int)current_cell.y()][(int)current_cell.x()].right_wall = false;

					//		sprites[current_sprite++].init(white, x0 + current_cell.x()*step + step / 2, y0 + current_cell.y()*step, step - net_width, net_width);
					//	else
					//		sprites[current_sprite++].init(white, x0 + current_cell.x()*step + step / 2, y0 + next_cell.y()*step, step - net_width, net_width);
					//else if (current_cell.x() > next_cell.x())
					//	sprites[current_sprite++].init(white, x0 + step * current_cell.x(), y0 + current_cell.y()*step + step / 2, net_width, step - net_width);
					//else
					//	sprites[current_sprite++].init(white, x0 + step*next_cell.x(), y0 + current_cell.y()*step + step / 2, net_width, step - net_width);

					//std::cout << "###################### Wall [" << current_cell.x() << ", " << current_cell.y() << "] -> [" << next_cell.x() << ", " << next_cell.y() << "] removed. ######################" << std::endl;

					it++;
					//if (it == 1)
					//	complete = true;

					labyrinth_stack->showElements();

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
					return (vec2(cur_x - 1, cur_y));
					//top
				case 2:
					return (vec2(cur_x, cur_y + 1));
					//right
				case 3:
					return (vec2(cur_x + 1, cur_y));
					//bottom
				case 4:
					return (vec2(cur_x, cur_y - 1));
				}
			}
			//no unvisited neighbours
			else
			{
				vec2 previous = labyrinth_stack->pop();
				labyrinth_stack->showElements();
				if (previous.x() == -1)
					return previous;
				return(find_unvisited_cell((int)previous.x(), (int)previous.y()));
			}
		}

		bool check_cell(int x, int y)
		{
			if ((x >= 0) && (x < cells_number) && (y >= 0) && (y < cells_number))
			{
				if (visited[x][y])
					return false;
				else
					return true;
			}
			return false;
		}

		void calculate_walls()
		{

		}

		void place_textures(int current_sprite, sprite sprites[], int step)
		{
			GLuint white = resource_dict::get_texture_handle(GL_RGB, "#ffffff");
			GLuint black = resource_dict::get_texture_handle(GL_RGB, "#000000");
			GLuint gray = resource_dict::get_texture_handle(GL_RGB, "#808080");

			int x0 = -map_size + alignment_left,
				x1 = map_size - alignment_right,
				y0 = -map_size + alignment_top,
				y1 = map_size - alignment_bottom;

			float border_width = 1.0f,
				wall_width = 0.5f,
				net_width = 0.35f;

			for (int i = x0; i <= x1; i += step)
				sprites[current_sprite++].init(gray, 0, i, y1 - y0 + net_width, net_width);
			for (int i = y0; i <= y1; i += step)
				sprites[current_sprite++].init(gray, i, 0, net_width, x1 - x0 + net_width);

			sprites[current_sprite++].init(white, y0, 0, net_width, x1 - x0 + net_width);
			sprites[current_sprite++].init(white, y1, 0, net_width, x1 - x0 + net_width);
			sprites[current_sprite++].init(white, 0, x0, x1 - x0 + net_width, net_width);
			sprites[current_sprite++].init(white, 0, x1, x1 - x0 + net_width, net_width);


			sprites[current_sprite++].init(black, x0 + entrance_index*step + step / 2., y0, step - net_width, net_width);

		}

	public:

		Labyrinth()
		{
			for (int i = 0; i < cells_number; i++)
				for (int j = 0; j < cells_number; j++)
				{
					cells[i][j].x = j;
					cells[i][j].y = i;
				}
		}

		Cell Draw(int current_sprite_index, sprite sprites[])
		{
			//randomize entrance
			srand(time(NULL));
			entrance_index = rand() % cells_number;

			calculate_walls();
			place_textures(current_sprite_index, sprites, 9);
			return cells[][];
		}

	};

}