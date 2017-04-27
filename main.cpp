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
#include "GJK.h"
#include "Level.h"

int main(){
	if(!init_gl(window, "Level Collision", gl_width, gl_height)){ return 1; }

	//Load player mesh
	GLuint player_vao;
	unsigned int player_num_indices = 0;
	{
		float* vp = NULL;
		float* vn = NULL;
		float* vt = NULL;
		uint16_t* indices = NULL;
		unsigned int num_verts = 0;
		load_obj_indexed("capsule.obj", &vp, &vt, &vn, &indices, &num_verts, &player_num_indices);

		glGenVertexArrays(1, &player_vao);
		glBindVertexArray(player_vao);
		
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
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, player_num_indices*sizeof(unsigned short), indices, GL_STATIC_DRAW);
		free(indices);
	}

	//Load ground mesh
	GLuint ground_vao;
	unsigned int ground_num_indices = 0;
	LevelCollider level;
	{
		float* vp = NULL;
		float* vn = NULL;
		float* vt = NULL;
		uint16_t* indices = NULL;
		unsigned int num_verts = 0;
		load_obj_indexed("ground.obj", &vp, &vt, &vn, &indices, &num_verts, &ground_num_indices);

		level = init_level(vp, indices, num_verts, ground_num_indices);

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
		// free(indices);
	}

	//Player collision mesh
	Capsule player_collider;
	{
		player_collider.r = 1; 		//NB: these are the dimensions of the collider mesh (capsule.obj),
		player_collider.y_base = 1; //they will be scaled using the player's model matrix!
		player_collider.y_cap = 2;
		player_collider.pos = player_pos;
		player_collider.matRS = player_M;
		player_collider.matRS_inverse = inverse(player_M);
	}

	g_camera.init(vec3(0,2,35), vec3(0,-5,0));
	
	init_debug_draw();

    //Load shaders
	Shader basic_shader = init_shader("MVP.vert", "uniform_colour_sunlight.frag");
	GLuint colour_loc = glGetUniformLocation(basic_shader.id, "colour");

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
		
		//Check button presses
		static bool freecam_mode = true;
		static bool draw_wireframe = true;
		{
			if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
				glfwSetWindowShouldClose(window, 1);
			}

			//Tab to toggle player_cam/freecam
			static bool tab_was_pressed = false;
			if(glfwGetKey(window, GLFW_KEY_TAB)) {
				if(!tab_was_pressed) { freecam_mode = !freecam_mode; }
				tab_was_pressed = true;
			}
			else tab_was_pressed = false;

			//Slash to toggle ground wireframe
			static bool slash_was_pressed = false;
			if(glfwGetKey(window, GLFW_KEY_SLASH)) {
				if(!slash_was_pressed) { draw_wireframe = !draw_wireframe; }
				slash_was_pressed = true;
			}
			else slash_was_pressed = false;

			//R to reset
			static bool r_was_pressed = false;
			if(glfwGetKey(window, GLFW_KEY_R)) {
				if(!r_was_pressed) {
					player_pos = vec3(0,2,0);
					player_vel = vec3(0,0,0);
				 }
				r_was_pressed = true;
			}
			else r_was_pressed = false;

			//T to teleport player
			static bool t_was_pressed = false;
			if(glfwGetKey(window, GLFW_KEY_T)) {
				if(!t_was_pressed) {
					player_pos = g_camera.pos + g_camera.fwd*5 - g_camera.up*2;
					player_vel = vec3(0,0,0);
				 }
				t_was_pressed = true;
			}
			else t_was_pressed = false;

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
		}

		//Move player
		if(!freecam_mode) player_update(dt);
		player_collider.pos = player_pos;
		player_collider.matRS = player_M;
		player_collider.matRS_inverse = inverse(player_M);

		//Do collision with ground
		collide_player_ground(level, &player_collider);
		player_pos = player_collider.pos;
		player_M = translate(scale(identity_mat4(), player_scale), player_pos);

		//Update camera
		if(freecam_mode)g_camera.update_debug(dt);
		else g_camera.update_player(player_pos, dt);

		glUseProgram(basic_shader.id);
		glUniformMatrix4fv(basic_shader.V_loc, 1, GL_FALSE, g_camera.V.m);
		glUniformMatrix4fv(basic_shader.P_loc, 1, GL_FALSE, g_camera.P.m);

		//Draw player
		glBindVertexArray(player_vao);
		glUniform4fv(colour_loc, 1, player_colour.v);
		glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, player_M.m);
        glDrawElements(GL_TRIANGLES, player_num_indices, GL_UNSIGNED_SHORT, 0);

		//Draw ground
		glBindVertexArray(ground_vao);
		glUniform4fv(colour_loc, 1, vec4(0.6,0.7,0.8,1).v);
		glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, identity_mat4().m);
        glDrawElements(GL_TRIANGLES, ground_num_indices, GL_UNSIGNED_SHORT, 0);

		if(draw_wireframe){
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glUseProgram(debug_shader.id);
			glUniform4fv(colour_loc, 1, vec4(0,0,0,1).v);
			glUniformMatrix4fv(basic_shader.M_loc, 1, GL_FALSE, identity_mat4().m);
			glDrawElements(GL_TRIANGLES, ground_num_indices, GL_UNSIGNED_SHORT, 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		glfwSwapBuffers(window);

		check_gl_error();
	}//end main loop

    return 0;
}
