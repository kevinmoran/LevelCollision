#pragma once

#define NAV_MESH_MAX_NEIGHBOURS 16
struct NavMeshTri{
	uint16_t indices[3];
	uint16_t num_neighbours;
	uint16_t neighbours[NAV_MESH_MAX_NEIGHBOURS];
};

struct NavMesh {
	float* verts;
	NavMeshTri* faces;
	uint16_t num_faces;
};

//Create a navmesh from vertex data; generates a list of triangles which store their neighbours
NavMesh init_navmesh(float* vp, uint16_t* indices, uint32_t vert_count, uint32_t index_count){
    NavMesh n;
    
    n.verts = vp;
    n.num_faces = index_count/3;
    n.faces = (NavMeshTri*)calloc(n.num_faces, sizeof(NavMeshTri));

    for(int i=0; i<n.num_faces; i++){
        n.faces[i].indices[0] = indices[3*i];
        n.faces[i].indices[1] = indices[3*i+1];
        n.faces[i].indices[2] = indices[3*i+2];
    }

    //Generate connectivity information (It's O(n^2) but no better ideas. Do this offline!)
    for(int i=0; i<n.num_faces; i++){
        //Check all faces further in the list
        for(int j = i+1; j<n.num_faces; j++){
            //If faces i and j share any verts, they're neighbours
            int num_equal_verts = 0;

            for(int k=0; k<3; k++){
                if(cmpf(n.faces[i].indices[0], n.faces[j].indices[k]) || 
                   cmpf(n.faces[i].indices[1], n.faces[j].indices[k]) || 
                   cmpf(n.faces[i].indices[2], n.faces[j].indices[k]) )
                {
                    num_equal_verts+=1;
                }
            }
            if(num_equal_verts>0){
                assert(n.faces[i].num_neighbours<NAV_MESH_MAX_NEIGHBOURS);
                n.faces[i].neighbours[n.faces[i].num_neighbours] = j;
                n.faces[i].num_neighbours += 1;

                assert(n.faces[j].num_neighbours<NAV_MESH_MAX_NEIGHBOURS);
                n.faces[j].neighbours[n.faces[j].num_neighbours] = i;
                n.faces[j].num_neighbours += 1;
            }
            if(num_equal_verts==3){
                printf("NavMesh init warning: Mesh contains duplicate triangle!\n");
            }
        }
    }
    return n;
}

void get_face(const NavMesh &n, int index, vec3* p0, vec3* p1, vec3* p2){
    NavMeshTri face_i = n.faces[index];

    uint16_t idx0 = face_i.indices[0];
    *p0 = vec3(n.verts[3*idx0], n.verts[3*idx0+1], n.verts[3*idx0+2]);

    uint16_t idx1 = face_i.indices[1];
    *p1 = vec3(n.verts[3*idx1], n.verts[3*idx1+1], n.verts[3*idx1+2]);

    uint16_t idx2 = face_i.indices[2];
    *p2 = vec3(n.verts[3*idx2], n.verts[3*idx2+1], n.verts[3*idx2+2]);
}

//Returns vector from point p to closest point on triangle abc
vec3 get_vec_to_triangle(vec3 p, vec3 a, vec3 b, vec3 c){
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
        return p-proj;
    }
    if(u<0){ //In front of bc
        float len_cb = length(c-b);
        float dot_factor = dot(proj-b, (c-b)/len_cb)/len_cb;
        dot_factor = CLAMP(dot_factor,0,1);
        // draw_vec(b+(c-b)*dot_factor, p-(b+(c-b)*dot_factor)+vec3(0,0,0.1), vec4(0,0.8,0,1));
        return p-(b+(c-b)*dot_factor);
    }
    if(v<0){ //In front of ac
        float len_ca = length(c-a);
        float dot_factor = dot(proj-a, (c-a)/len_ca)/len_ca;
        dot_factor = CLAMP(dot_factor,0,1);
        // draw_vec(a+(c-a)*dot_factor, p-(a+(c-a)*dot_factor)+vec3(0,0,0.1), vec4(0,0.8,0,1));
        return p-(a+(c-a)*dot_factor);
    }
    if(w<0){ //In front of ab
        float len_ba = length(b-a);
        float dot_factor = dot(proj-a, (b-a)/len_ba)/len_ba;
        dot_factor = CLAMP(dot_factor,0,1);
        // draw_vec(a+(b-a)*dot_factor, p-(a+(b-a)*dot_factor)+vec3(0,0,0.1), vec4(0,0.8,0,1));
        return p-(a+(b-a)*dot_factor);
    }
    assert(false);
    return vec3(INFINITY,INFINITY,INFINITY);
}

//Find index of triangle on navmesh below pos
//Uses previous result (index param) and connectivity info for speed!
void find_face_below_pos(const NavMesh &n, vec3 pos, int* index){
    assert(*index<n.num_faces);
    vec3 closest_face;
    {
        vec3 current_tri[3];
        get_face(n, *index, &current_tri[0], &current_tri[1], &current_tri[2]);
        closest_face = get_vec_to_triangle(pos, current_tri[0], current_tri[1], current_tri[2]);
        float curr_dist2 = length2_xz(closest_face);
        if(curr_dist2<0.00001) return;
    }

    for(int iterations=0; iterations<32; iterations++)
    {
        vec3 closest_neighbour = vec3(999,999,999);
        int closest_neighbour_idx = -1;
		for(int i=0; i<n.faces[*index].num_neighbours; i++)
        {
            uint16_t neigh_idx = n.faces[*index].neighbours[i];
            vec3 current_tri[3];
            get_face(n, neigh_idx, &current_tri[0], &current_tri[1], &current_tri[2]);
            vec3 v = get_vec_to_triangle(pos, current_tri[0], current_tri[1], current_tri[2]);
            if(length2_xz(v) < length2_xz(closest_neighbour)){
                closest_neighbour = v;
                closest_neighbour_idx = neigh_idx;
            }
        }
        if(length2_xz(closest_neighbour)<length2_xz(closest_face)){
            *index = closest_neighbour_idx;
            closest_face = closest_neighbour;
            if(length2_xz(closest_neighbour)<0.00001) return;
        }
        else return;
    }
}

//Brute force check all triangles in navmesh, return closest one to pos
int find_closest_face_SLOW(const NavMesh &n, vec3 pos){
    float min_dist = 999;
    int closest_face = -1;
    for(int i=0; i< n.num_faces; i++){
        vec3 current_tri[3];
        get_face(n, i, &current_tri[0], &current_tri[1], &current_tri[2]);
        vec3 v = get_vec_to_triangle(pos, current_tri[0], current_tri[1], current_tri[2]);
        float d = length2(v);
        if(d < min_dist){
            min_dist = d;
            closest_face = i;
        }
    }
    return closest_face;
}

void clear_navmesh(NavMesh* n){
    free(n->verts);
    free(n->faces);
    n->num_faces = -1;
}
