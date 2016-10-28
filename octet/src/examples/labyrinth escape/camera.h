#pragma once
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

//		float min_distance;
		bool moving;
		int cell_size;
		int lab_size;

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
				modifier = 10 * (current - 0.1);

			current_speed.x() = current_speed.y() = std::min((2.f*z) / (modifier * 30), z / 4);
		}

	public:
		float min_distance;
		float init_distance;

		void init(int Cell_size, int Lab_size)
		{
			// set up the matrices with a camera 5 units from the origin
			cameraToWorld.loadIdentity();
			
			cell_size = Cell_size;
			lab_size = Lab_size;

			//set the minimum and maximum Z distance for camera
			min_distance = lab_size*0.2f;
			init_distance = lab_size*0.5;

			x = 0;
			y = 0;
			z = init_distance;

			cameraToWorld.translate(x, y, z);
			initial_position = vec3(x, y, z);

			path_remaining = vec3(0, 0, 0);
			basic_speed = vec3(cell_size/120.f, cell_size / 120.f, cell_size / 240.f);
			current_speed = basic_speed;
			moving = false;
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
			translate(vec3(x1 - x, y1 - y, z1 - z));
		}
	};
}
