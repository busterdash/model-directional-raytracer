/*==============================================================================================

	smd_model_reader.cpp
	Copyright 2019 Buster Schrader

	This file is part of The Sunlight Machine.

    The Sunlight Machine is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    The Sunlight Machine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with The Sunlight Machine.  If not, see <https://www.gnu.org/licenses/>.
	
==============================================================================================*/

#include "smd_model_reader.hpp"
#include <stdlib.h>

smd_model_reader::smd_model_reader(std::string file)
{
	std::string line;
	bool triangles = false;
	long counter = 0;
	vertex* vbuffer[3];
	short vertex_counter = 0;
	
	fi.open(file.c_str());
	
	if (fi.is_open())
	{
		while (getline(fi, line))
		{
			if (!triangles && line == "triangles")
			{
				triangles = true;
				continue;
			}
		
			if (triangles)
			{
				counter++;
				
				if (counter == 1) //Skip the first material definition.
				{
					continue;	
				}
				
				if (line.length() > 2)
				{
					if (line.substr(0,2) == "0 ")
					{
						vbuffer[vertex_counter] = parse_line(line);
						vertex_counter++;
					}
					else
					{
						triangle* tri = new triangle(vbuffer[0],vbuffer[1],vbuffer[2]);
						tri_list.push_back(tri);
						vertex_counter = 0;
					}
				}
			}
		}
	}
}

vertex* smd_model_reader::parse_line(std::string line)
{
	short data_position = 0;
	float line_data[10];
	std::string buffer = "";
	bool last_char_is_space = false; //Double spaces can't kill me.
	vertex* v;
	
	for (short i = 0; i < line.length(); i++)
	{
		if (line[i] != ' ' && line[i] != '\t')
		{
			buffer += line[i];
			last_char_is_space = false;
		}
		else
		{
			if (last_char_is_space) { continue; }
			line_data[data_position] = strtof(buffer.c_str(),0);
			data_position++;
			last_char_is_space = true;
			buffer = "";
		}
	}
	
	v = new vertex(line_data[1],line_data[2],line_data[3],line_data[4],line_data[5],line_data[6],line_data[7],line_data[8]);
	return v;
}

smd_model_reader::~smd_model_reader()
{
	fi.close();
	tri_list.clear();
}

triangle* smd_model_reader::get_triangle(unsigned int index)
{
	if (index >= tri_list.size())
	{
		return NULL;
	}
	else
	{
		return tri_list[index];
	}
}

float smd_model_reader::get_max_coordinate()
{
	float max = -1.0f;
	
	for (int i = 0; i < tri_list.size(); i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (tri_list[i]->v[j]->x > max)
			{
				max = tri_list[i]->v[j]->x;
			}
			
			if (tri_list[i]->v[j]->y > max)
			{
				max = tri_list[i]->v[j]->y;
			}
			
			if (tri_list[i]->v[j]->z > max)
			{
				max = tri_list[i]->v[j]->z;
			}
		}
	}
	
	return max;
}

unsigned int smd_model_reader::get_triangle_count()
{
	return tri_list.size();
}
