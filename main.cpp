#define GL_LITE_IMPLEMENTATION
#include "gl_lite.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GLFWwindow* window = NULL;
int gl_width = 400;
int gl_height = 300;
float gl_aspect_ratio = (float)gl_width/gl_height;
bool gl_fullscreen = false;

#include "GameMaths.h"
#include "Input.h"
#include "Camera3D.h"
#include "init_gl.h"
#include "load_obj.h"
#include "Shader.h"
#include "DebugDrawing.h"
#include "Player.h"
#include "NavMesh.h"

int main(){
	if(!init_gl(window, "Navmesh", gl_width, gl_height)){ return 1; }

	//Load cube mesh
	GLuint cube_vao;
	unsigned int cube_num_indices = 0;
	{
		float* vp = NULL;
		float* vn = NULL;
		float* vt = NULL;
		uint16_t* indices = NULL;
		unsigned int num_verts = 0;
		load_obj_indexed("cube.obj", &vp, &vt, &vn, &indices, &num_verts, &cube_num_indices, false);

		glGenVertexArrays(1, &cube_vao);
		glBindVertexArray(cube_vao);
		
		GLuint points_vbo;
		glGenBuffers(1, &points_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_verts*3*sizeof(float), vp, GL_STATIC_DRAW);
		glEnableVertexAttribArray(VP_ATTRIB_LOC);
		glVertexAttribPointer(VP_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		free(vp);

		free(vt);

		GLuint normals_vbo;
		glGenBuffers(1, &normals_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_verts*3*sizeof(float), vn, GL_STATIC_DRAW);
		glEnableVertexAttribArray(VN_ATTRIB_LOC);
		glVertexAttribPointer(VN_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		free(vn);

		GLuint index_vbo;
		glGenBuffers(1, &index_vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube_num_indices*sizeof(unsigned short), indices, GL_STATIC_DRAW);
		free(indices);
	}

	//Load ground mesh
	GLuint ground_vao;
	unsigned int ground_num_indices = 0;
	NavMesh nav_mesh;
	{
		float* vp = NULL;
		float* vn = NULL;
		float* vt = NULL;
		uint16_t* indices = NULL;
		unsigned int num_verts = 0;
		load_obj_indexed("ground.obj", &vp, &vt, &vn, &indices, &num_verts, &ground_num_indices);

		nav_mesh = init_navmesh(vp, indices, num_verts, ground_num_indices);

		glGenVertexArrays(1, &ground_vao);
		glBindVertexArray(ground_vao);
		
		GLuint points_vbo;
		glGenBuffers(1, &points_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_verts*3*sizeof(float), vp, GL_STATIC_DRAW);
		glEnableVertexAttribArray(VP_ATTRIB_LOC);
		glBindBuffer(GL_ARRAY_BUFFER, points_vbo);
		glVertexAttribPointer(VP_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		// free(vp);

		free(vt);

		GLuint normals_vbo;
		glGenBuffers(1, &normals_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, normals_vbo);
		glBufferData(GL_ARRAY_BUFFER, num_verts*3*sizeof(float), vn, GL_STATIC_DRAW);
		glEnableVertexAttribArray(VN_ATTRIB_LOC);
		glVertexAttribPointer(VN_ATTRIB_LOC, 3, GL_FLOAT, GL_FALSE, 0, NULL);
		free(vn);

		GLuint index_vbo;
		glGenBuffers(1, &index_vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_vbo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, ground_num_indices*sizeof(unsigned short), indices, GL_STATIC_DRAW);
		free(indices);
	}

    g_camera.init(vec3(0,2,35), vec3(0,-5,0));

	init_debug_draw();

    //Load shaders
	Shader basic_shader = init_shader("MVP.vert", "uniform_colour_sunlight.frag");
	GLuint colour_loc = glGetUniformLocation(basic_shader.id, "colour");
	glUseProgram(basic_shader.id);
	glUniformMatrix4fv(basic_shader.V_loc, 1, GL_FALSE, g_camera.V.m);
	glUniformMatrix4fv(basic_shader.P_loc, 1, GL_FALSE, g_camera.P.m);

	check_gl_error();

    double curr_time = glfwGetTime(), prev_time, dt;
	//-------------------------------------------------------------------------------------//
	//-------------------------------------MAIN LOOP---------------------------------------//
	//-------------------------------------------------------------------------------------//
	while(!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Get dt
		prev_time = curr_time;
		curr_time = glfwGetTime();
		dt = curr_time - prev_time;
		if(dt > 0.1) dt = 0.1;

		//Get Input
		g_mouse.prev_xpos = g_mouse.xpos;
    	g_mouse.prev_ypos = g_mouse.ypos;
		glfwPollEvents();
		
		if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
			glfwSetWindowShouldClose(window, 1);
			continue;
		}

		//Tab to toggle player_cam/freecam
		static bool freecam_mode = true;
		static bool tab_was_pressed = false;
		if(glfwGetKey(window, GLFW_KEY_TAB)) {
			if(!tab_was_pressed) { freecam_mode = !freecam_mode; }
			tab_was_pressed = true;
		}
		else tab_was_pressed = false;

		//Slash to toggle ground wireframe
		static bool draw_wireframe = true;
		static bool slash_was_pressed = false;
		if(glfwGetKey(window, GLFW_KEY_SLASH)) {
			if(!slash_was_pressed) { draw_wireframe = !draw_wireframe; }
			slash_was_pressed = true;
		}
		else slash_was_pressed = false;

		//Ctrl/Command-F to toggle fullscreen
		//Note: window_resize_callback takes care of resizing viewport/recalculating P matrix
		static bool F_was_pressed = false;
		if(glfwGetKey(window, GLFW_KEY_F)) {
			if(!F_was_pressed){
				if(glfwGetKey(window, CTRL_KEY_LEFT) || glfwGetKey(window, CTRL_KEY_RIGHT)){
					gl_fullscreen = !gl_fullscreen;
					static int old_win_x, old_win_y, old_win_w, old_win_h;
					if(gl_fullscreen){
						glfwGetWindowPos(window, &old_win_x, &old_win_y);
						glfwGetWindowSize(window, &old_win_w, &old_win_h);
						GLFWmonitor* mon = glfwGetPrimaryMonitor();
						const GLFWvidmode* vidMode = glfwGetVideoMode(mon);
						glfwSetWindowMonitor(window, mon, 0, 0, vidMode->width, vidMode->height, vidMode->refreshRate);
					}
					else glfwSetWindowMonitor(window, NULL, old_win_x, old_win_y, old_win_w, old_win_h, GLFW_DONT_CARE);
				}
			}
			F_was_pressed = true;
		}
		else F_was_pressed = false;

		//Move player
		if(!freecam_mode) player_update(dt);

		//Do collision with navmesh
		{
			static int nav_idx = find_closest_face_SLOW(nav_mesh, player_pos);
			
			find_face_below_pos(nav_mesh, player_pos, &nav_idx);

			//Handle wall collision
			// int wall_idx = find_wall(nav_mesh, player_pos, nav_idx);
			// if(wall_idx>=0){
			// 	vec3 wall_vp[3];
			// 	get_face(nav_mesh, wall_idx, &wall_vp[0], &wall_vp[1], &wall_vp[2]);
			// 	vec3 wall_norm = normalise(cross(wall_vp[1]-wall_vp[0], wall_vp[2]-wall_vp[0]));
			// 	vec3 vec_to_wall = player_pos-wall_vp[0] * dot(player_pos-wall_vp[0], wall_norm);
			// 	if(length(vec_to_wall)<player_scale.x){
			// 		printf("HIT WALL\n");
			// 	}
			// }

			//Get height of ground directly below player
			vec3 ground_face_vp[3];
			get_face(nav_mesh, nav_idx, &ground_face_vp[0], &ground_face_vp[1], &ground_face_vp[2]);
			float ground_y;
			{
				//Project everything onto xz for barycentric interp
				vec3 a = vec3(ground_face_vp[0].x, 0, ground_face_vp[0].z);
				vec3 b = vec3(ground_face_vp[1].x, 0, ground_face_vp[1].z);
				vec3 c = vec3(ground_face_vp[2].x, 0, ground_face_vp[2].z);

				float double_area_abc = length(cross(b-a, c-a));
				vec3 player_xz = vec3(player_pos.x, 0, player_pos.z);

				//Get barycentric coords u,v,w
				float double_area_pbc = length(cross(b-player_xz, c-player_xz));
				float double_area_pca = length(cross(c-player_xz, a-player_xz));
				float double_area_pab = length(cross(a-player_xz, b-player_xz));
				float u = double_area_pbc/double_area_abc;
				float v = double_area_pca/double_area_abc;
				float w = double_area_pab/double_area_abc;

				ground_y = u*ground_face_vp[0].y +v*ground_face_vp[1].y + w*ground_face_vp[2].y;
				const float bary_epsilon = 0.01f;
				if(u+v+w > 1 + bary_epsilon) ground_y = player_pos.y - 1;
			}

			//Get minimum translation vector from ground to player (along ground norm)
			vec3 ground_mtv;
			{
				vec3 ground_norm = normalise(cross(ground_face_vp[1]-ground_face_vp[0], ground_face_vp[2]-ground_face_vp[0]));
				vec3 vec_to_player = player_pos-ground_face_vp[0];
				ground_mtv = ground_norm * dot(vec_to_player,ground_norm);
				draw_vec(player_pos-ground_mtv, ground_mtv, vec4(0.6,0.2,0.5,1));
			}

			//Check player collision with ground
			{
				const float player_autosnap_height = 0.25f;
				if(player_is_on_ground){
					if(player_pos.y > ground_y) {
						if(player_pos.y-ground_y<player_autosnap_height){
							player_pos.y = ground_y;
						}
						else player_is_on_ground = false;
					}
				}
				if(player_pos.y < ground_y) {
					//push player out of ground along normal
					player_pos -= ground_mtv;
					player_vel.y = 0.0f;
					player_is_on_ground = true;
					player_is_jumping = false;
				}
			}

			if(player_pos.y<-20){
				player_pos = vec3(0,1,0);
			}
		}

		if(freecam_mode)g_camera.update_debug(dt);
		else g_camera.update_player(player_pos, dt);

		glUseProgram(basic_shader.id);
		glUniformMatrix4fv(basic_shader.V_loc, 1, GL_FALSE, g_camera.V.m);
		glUniformMatrix4fv(basic_shader.P_loc, 1, GL_FALSE, g_camera.P.m);

		//Draw player
		glBindVertexArray(cube_vao);
		glUniform4fv(colour_loc, 1, player_colour.v);
		glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, player_M.m);
        glDrawElements(GL_TRIANGLES, cube_num_indices, GL_UNSIGNED_SHORT, 0);

		//Draw ground
		glBindVertexArray(ground_vao);
		glUniform4fv(colour_loc, 1, vec4(0.6,0.7,0.8,1).v);
		glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, identity_mat4().m);
        glDrawElements(GL_TRIANGLES, ground_num_indices, GL_UNSIGNED_SHORT, 0);

		if(draw_wireframe){
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glUniform4fv(colour_loc, 1, vec4(0,0,0,1).v);
			glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, translate(identity_mat4(), vec3(0,0.1f,0)).m);
			glDrawElements(GL_TRIANGLES, ground_num_indices, GL_UNSIGNED_SHORT, 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glfwSwapBuffers(window);

		check_gl_error();
	}//end main loop

    return 0;
}
