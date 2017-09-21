#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//****************************************
//Kevin's wavefront obj loading functions
//****************************************

//----------------------------------------------------------------------------------------------------------------------
//Load unindexed meshes (with/without UVs and normals)
bool load_obj(const char* file_name, 
			  float**     vp, 
			  uint32_t*   vert_count
			  );
bool load_obj(const char* file_name, 
			  float**     vp, 
			  float**     vt, 
			  float**     vn, 
			  uint32_t*   vert_count
			  );

//Load indexed meshes (with/without UVs and normals)
bool load_obj_indexed(const char* file_name, 
					  float**     vp, 
					  uint16_t**  indices, 
					  uint32_t*   vert_count, 
					  uint32_t*   index_count
					  );
bool load_obj_indexed(const char* file_name, 
					  float**     vp, 
					  float**     vt, 
					  float**     vn, 
					  uint16_t**  indices, 
					  uint32_t*   vert_count, 
					  uint32_t*   index_count, 
					  float       smooth_normal_factor=0.5 // (from 0-1) factor to decide if 2 vertices with different 
					  );                                   // normals should be treated as the same vert with an averaged
					                                       // norm or as two separate vertices;
                                                           // if dot(n1, n2) > factor then v1 and v2 are the same
					                                       // i.e. factor=cos(theta) means smooth normals if angle between faces is > theta
		                                                   // 0 is always smooth normals, 1 is never smooth normals
//----------------------------------------------------------------------------------------------------------------------

#define OBJ_PATH "Meshes/"
#define OBJLOAD_LINE_SIZE 256

