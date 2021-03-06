#include <cstdlib>
#include <ctime>
#include "game_level.h"
#pragma once

namespace octet {

	class Cell {
	public:
		//cell location in the maze from (0,0) to (lab.size, lab.size)
		int x;
		int y;
		//walls indicators
		bool left_wall;
		bool top_wall;
		bool right_wall;
		bool bottom_wall;

		//the distance (measured in cells) from the last crossroads [to indicate deadends]
		int distance;
		//loot stored in the cell
		Loot loot = none;

		int sprite_index;

		//blinking (for stairs)
		int blinking_time;

		//bouncing
		bool bouncing;
		float bounce_max_value;
		float current_bounce_position;
		//top or bottom
		bool direction;
		Cell() 
		{

		}

		void clear()
		{
			top_wall = true;
			bottom_wall = true;
			right_wall = true;
			left_wall = true;
			distance = 0;
			blinking_time = 0;
			loot = none;
			sprite_index = -1;
			bouncing = false;
		}

		Cell(int X, int Y)
		{
			x = X; y = Y;
		}

		bool operator<(const Cell &rhs) const { return distance < rhs.distance; }
	};

	class Labyrinth {

		//stack element
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
		
		//stack, that contains lab cells
		class Stack
		{
		private:
			StackNode *head;
			int count = 1;
		public:
			vec2 distant_cell;
			int path_length = 1;
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
				if (count > path_length)
				{
					path_length = count;
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

		};
public:
		enum {
			absolute_size = 120,
			cell_size = 10,

			cells_number = absolute_size / cell_size,
			half_size = absolute_size/2,
			half_cell = cell_size/2,
		};

		int entrance_index;
		int path_length;
		int hall_width = 2;
		Cell cells[cells_number][cells_number];

private:
		bool visited[cells_number][cells_number];
		Stack *labyrinth_stack;
		int return_path_length;
		vec2 dead_end;

		//Recursive backtracker algorithm: https://en.wikipedia.org/wiki/Maze_generation_algorithm
		void construct_walls()
		{
			//matrix with elements, which indicates whether the cell was visited by algorithm
			for (int i = 0; i < cells_number; i++)
				for (int j = 0; j < cells_number; j++)
				{
					visited[i][j] = false;
					cells[i][j].clear();
				}

			return_path_length = 0;

			vec2 current_cell(entrance_index, 0);
			labyrinth_stack = new Stack(entrance_index, 0);
			visited[entrance_index][0] = true;
			bool complete = false;
			int it = 0;

			cells[0][entrance_index].bottom_wall = false;

			while (!complete)
			{
				vec2 next_cell = find_unvisited_cell((int)current_cell.x(), (int)current_cell.y());
				if (next_cell.x() == -1)
					complete = true;
				else
				{
					current_cell = labyrinth_stack->get_head();
					if (return_path_length > 1)
					{
						cells[(int)dead_end.y()][(int)dead_end.x()].distance = return_path_length;
						return_path_length = 0;
					}
					int x = next_cell.x(), y = next_cell.y();
					labyrinth_stack->push(x, y);
					visited[x][y] = true;

					
					//remove wall
					if (current_cell.x() == next_cell.x())
						if (current_cell.y() > next_cell.y())
						{
							//remove bottom wall
							cells[(int)current_cell.y()][(int)current_cell.x()].bottom_wall = false;
							cells[(int)next_cell.y()][(int)current_cell.x()].top_wall = false;
						}
						else
						{
							//remove top wall
							cells[(int)current_cell.y()][(int)current_cell.x()].top_wall = false;
							cells[(int)next_cell.y()][(int)current_cell.x()].bottom_wall = false;
						}
					else if (current_cell.x() > next_cell.x())
					{
						//remove left wall
						cells[(int)current_cell.y()][(int)current_cell.x()].left_wall = false;
						cells[(int)current_cell.y()][(int)next_cell.x()].right_wall = false;
					}
					else
					{
						//remove right wall
						cells[(int)current_cell.y()][(int)current_cell.x()].right_wall = false;
						cells[(int)current_cell.y()][(int)next_cell.x()].left_wall = false;
					}
					current_cell = next_cell;
				}
			}
			path_length = labyrinth_stack->path_length;
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
				//if it was the furthest cell at the moment - save it
				if (return_path_length == 0)
					dead_end = labyrinth_stack->get_head();
				//increment the length of the go back path
				return_path_length++;
				//go to the previous cell
				vec2 previous = labyrinth_stack->pop();
				if (previous.x() == -1)
					return previous;
				return(find_unvisited_cell((int)previous.x(), (int)previous.y()));
			}
		}

		//check if the cell was visited by the algorithm
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

		void construct_labyrinth()
		{
			srand(time(NULL));
			entrance_index = rand() % cells_number;
			
			construct_walls();

			delete labyrinth_stack;
		}
	};

}