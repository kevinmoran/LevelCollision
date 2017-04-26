#pragma once

struct LevelCollider {
	float* verts;
	uint16_t* indices;
	uint16_t num_faces;
};

//Create a LevelCollider object from vertex data; generates a list of triangles which store their neighbours
LevelCollider init_level(float* vp, uint16_t* indices, uint32_t vert_count, uint32_t index_count){
    LevelCollider level;
    
    level.verts = vp;
    level.indices = indices;
    level.num_faces = index_count/3;

    return level;
}

void get_face(const LevelCollider &level, int index, vec3* p0, vec3* p1, vec3* p2){

    uint16_t idx0 = level.indices[3*index];
    *p0 = vec3(level.verts[3*idx0], level.verts[3*idx0+1], level.verts[3*idx0+2]);

    uint16_t idx1 = level.indices[3*index+1];
    *p1 = vec3(level.verts[3*idx1], level.verts[3*idx1+1], level.verts[3*idx1+2]);

    uint16_t idx2 = level.indices[3*index+2];
    *p2 = vec3(level.verts[3*idx2], level.verts[3*idx2+1], level.verts[3*idx2+2]);
}

//Returns vector result from point p to closest point on triangle abc
//Returns true if p's projection onto the abc's plane lies within the triangle
bool get_vec_to_triangle(vec3 p, vec3 a, vec3 b, vec3 c, vec3* result){
    vec3 norm = cross(b-a, c-a);
    float double_area_abc = length(norm);
    norm = normalise(norm);
    float d = dot(p-a,norm);
    vec3 proj = p-(norm*d);

    //Get barycentric coords u,v,w
    vec3 u_cross = cross(b-proj,c-proj);
    vec3 v_cross = cross(c-proj,a-proj);
    vec3 w_cross = cross(a-proj,b-proj);
    float u = length(u_cross)/double_area_abc;
    float v = length(v_cross)/double_area_abc;
    float w = length(w_cross)/double_area_abc;
    if(dot(u_cross,norm)<0) u*= -1;
    if(dot(v_cross,norm)<0) v*= -1;
    if(dot(w_cross,norm)<0) w*= -1;

    if(u>0 && v>0 && w>0){//inside triangle
        // draw_vec(proj, p-proj+vec3(0,0,0.1), vec4(0,0.8,0,1));
        *result = p-proj;
        return true;
    }
    if(u<0){ //In front of bc
        float len_cb = length(c-b);
        float dot_factor = dot(proj-b, (c-b)/len_cb)/len_cb;
        dot_factor = CLAMP(dot_factor,0,1);
        // draw_vec(b+(c-b)*dot_factor, p-(b+(c-b)*dot_factor)+vec3(0,0,0.1), vec4(0,0.8,0,1));
        *result = p-(b+(c-b)*dot_factor);
        return false;
    }
    if(v<0){ //In front of ac
        float len_ca = length(c-a);
        float dot_factor = dot(proj-a, (c-a)/len_ca)/len_ca;
        dot_factor = CLAMP(dot_factor,0,1);
        // draw_vec(a+(c-a)*dot_factor, p-(a+(c-a)*dot_factor)+vec3(0,0,0.1), vec4(0,0.8,0,1));
        *result = p-(a+(c-a)*dot_factor);
        return false;
    }
    if(w<0){ //In front of ab
        float len_ba = length(b-a);
        float dot_factor = dot(proj-a, (b-a)/len_ba)/len_ba;
        dot_factor = CLAMP(dot_factor,0,1);
        // draw_vec(a+(b-a)*dot_factor, p-(a+(b-a)*dot_factor)+vec3(0,0,0.1), vec4(0,0.8,0,1));
        *result = p-(a+(b-a)*dot_factor);
        return false;
    }
    return false;
}

void collide_player_ground(const LevelCollider &level, Capsule* player_collider) {
    bool hit_ground = false;

    //Calculate bounding sphere for player
    float player_sphere_radius = (player_collider->y_base + player_collider->y_cap)/2;
    vec3 player_sphere_center = player_collider->pos + player_collider->matRS*vec3(0,player_sphere_radius,0);

    for(int i=0; i<level.num_faces; i++){
        //Get current face
        vec3 level_face_a, level_face_b, level_face_c;
        get_face(level, i, &level_face_a, &level_face_b, &level_face_c);

        //Broad phase
        //Get face's bounding sphere
        vec3 face_sphere_center = (level_face_a+level_face_b+level_face_c)/3;
        float face_sphere_radius = length(level_face_a-face_sphere_center);

        //Sphere intersection test
        vec3 player_to_face_vec = player_sphere_center-face_sphere_center;
        if(length(player_to_face_vec)>face_sphere_radius + player_sphere_radius) continue;

        //Narrow phase, using GJK
        vec3 level_face_norm = normalise(cross(level_face_b-level_face_a, level_face_c-level_face_a));
        vec3 support_point = player_collider->support(-level_face_norm);
        float player_dist_along_norm = dot(support_point-level_face_a,level_face_norm);
        vec3 ground_to_player_vec = level_face_norm*player_dist_along_norm;

        TriangleCollider triangle_collider;
        triangle_collider.pos = face_sphere_center;
        triangle_collider.points[0] = level_face_a;
        triangle_collider.points[1] = level_face_b;
        triangle_collider.points[2] = level_face_c;
        triangle_collider.normal = level_face_norm;

        //Check intersection
        if(!gjk(player_collider, &triangle_collider)) continue;

        bool face_is_ground = true;
        float face_slope = RAD2DEG(acos(dot(level_face_norm, vec3(0,1,0))));
        if(face_slope>player_max_stand_slope) face_is_ground = false;

        player_collider->pos -= ground_to_player_vec;

        //Check if it's a ground face
        if(face_is_ground){
            if(!player_is_on_ground) player_vel.y = 0.0f; //only kill y velocity if falling
            hit_ground = true;
        }
    }
    //If we hit any ground faces, player is on ground
    player_is_on_ground = hit_ground;
    if(hit_ground){ 
        player_is_jumping = false;
    }
}

void clear_level(LevelCollider* level){
    free(level->verts);
    free(level->indices);
    level->num_faces = -1;
}
