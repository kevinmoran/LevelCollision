#pragma once

#define NAV_MESH_MAX_NEIGHBOURS 16
struct LevelColliderTri{
	uint16_t indices[3];
	uint16_t num_neighbours;
	uint16_t neighbours[NAV_MESH_MAX_NEIGHBOURS];
};

struct LevelCollider {
	float* verts;
	LevelColliderTri* faces;
	uint16_t num_faces;
};

//Create a LevelCollider object from vertex data; generates a list of triangles which store their neighbours
LevelCollider init_level(float* vp, uint16_t* indices, uint32_t vert_count, uint32_t index_count){
    LevelCollider level;
    
    level.verts = vp;
    level.num_faces = index_count/3;
    level.faces = (LevelColliderTri*)calloc(level.num_faces, sizeof(LevelColliderTri));

    for(int i=0; i<level.num_faces; i++){
        level.faces[i].indices[0] = indices[3*i];
        level.faces[i].indices[1] = indices[3*i+1];
        level.faces[i].indices[2] = indices[3*i+2];
    }

    //Generate connectivity information (It's O(n^2) but no better ideas. Do this offline!)
    for(int i=0; i<level.num_faces; i++){
        //Check all faces further in the list
        for(int j = i+1; j<level.num_faces; j++){
            //If faces i and j share any verts, they're neighbours
            int num_equal_verts = 0;

            for(int k=0; k<3; k++){
                if((level.faces[i].indices[0]==level.faces[j].indices[k]) || 
                   (level.faces[i].indices[1]==level.faces[j].indices[k]) || 
                   (level.faces[i].indices[2]==level.faces[j].indices[k]) )
                {
                    num_equal_verts+=1;
                }
            }
            if(num_equal_verts>0){
                assert(level.faces[i].num_neighbours<NAV_MESH_MAX_NEIGHBOURS);
                level.faces[i].neighbours[level.faces[i].num_neighbours] = j;
                level.faces[i].num_neighbours += 1;

                assert(level.faces[j].num_neighbours<NAV_MESH_MAX_NEIGHBOURS);
                level.faces[j].neighbours[level.faces[j].num_neighbours] = i;
                level.faces[j].num_neighbours += 1;
            }
            if(num_equal_verts==3){
                printf("LevelCollider init warning: Mesh contains duplicate triangle!\n");
            }
        }
    }
    return level;
}

void get_face(const LevelCollider &level, int index, vec3* p0, vec3* p1, vec3* p2){
    LevelColliderTri face_i = level.faces[index];

    uint16_t idx0 = face_i.indices[0];
    *p0 = vec3(level.verts[3*idx0], level.verts[3*idx0+1], level.verts[3*idx0+2]);

    uint16_t idx1 = face_i.indices[1];
    *p1 = vec3(level.verts[3*idx1], level.verts[3*idx1+1], level.verts[3*idx1+2]);

    uint16_t idx2 = face_i.indices[2];
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

//Find index of triangle on level closest to pos
//Uses seeded index and connectivity info for speed!
void find_closest_face(const LevelCollider &level, vec3 pos, int* index){
    assert(*index<level.num_faces);

    //Get vector from pos to level face at seeded index
    vec3 vec_to_closest_face;
    {
        vec3 curr_tri[3];
        get_face(level, *index, &curr_tri[0], &curr_tri[1], &curr_tri[2]);
        get_vec_to_triangle(pos, curr_tri[0], curr_tri[1], curr_tri[2], &vec_to_closest_face);
    }

    //Every iteration we jump to a neighbouring face which is closer to pos until none are closer
    //Should realistically need only 1 or 2 iterations since player won't move much per frame
    for(int iterations=0; iterations<32; iterations++)
    {
        vec3 vec_to_closest_neighbour = vec3(999,999,999);
        int closest_neighbour_idx = -1;

        //Check distance to all neighbours of current face, store closest
		for(int i=0; i<level.faces[*index].num_neighbours; i++)
        {
            //Get neighbour face
            vec3 curr_tri[3];
            uint16_t neigh_idx = level.faces[*index].neighbours[i];
            get_face(level, neigh_idx, &curr_tri[0], &curr_tri[1], &curr_tri[2]);

            //Get vector from pos to neighbour
            vec3 vec_to_curr_face;
            get_vec_to_triangle(pos, curr_tri[0], curr_tri[1], curr_tri[2], &vec_to_curr_face);

            //If this neighbour is the closest to pos so far, save it
            if(length2(vec_to_curr_face) < length2(vec_to_closest_neighbour)){
                vec_to_closest_neighbour = vec_to_curr_face;
                closest_neighbour_idx = neigh_idx;
            }
        }
        //If one of the neighbours is closer than current face, make it current face
        const float bias = 0.00001;
        if(length2(vec_to_closest_neighbour)<length2(vec_to_closest_face)-bias){
            *index = closest_neighbour_idx;
            vec_to_closest_face = vec_to_closest_neighbour;
        }
        //Otherwise we can't get closer to pos, return
        else return;
    }
    printf("WARNING: find_closest_face ran out of iterations\n");
}

void collide_player_ground(const LevelCollider &level, Capsule* player_collider, int closest_face_idx) {
    int num_neighbours = level.faces[closest_face_idx].num_neighbours;
    bool hit_ground = false;
    for(int i=-1; i<num_neighbours; i++){
        //Get current face
        vec3 level_face_a, level_face_b, level_face_c;
        {
            int level_idx;
            if(i<0) level_idx = closest_face_idx; //ugly but whatever
            else level_idx = level.faces[closest_face_idx].neighbours[i];
            get_face(level, level_idx, &level_face_a, &level_face_b, &level_face_c);
        }
        vec3 level_face_norm = normalise(cross(level_face_b-level_face_a, level_face_c-level_face_a));

        TriangleCollider triangle_collider;
        triangle_collider.points[0] = level_face_a;
        triangle_collider.points[1] = level_face_b;
        triangle_collider.points[2] = level_face_c;
        triangle_collider.normal = level_face_norm;

        //Check player collision with current face
        vec3 support_point = player_collider->support(-level_face_norm);
        float d = dot(support_point-level_face_a,level_face_norm);
        vec3 ground_to_player_vec = level_face_norm*(d);

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

//Brute force check all triangles in level, return closest one to pos
int find_closest_face_SLOW(const LevelCollider &level, vec3 pos){
    float min_dist = 999;
    int closest_face = -1;
    for(int i=0; i<level.num_faces; i++){
        vec3 curr_tri[3];
        get_face(level, i, &curr_tri[0], &curr_tri[1], &curr_tri[2]);
        vec3 v;
        get_vec_to_triangle(pos, curr_tri[0], curr_tri[1], curr_tri[2], &v);
        float d = length2(v);
        if(d < min_dist){
            min_dist = d;
            closest_face = i;
        }
    }
    return closest_face;
}

void clear_level(LevelCollider* level){
    free(level->verts);
    free(level->faces);
    level->num_faces = -1;
}