//Load unindexed vertex positions (i.e. returns a triangulated points array), ignore tex coords and normals if present
bool load_obj(const char* file_name, float** vp, uint32_t* vert_count){
	char obj_file_path[64];
    sprintf(obj_file_path, "%s%s", OBJ_PATH, file_name);
	FILE* fp = fopen(obj_file_path, "r");
	if(!fp) {
		printf("Error: Failed to open %s\n", file_name);
		fclose(fp);
		return false;
	}
	printf("Loading obj: '%s'\n", file_name);

	uint32_t num_vps = 0;
	uint32_t num_vts = 0;
	uint32_t num_vns = 0;
	uint32_t num_faces = 0;

	char line[OBJLOAD_LINE_SIZE];
	line[0] = '\0';

	//Count number of elements
	while(fgets(line, OBJLOAD_LINE_SIZE, fp)){
		if(line[0]=='v'){
			if(line[1]==' ') num_vps++;
			else if(line[1]=='t') num_vts++;
			else if(line[1]=='n') num_vns++;
		}
		else if(line[0]=='f') num_faces++;
	}
	rewind(fp);

	// printf("%u vps, ", num_vps);
	// printf("%u vts, ", num_vts);
	// printf("%u vns, ", num_vns);
	// printf("%u faces ", num_faces);

	*vert_count = 3*num_faces;
	float* vp_unsorted = (float*)malloc(num_vps*3*sizeof(float)); //temp
	*vp = (float*)malloc(*vert_count*3*sizeof(float));
	uint32_t mem_alloced = (uint32_t)(*vert_count*3*sizeof(float));
	printf("(Allocated %u bytes)\n", mem_alloced);

	//Iterators
	uint32_t unsort_vp_it = 0; //for unsorted vp
	uint32_t vp_it = 0; //for sorted vp

	while(fgets(line, OBJLOAD_LINE_SIZE, fp)){
		if(line[0]=='v' && line[1]==' '){
			int ret = sscanf(line, "v %f %f %f", &vp_unsorted[3*unsort_vp_it], &vp_unsorted[3*unsort_vp_it+1], &vp_unsorted[3*unsort_vp_it+2]);
			if(ret!=3){
				printf("ERROR: Wrong vertex position layout \n");
				printf("Expected format: v x y z \n");
				printf("Observed format: %s\n", line);
				return false;
			}
			unsort_vp_it+=1;
		}
		else if(line[0]=='f'){
			uint16_t indices[3];
			//Scan the line depending on what parameters are included for faces
			if(num_vts==0 && num_vns==0){ //Just vertex positions
				int ret = sscanf(line, "f %hu %hu %hu", &indices[0], &indices[1], &indices[2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v v v \n");
					printf("Observed format: %s\n", line);
					return false;
				}
			}
			else if(num_vts==0){ //vertex positions and normals
				int ret = sscanf(line, "f %hu//%*u %hu//%*u %hu//%*u", &indices[0], &indices[1], &indices[2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v//n v//n v//n \n");
					printf("Observed format: %s\n", line);
					return false;
				}
			}
			else if(num_vns==0){ //vertex positions and tex coords
				int ret = sscanf(line, "f %hu/%*u %hu/%*u %hu/%*u", &indices[0], &indices[1], &indices[2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v/t v/t v/t \n");
					printf("Observed format: %s\n", line);
					return false;
				}
			}
			else{ //vertex positions and tex coords and normals
				int ret = sscanf(line, "f %hu/%*u/%*u %hu/%*u/%*u %hu/%*u/%*u", &indices[0], &indices[1], &indices[2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v/t/n v/t/n v/t/n \n");
					printf("Observed format: %s\n", line);
					return false;
				}
			}
			for(int i=0; i<3; ++i){
				indices[i]-=1; //wavefront obj doesn't use zero indexing
				(*vp)[3*vp_it  ] = vp_unsorted[3*indices[i]];   //x
				(*vp)[3*vp_it+1] = vp_unsorted[3*indices[i]+1]; //y
				(*vp)[3*vp_it+2] = vp_unsorted[3*indices[i]+2]; //z
				vp_it+=1;
			}
		}//end elseif for 'f'
	}//endwhile

	fclose(fp);
	free(vp_unsorted);
	return true;
}

//Load unindexed vertex positions, tex coords and normals
bool load_obj(const char* file_name, float** vp, float** vt, float** vn, uint32_t* vert_count){
	char obj_file_path[64];
    sprintf(obj_file_path, "%s%s", OBJ_PATH, file_name);
	FILE* fp = fopen(obj_file_path, "r");
	if(!fp) {
		printf("Error: Failed to open %s\n", file_name);
		fclose(fp);
		return false;
	}
	printf("Loading obj: '%s'\n", file_name);

	uint32_t num_vps = 0;
	uint32_t num_vts = 0;
	uint32_t num_vns = 0;
	uint32_t num_faces = 0;

	char line[OBJLOAD_LINE_SIZE];
	line[0] = '\0';

	//Count number of elements
	while(fgets(line, OBJLOAD_LINE_SIZE, fp)){
		if(line[0]=='v'){
			if(line[1]==' ') num_vps++;
			else if(line[1]=='t') num_vts++;
			else if(line[1]=='n') num_vns++;
		}
		else if(line[0]=='f') num_faces++;
	}
	rewind(fp);

	// printf("%u vps, ", num_vps);
	// printf("%u vts, ", num_vts);
	// printf("%u vns, ", num_vns);
	// printf("%u faces ", num_faces);

	*vert_count = 3*num_faces;
	//vp, tex coords and vn arrays that will be sorted based on indices in file
	*vp = (float*)malloc(*vert_count*3*sizeof(float));
	if(num_vts>0) *vt = (float*)malloc(*vert_count*2*sizeof(float));
	if(num_vns>0) *vn = (float*)malloc(*vert_count*3*sizeof(float));

	uint32_t mem_alloced = *vert_count* 2*sizeof(float);
	if(num_vts>0) mem_alloced += *vert_count*2*sizeof(float);
	if(num_vns>0) mem_alloced += *vert_count*3*sizeof(float);
	printf("(Allocated %u bytes)\n", mem_alloced);

	//Arrays to hold the unsorted data from the obj
	float* vp_unsorted = (float*)malloc(num_vps*3*sizeof(float));
	float* vt_unsorted = (float*)malloc(num_vts*2*sizeof(float));
	float* vn_unsorted = (float*)malloc(num_vns*3*sizeof(float));

	//Iterators
	uint32_t unsort_vp_it = 0; 
	uint32_t unsort_vt_it = 0; 
	uint32_t unsort_vn_it = 0; 
	uint32_t vp_it = 0; //final sorted buffers
	uint32_t vt_it = 0;
	uint32_t vn_it = 0;

	while(fgets(line, OBJLOAD_LINE_SIZE, fp)){
		if(line[0]=='v'){
			//Read in unsorted data (vp, vt, vn)
			if(line[1]==' '){
				int ret = sscanf(line, "v %f %f %f", &vp_unsorted[3*unsort_vp_it], &vp_unsorted[3*unsort_vp_it+1], &vp_unsorted[3*unsort_vp_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong vertex position layout \n");
					printf("Expected format: v x y z \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				unsort_vp_it+=1;
			}
			else if(line[1]=='t'){
				int ret = sscanf(line, "vt %f %f", &vt_unsorted[2*unsort_vt_it], &vt_unsorted[2*unsort_vt_it+1]);
				if(ret!=2){
					printf("ERROR: Wrong vertex uv layout \n");
					printf("Expected format: vt u v \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				unsort_vt_it+=1;
			}
			else if(line[1]=='n'){
				int ret = sscanf(line, "vn %f %f %f", &vn_unsorted[3*unsort_vn_it], &vn_unsorted[3*unsort_vn_it+1], &vn_unsorted[3*unsort_vn_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong vertex normal layout \n");
					printf("Expected format: vn x y z \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				unsort_vn_it+=1;
			}
		}
		else if(line[0]=='f'){
			uint16_t indices[3];
			//Scan the line depending on what parameters are included for faces
			if(num_vts==0 && num_vns==0){ //Just vertex positions
				int ret = sscanf(line, "f %hu %hu %hu", &indices[0], &indices[1], &indices[2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v v v \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				for(int i=0; i<3; i++){
					indices[i]-=1; //wavefront obj doesn't use zero indexing
					(*vp)[3*vp_it  ] = vp_unsorted[3*indices[i]];   //x
					(*vp)[3*vp_it+1] = vp_unsorted[3*indices[i]+1]; //y
					(*vp)[3*vp_it+2] = vp_unsorted[3*indices[i]+2]; //z
					vp_it+=1;
				}
			}
			else if(num_vts==0){ //vertex positions and normals
				uint16_t vn_index[3];
				int ret = sscanf(line, "f %hu//%hu %hu//%hu %hu//%hu", &indices[0], &vn_index[0], &indices[1], &vn_index[1], &indices[2], &vn_index[2]);
				if(ret!=6){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v//n v//n v//n \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				for(int i=0; i<3; i++){
					indices[i]-=1; //wavefront obj doesn't use zero indexing
					vn_index[i]-=1;
					(*vp)[3*vp_it  ] = vp_unsorted[3*indices[i]  ]; //x
					(*vp)[3*vp_it+1] = vp_unsorted[3*indices[i]+1]; //y
					(*vp)[3*vp_it+2] = vp_unsorted[3*indices[i]+2]; //z
					(*vn)[3*vn_it  ] = vn_unsorted[3*vn_index[i]  ];//x
					(*vn)[3*vn_it+1] = vn_unsorted[3*vn_index[i]+1];//y
					(*vn)[3*vn_it+2] = vn_unsorted[3*vn_index[i]+2];//z
					vp_it+=1;
					vn_it+=1;
				}
			}
			else if(num_vns==0){ //vertex positions and tex coords
				uint16_t vt_index[3];
				int ret = sscanf(line, "f %hu/%hu %hu/%hu %hu/%hu", &indices[0], &vt_index[0], &indices[1], &vt_index[1], &indices[2], &vt_index[2]);
				if(ret!=6){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v/t v/t v/t \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				for(int i=0; i<3; i++){
					indices[i]-=1; //wavefront obj doesn't use zero indexing
					vt_index[i]-=1;
					(*vp)[3*vp_it  ] = vp_unsorted[3*indices[i]  ]; //x
					(*vp)[3*vp_it+1] = vp_unsorted[3*indices[i]+1]; //y
					(*vp)[3*vp_it+2] = vp_unsorted[3*indices[i]+2]; //z
					(*vt)[2*vt_it  ] = vt_unsorted[2*vt_index[i]  ];//u
					(*vt)[2*vt_it+1] = vt_unsorted[2*vt_index[i]+1];//v
					vp_it+=1;
					vt_it+=1;
				}
			}
			else{ //vertex positions and tex coords and normals
				uint16_t vt_index[3], vn_index[3];
				int ret = sscanf(line, "f %hu/%hu/%hu %hu/%hu/%hu %hu/%hu/%hu", &indices[0], &vt_index[0], &vn_index[0], 
															          			&indices[1], &vt_index[1], &vn_index[1], 
																	  			&indices[2], &vt_index[2], &vn_index[2]);
				if(ret!=9){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v/t/n v/t/n v/t/n \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				for(int i=0; i<3; i++){
					indices[i]-=1; //wavefront obj doesn't use zero indexing
					vt_index[i]-=1;
					vn_index[i]-=1;
					(*vp)[3*vp_it  ] = vp_unsorted[3*indices[i] ];  //x
					(*vp)[3*vp_it+1] = vp_unsorted[3*indices[i]+1]; //y
					(*vp)[3*vp_it+2] = vp_unsorted[3*indices[i]+2]; //z
					(*vt)[2*vt_it  ] = vt_unsorted[2*vt_index[i]  ]; //u
					(*vt)[2*vt_it+1] = vt_unsorted[2*vt_index[i]+1]; //v
					(*vn)[3*vn_it  ] = vn_unsorted[3*vn_index[i]  ]; //x
					(*vn)[3*vn_it+1] = vn_unsorted[3*vn_index[i]+1]; //y
					(*vn)[3*vn_it+2] = vn_unsorted[3*vn_index[i]+2]; //z
					vp_it+=1;
					vt_it+=1;
					vn_it+=1;
				}
			}
		}
	}
	fclose(fp);

	free(vp_unsorted);
	free(vt_unsorted);
	free(vn_unsorted);

	return true;
}

//Load vertex positions with index buffer, ignore tex coords and normals if present
bool load_obj_indexed(const char* file_name, float** vp, uint16_t** indices, uint32_t* vert_count, uint32_t* index_count){
	char obj_file_path[64];
    sprintf(obj_file_path, "%s%s", OBJ_PATH, file_name);
	FILE* fp = fopen(obj_file_path, "r");
	if(!fp) {
		printf("Error: Failed to open %s\n", file_name);
		fclose(fp);
		return false;
	}
	printf("Loading obj: '%s'\n", file_name);

	uint32_t num_vps = 0;
	uint32_t num_vts = 0;
	uint32_t num_vns = 0;
	uint32_t num_faces = 0;

	char line[OBJLOAD_LINE_SIZE];
	line[0] = '\0';

	//Count number of elements
	while(fgets(line, OBJLOAD_LINE_SIZE, fp)){
		if(line[0]=='v'){
			if(line[1]==' ') num_vps++;
			else if(line[1]=='t') num_vts++;
			else if(line[1]=='n') num_vns++;
		}
		else if(line[0]=='f') num_faces++;
	}
	rewind(fp);

	// printf("%u vps, ", num_vps);
	// printf("%u vts, ", num_vts);
	// printf("%u vns, ", num_vns);
	// printf("%u faces ", num_faces);

	if(num_faces>(1<<16)/3){
		printf("ERROR loading %s: Too many faces (%u) for 16-bit index buffer\n", file_name, num_faces);
		return false;
	}

	*index_count = 3*num_faces;
	*vert_count = num_vps;
	*vp = (float*)malloc(num_vps*3*sizeof(float));
	*indices = (uint16_t*)malloc(*index_count*sizeof(uint16_t));
	uint32_t mem_alloced = (uint32_t)(num_vps*3*sizeof(float) + (*index_count)*sizeof(uint16_t));
	printf("(Allocated %u bytes)\n", mem_alloced);

	//Iterators
	uint32_t unsort_vp_it = 0;
	uint32_t index_it = 0;

	while(fgets(line, OBJLOAD_LINE_SIZE, fp)){
		if(line[0]=='v'){
			if(line[1]==' '){
				int ret = sscanf(line, "v %f %f %f", &(*vp)[3*unsort_vp_it], &(*vp)[3*unsort_vp_it+1], &(*vp)[3*unsort_vp_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong vertex position layout \n");
					printf("Expected format: v x y z \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				unsort_vp_it+=1;
			}
		}
		else if(line[0]=='f'){
			//Scan the line depending on what parameters are included for faces
			if(num_vts==0 && num_vns==0){ //Just vertex positions
				int ret = sscanf(line, "f %hu %hu %hu",  &(*indices)[index_it], &(*indices)[index_it+1], &(*indices)[index_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v v v \n");
					printf("Observed format: %s\n", line);
					return false;
				}
			}
			else if(num_vts==0){ //vertex positions and normals
				int ret = sscanf(line, "f %hu//%*u %hu//%*u %hu//%*u", &(*indices)[index_it], &(*indices)[index_it+1], &(*indices)[index_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v//n v//n v//n \n");
					printf("Observed format: %s\n", line);
					return false;
				}
			}
			else if(num_vns==0){ //vertex positions and tex coords
				int ret = sscanf(line, "f %hu/%*u %hu/%*u %hu/%*u", &(*indices)[index_it], &(*indices)[index_it+1], &(*indices)[index_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v/t v/t v/t \n");
					printf("Observed format: %s\n", line);
					return false;
				}
			}
			else{ //vertex positions and tex coords and normals
				int ret = sscanf(line, "f %hu/%*u/%*u %hu/%*u/%*u %hu/%*u/%*u", &(*indices)[index_it], &(*indices)[index_it+1], &(*indices)[index_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v/t/n v/t/n v/t/n \n");
					printf("Observed format: %s\n", line);
					return false;
				}
			}
			for(int i=0; i<3; i++){
				(*indices)[index_it]--; //wavefront obj doesn't use zero indexing
				index_it+=1;
			}
		}//end elseif for 'f'
	}//endwhile
	fclose(fp);
	return true;
}

//Load vertex positions, tex coords and normals with index buffer
//Smooth normals by default
bool load_obj_indexed(const char* file_name, float** vp, float** vt, float** vn, uint16_t** indices, uint32_t* vert_count, uint32_t* index_count, float smooth_normal_factor){
	char obj_file_path[64];
    sprintf(obj_file_path, "%s%s", OBJ_PATH, file_name);
	FILE* fp = fopen(obj_file_path, "r");
	if(!fp) {
		printf("Error: Failed to open %s\n", file_name);
		fclose(fp);
		return false;
	}
	printf("Loading obj: '%s'\n", file_name);

	uint32_t num_vps = 0;
	uint32_t num_vts = 0;
	uint32_t num_vns = 0;
	uint32_t num_faces = 0;

	char line[OBJLOAD_LINE_SIZE];
	line[0] = '\0';

	//Count number of elements
	while(fgets(line, OBJLOAD_LINE_SIZE, fp)){
		if(line[0]=='v'){
			if(line[1]==' ') num_vps++;
			else if(line[1]=='t') num_vts++;
			else if(line[1]=='n') num_vns++;
		}
		else if(line[0]=='f') num_faces++;
	}
	rewind(fp);

	// printf("%u vps, ", num_vps);
	// printf("%u vts, ", num_vts);
	// printf("%u vns, ", num_vns);
	// printf("%u faces ", num_faces);

	if(num_faces>(1<<16)/3){
		printf("ERROR loading %s: Too many faces (%u) for 16-bit index buffer\n", file_name, num_faces);
		return false;
	}
	
	//overallocate to worst possible case, i.e. every vertex is unique
	//realloc to shrink later
	*index_count = 3*num_faces; //3 verts for every face (all verts unique)
	*vp = (float*)malloc(*index_count*3*sizeof(float)); 
	*indices = (uint16_t*)malloc(*index_count*sizeof(uint16_t));

	//vt and vn arrays that will be sorted based on index buffer
	if(num_vts>0) *vt = (float*)malloc(*index_count*2*sizeof(float));
	if(num_vns>0) *vn = (float*)calloc(*index_count*3, sizeof(float) + sizeof(float)); //must be zeroed, we add to this later

	//Arrays to hold the unsorted data from the obj
	float* vp_unsorted = (float*)malloc(3*num_vps*sizeof(float));
	float* vt_unsorted = (float*)malloc(2*num_vts*sizeof(float));
	float* vn_unsorted = (float*)malloc(3*num_vns*sizeof(float));

	//Iterators
	uint32_t vert_it = 0;
	uint32_t unsort_vp_it = 0;
	uint32_t unsort_vt_it = 0;
	uint32_t unsort_vn_it = 0;
	uint32_t index_it = 0; //iterator for index buffer

	//if we get a file with just positions then we won't add any more
	//vertices and won't use this iterator. Set it to its final value
	if(num_vts==0 && num_vns==0) vert_it = num_vps;

	while(fgets(line, OBJLOAD_LINE_SIZE, fp)){
		if(line[0]=='v'){
			if(line[1]==' '){
				int ret = sscanf(line, "v %f %f %f", &vp_unsorted[3*unsort_vp_it], &vp_unsorted[3*unsort_vp_it+1], &vp_unsorted[3*unsort_vp_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong vertex position layout \n");
					printf("Expected format: v x y z \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				unsort_vp_it+=1;
			}
			else if(line[1]=='t'){
				int ret = sscanf(line, "vt %f %f", &vt_unsorted[2*unsort_vt_it], &vt_unsorted[2*unsort_vt_it+1]);
				if(ret!=2){
					printf("ERROR: Wrong vertex uv layout \n");
					printf("Expected format: vt u v \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				unsort_vt_it+=1;
			}
			else if(line[1]=='n'){
				int ret = sscanf(line, "vn %f %f %f", &vn_unsorted[3*unsort_vn_it], &vn_unsorted[3*unsort_vn_it+1], &vn_unsorted[3*unsort_vn_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong vertex normal layout \n");
					printf("Expected format: vn x y z \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				unsort_vn_it+=1;
			}
		}

		else if(line[0]=='f'){
			//Scan the line depending on what parameters are included for faces
			if(num_vts==0 && num_vns==0){ //Just vertex positions
				int ret = sscanf(line, "f %hu %hu %hu", &(*indices)[index_it], &(*indices)[index_it+1], &(*indices)[index_it+2]);
				if(ret!=3){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v v v \n");
					printf("Observed format: %s\n", line);
					return false;
				}

				for(int i=0; i<3; i++){
					((*indices)[index_it])--; //wavefront doesn't use zero indexing
					uint16_t curr_ind = (*indices)[index_it];
					(*vp)[3*curr_ind]   = vp_unsorted[3*curr_ind];
					(*vp)[3*curr_ind+1] = vp_unsorted[3*curr_ind+1];
					(*vp)[3*curr_ind+2] = vp_unsorted[3*curr_ind+2];
					index_it+=1;
				}
			}

			else if(num_vts==0){ //positions and normals
				uint16_t index[3], vn_index[3];
				int ret = sscanf(line, "f %hu//%hu %hu//%hu %hu//%hu", &index[0], &vn_index[0], &index[1], &vn_index[1],  &index[2], &vn_index[2]);
				if(ret!=6){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v//n v//n v//n \n");
					printf("Observed format: %s\n", line);
					return false;
				}

				for(int i=0; i<3; i++){ //add vn for the 3 verts in this face
					index[i]-=1; //wavefront doesn't use zero indexing
					vn_index[i]-=1;

					(*indices)[index_it] = index[i];
					uint16_t curr_ind = index[i];

					//Search index buffer for current vert, see if it already exists
					// *** NB: O(n), yikes! Could try to improve, but the real solution is to use a saner file format 
					//than .obj for indexed meshes with UVs! Run this horrorshow once and convert to a better format ***
					bool found_duplicate = false;
					//Get vertex data for the vert we're about to add:
					vec3 vp_curr = vec3(vp_unsorted[3*curr_ind],    vp_unsorted[3*curr_ind+1],    vp_unsorted[3*curr_ind+2]);
					vec3 vn_curr = vec3(vn_unsorted[3*vn_index[i]], vn_unsorted[3*vn_index[i]+1], vn_unsorted[3*vn_index[i]+2]);
					for(int j=index_it-1; j>=0; --j){ //iterate backwards, dupe verts are usually close
						//Get jth vertex data
						vec3 vp_j = vec3((*vp)[3*(*indices)[j]], (*vp)[3*(*indices)[j]+1], (*vp)[3*(*indices)[j]+2]);
						vec3 vn_j = vec3((*vn)[3*(*indices)[j]], (*vn)[3*(*indices)[j]+1], (*vn)[3*(*indices)[j]+2]);

						//Check if jth vertex is the same as new vertex
						if(vp_curr == vp_j){
							//If we don't want smoothed normals, normal must be the same
							if(dot(vn_curr, vn_j) < smooth_normal_factor) continue;

							//Vertex is the same! Just append its index to buffer
							found_duplicate = true;
							(*indices)[index_it] = (*indices)[j];
							//Add new (jth) vertex normal to existing one and normalise later
							(*vn)[3*(*indices)[j]  ] += vn_curr.x;
							(*vn)[3*(*indices)[j]+1] += vn_curr.y;
							(*vn)[3*(*indices)[j]+2] += vn_curr.z;
							break;
						}
					}//end for j

					if(!found_duplicate){ //Current vertex is new, add to buffers
						//Add point to *vp
						assert(vert_it < *index_count);
						(*vp)[3*vert_it]   = vp_curr.x;
						(*vp)[3*vert_it+1] = vp_curr.y;
						(*vp)[3*vert_it+2] = vp_curr.z;
						//Add normal
						(*vn)[3*vert_it]   += vn_curr.x;
						(*vn)[3*vert_it+1] += vn_curr.y;
						(*vn)[3*vert_it+2] += vn_curr.z;
						//Append new index to index buffer
						assert(index_it<*index_count);
						assert(index_it<(1<<16));
						(*indices)[index_it] = vert_it;
						vert_it+=1; //advance index
					}
					index_it+=1;

				}//end for i
			}//end if num_vts

			else if(num_vns==0){ //positions and tex coords
				uint16_t index[3], vt_index[3];
				int ret = sscanf(line, "f %hu/%hu %hu/%hu %hu/%hu", &index[0], &vt_index[0], &index[1], &vt_index[1],  &index[2], &vt_index[2]);
				if(ret!=6){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v/t v/t v/t \n");
					printf("Observed format: %s\n", line);
					return false;
				}

				for(int i=0; i<3; i++){
					index[i]--; //wavefront doesn't use zero indexing
					vt_index[i]--;
					uint16_t curr_ind = index[i];
					//Search index buffer for current vert, see if it already exists
					// *** NB: O(n), yikes! Could try to improve, but the real solution is to use a saner file format 
					//than .obj for indexed meshes with UVs! Run this horrorshow once and convert to a better format ***
					bool found_duplicate = false;
					//Get vertex data for the vert we're about to add:
					vec3 vp_curr = vec3(vp_unsorted[3*curr_ind],    vp_unsorted[3*curr_ind+1], vp_unsorted[3*curr_ind+2]);
					vec2 vt_curr = vec2(vt_unsorted[2*vt_index[i]], vt_unsorted[2*vt_index[i]+1]);
					for(int j=index_it-1; j>=0; --j){ //iterate backwards, dupe verts are usually close
						//Get jth vertex data
						vec3 vp_j = vec3((*vp)[3*(*indices)[j]], (*vp)[3*(*indices)[j]+1], (*vp)[3*(*indices)[j]+2]);
						vec2 vt_j = vec2((*vt)[2*(*indices)[j]], (*vt)[2*(*indices)[j]+1]);

						//Check if jth vertex is the same as new vertex
						if((vp_curr ==vp_j) && (vt_curr == vt_j)){
							//Vertex is the same! Just append its index to buffer
							found_duplicate = true;
							(*indices)[index_it] = (*indices)[j];
							break;
						}
					}//end for j

					if(!found_duplicate){ //Current vertex is new, add to buffers
						//Add point to *vp
						assert(vert_it < *index_count);
						(*vp)[3*vert_it]   = vp_curr.x;
						(*vp)[3*vert_it+1] = vp_curr.y;
						(*vp)[3*vert_it+2] = vp_curr.z;
						//Add UV to *vt
						(*vt)[2*vert_it]   = vt_unsorted[2*vt_index[i]];
						(*vt)[2*vert_it+1] = vt_unsorted[2*vt_index[i]+1];
						//Change index to be newest point
						assert(index_it<*index_count);
						assert(index_it<(1<<16));
						(*indices)[index_it] = vert_it;
						vert_it+=1;
					}
					index_it+=1;
				}//end for i
			}//end if num_vns

			else{ //positions, tex coords and normals
				uint16_t index[3], vt_index[3], vn_index[3];
				int ret = sscanf(line, "f %hu/%hu/%hu %hu/%hu/%hu %hu/%hu/%hu", &index[0], &vt_index[0], &vn_index[0], 
																	  			&index[1], &vt_index[1], &vn_index[1], 
																	 			&index[2], &vt_index[2], &vn_index[2]);
				if(ret!=9){
					printf("ERROR: Wrong face layout \n");
					printf("Expected format: f v/t/n v/t/n v/t/n \n");
					printf("Observed format: %s\n", line);
					return false;
				}
				for(int i=0; i<3; i++){
					index[i]--; //wavefront doesn't use zero indexing
					vt_index[i]--;
					vn_index[i]--;
					uint16_t curr_ind = index[i];
					//Search index buffer for current vert, see if it already exists
					// *** NB: O(n), yikes! Could try to improve, but the real solution is to use a saner file format 
					//than .obj for indexed meshes with UVs! Run this horrorshow once and convert to a better format ***
					bool found_duplicate = false;
					//Get vertex data for the vert we're about to add:
					vec3 vp_curr = vec3(vp_unsorted[3*curr_ind],    vp_unsorted[3*curr_ind+1], vp_unsorted[3*curr_ind+2]);
					vec2 vt_curr = vec2(vt_unsorted[2*vt_index[i]], vt_unsorted[2*vt_index[i]+1]);
					vec3 vn_curr = vec3(vn_unsorted[3*vn_index[i]], vn_unsorted[3*vn_index[i]+1], vn_unsorted[3*vn_index[i]+2]);
					for(int j=index_it-1; j>=0; --j){ //iterate backwards, dupe verts are usually close
						//Get jth vertex data
						vec3 vp_j = vec3((*vp)[3*(*indices)[j]], (*vp)[3*(*indices)[j]+1], (*vp)[3*(*indices)[j]+2]);
						vec2 vt_j = vec2((*vt)[2*(*indices)[j]], (*vt)[2*(*indices)[j]+1]);
						vec3 vn_j = vec3((*vn)[3*(*indices)[j]], (*vn)[3*(*indices)[j]+1], (*vn)[3*(*indices)[j]+2]);

						//Check if jth vertex is the same as new vertex
						if((vp_curr == vp_j) && (vt_curr == vt_j)) {
							//If we don't want smoothed normals, normal must be the same
							if(dot(vn_curr, vn_j) < smooth_normal_factor)  continue;
							
							//Vertex is the same! Just append its index to buffer
							found_duplicate = true;
							(*indices)[index_it] = (*indices)[j];

							//Add new (jth) vertex normal to existing one and normalise later
							(*vn)[3*(*indices)[j]  ] += vn_curr.x;
							(*vn)[3*(*indices)[j]+1] += vn_curr.y;
							(*vn)[3*(*indices)[j]+2] += vn_curr.z;
							break;
						}
					}//end for j

					if(!found_duplicate){ //Current vertex is new, add to buffers
						//Add point to *vp
						assert(vert_it < *index_count);
						(*vp)[3*vert_it]   = vp_curr.x;
						(*vp)[3*vert_it+1] = vp_curr.y;
						(*vp)[3*vert_it+2] = vp_curr.z;
						//Add UV to *vt
						(*vt)[2*vert_it]   = vt_curr.x;
						(*vt)[2*vert_it+1] = vt_curr.y;
						//Add normal
						(*vn)[3*vert_it]   += vn_curr.x;
						(*vn)[3*vert_it+1] += vn_curr.y;
						(*vn)[3*vert_it+2] += vn_curr.z;
						//Append new index to index buffer
						assert(index_it<*index_count);
						assert(index_it<(1<<16));
						(*indices)[index_it] = vert_it;
						vert_it+=1; //advance index
					}
					index_it+=1;
				}//end for i
			}//end else{ //positions, tex coords and normals

		}//end if line[0]=='f'
	}//end while
	
	//Resize everything to free up the space we didn't use
	*vert_count = vert_it;
	*index_count = index_it;
	*vp = (float*)realloc(*vp, *vert_count*3*sizeof(float));
	if(num_vts>0) *vt = (float*)realloc(*vt, *vert_count*2*sizeof(float));
	if(num_vns>0) *vn = (float*)realloc(*vn, *vert_count*3*sizeof(float));
	*indices = (uint16_t*)realloc(*indices, *index_count*sizeof(uint16_t));

	uint32_t mem_alloced = *vert_count*3*sizeof(float) + (*index_count)*sizeof(uint16_t);
	if(num_vts>0) mem_alloced += *vert_count*2*sizeof(float);
	if(num_vns>0) mem_alloced += *vert_count*3*sizeof(float);
	printf("(Allocated %u bytes)\n", mem_alloced);

	//Normalise vn
	if(num_vns>0){
		for(unsigned int i=0; i<*vert_count*3; i+=3){
			vec3 temp = vec3((*vn)[i], (*vn)[i+1], (*vn)[i+2]);
			temp = normalise(temp);
			(*vn)[i]   = temp.x;
			(*vn)[i+1] = temp.y;
			(*vn)[i+2] = temp.z;
		}
	}

	fclose(fp);

	free(vp_unsorted);
	free(vt_unsorted);
	free(vn_unsorted);

	return true;
}
