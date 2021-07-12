// Load an image and save it in PNG and JPG format using stb_image libraries
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"
// images are loaded row after row in the array

#define BLOCK_SIZE 7
#define SEARCH_BLOCK_SIZE 56

float abs_float(float value){
	if(value < 0){
		return -value;
	}
	else{
		return value;
	}
}

int min(int arg1, int arg2){
	if(arg1 < arg2){
		return arg1;
	}
	else{
		return arg2;
	}
}

int max(int arg1, int arg2){
	if(arg1 > arg2){
		return arg1;
	}
	else{
		return arg2;
	}
}

float compare_blocks(int row, int col, int width, int height, float *left_img, float *right_img){
	float sad = 0;
    int min_col = col;
    // compute bounding box for left image with (row, col) as top left point
    // compute bottom right point using (row, col)
    int bottom_row = min(row + BLOCK_SIZE, height - 1); // zero indexed, hence using (height - 1)
    int bottom_col = min(col + BLOCK_SIZE, width - 1);
    // compute bounding box for right image block in which 
    // we will scan and compare left block
    int col_min = max(0, col - SEARCH_BLOCK_SIZE);
    int col_max = min(width, col + SEARCH_BLOCK_SIZE); 
    int first_block = 1;
    float min_sad = 0;
    for (int r_indx = col_min; r_indx < col_max; ++r_indx){ // iterate through the search box horizontally left to right
        sad = 0;
        for (int i = row; i < bottom_row; ++i){ // iterate through the template vertically top to down
            int r_img_col = r_indx; // starting col for template matching in the search block
            for (int j = col; j < bottom_col; ++j){
                float left_pixel = left_img[i*width + j];
                // Right image index should be updated using offset
                // since we need to scan both left and right of the
                // block from the left image 
                float right_pixel = right_img[i*width + r_img_col];
				// printf("left_pixel: %f\n", left_pixel);
				// printf("right_pixel: %f\n", right_pixel);
                sad += abs_float(left_pixel - right_pixel);
                ++r_img_col;
            }
        } 

        if(first_block)
        {
            min_sad = sad;
            min_col = r_indx;
            first_block = 0;
        }
        else
        {
            if(sad < min_sad)
            {
                min_sad = sad;
                min_col = r_indx;
            }
        }
    }
    return col - min_col;
}

void compute_disparity(int start_chunk_row, int end_chunk_row, int start_chunk_col, int end_chunk_col, float *left_img, float *right_img, float *disparity_map, int height, int width){
	for (int k = start_chunk_row; k < end_chunk_row; ++k){
        for (int l = start_chunk_col; l < end_chunk_col; ++l){
			float sad = 0;
			int min_col = l;
			// compute bounding box for left image with (row, col) as top left point
			// compute bottom right point using (row, col)
			int bottom_row = min(k + BLOCK_SIZE, height - 1); // zero indexed, hence using (height - 1)
			int bottom_col = min(l + BLOCK_SIZE, width - 1);
			// compute bounding box for right image block in which 
			// we will scan and compare left block
			int col_min = max(0, l - SEARCH_BLOCK_SIZE);
			int col_max = min(width, l + SEARCH_BLOCK_SIZE); 
			int first_block = 1;
			float min_sad = 0;
			for (int r_indx = col_min; r_indx < col_max; ++r_indx){ // iterate through the search box horizontally left to right
				sad = 0;
				for (int i = k; i < bottom_row; ++i){ // iterate through the template vertically top to down
					int r_img_col = r_indx; // starting col for template matching in the search block
					for (int j = l; j < bottom_col; ++j){
						float left_pixel = left_img[i*width + j];
						// Right image index should be updated using offset
						// since we need to scan both left and right of the
						// block from the left image 
						float right_pixel = right_img[i*width + r_img_col];
						sad += abs_float(left_pixel - right_pixel);
						++r_img_col;
					}
				} 

				if(first_block)
				{
					min_sad = sad;
					min_col = r_indx;
					first_block = 0;
				}
				else
				{
					if(sad < min_sad)
					{
						min_sad = sad;
						min_col = r_indx;
					}
				}
			}

			int disp =  l - min_col;
            // int disp = compare_blocks(i, j, height, width, left_img, right_img);
            if(disp < 0){
                disparity_map[k*width + l] = 0;
            }
            else{
                disparity_map[k*width + l] = disp;
            }
        } 
    }
}

int main(int argc, char** argv) {
	// loading the left image and convert it to gray
    int left_width, left_height, left_channels;
    unsigned char *left_img = stbi_load("cones/cone_im2.png", &left_width, &left_height, &left_channels, 0);
    if(left_img == NULL) {
        printf("Error in loading the left image\n");
        exit(1);
    }
    printf("Loaded left image with a width of %dpx, a height of %dpx and %d channels\n", left_width, left_height, left_channels);
	
	// Convert the input image to gray
    size_t left_img_size = left_width * left_height * left_channels;
    int left_gray_channels = left_channels == 4 ? 2 : 1;
    size_t left_gray_img_size = left_width * left_height * left_gray_channels;

    unsigned char *left_gray_img = malloc(left_gray_img_size);
	float *left_gray_img_float = malloc(left_gray_img_size * sizeof(float));
    if(left_gray_img == NULL) {
        printf("Unable to allocate memory for the left gray image.\n");
        exit(1);
    }
	if(left_gray_img_float == NULL) {
        printf("Unable to allocate memory for the left gray image float.\n");
        exit(1);
    }

    for(unsigned char *p = left_img, *pg = left_gray_img; p != left_img + left_img_size; p += left_channels, pg += left_gray_channels) {
        *pg = (uint8_t)((*p + *(p + 1) + *(p + 2))/3.0);
        if(left_channels == 4) {
            *(pg + 1) = *(p + 3);
        }
    }
	
	int left_max = 0;
	for(int i = 0; i < left_gray_img_size; ++i){
		left_gray_img_float[i] = ((float)left_gray_img[i]) / 255.0;
		if(left_gray_img[i] > left_max){
			left_max = left_gray_img[i];
		}
	}
	printf("left gray image max = %d\n", left_max);

    stbi_write_png("left_img.png", left_width, left_height, left_gray_channels, left_gray_img, left_width * left_gray_channels);

    stbi_image_free(left_img);
	// free(left_gray_img);
	// free(left_gray_img_float);
	
	// loading the right image and convert it to gray
    int right_width, right_height, right_channels;
    unsigned char *right_img = stbi_load("cones/cone_im6.png", &right_width, &right_height, &right_channels, 0);
    if(right_img == NULL) {
        printf("Error in loading the right image\n");
        exit(1);
    }
    printf("Loaded right image with a width of %dpx, a height of %dpx and %d channels\n", right_width, right_height, right_channels);
	
	// Convert the input image to gray
    size_t right_img_size = right_width * right_height * right_channels;
    int right_gray_channels = right_channels == 4 ? 2 : 1;
    size_t right_gray_img_size = right_width * right_height * right_gray_channels;

    unsigned char *right_gray_img = malloc(right_gray_img_size);
	float *right_gray_img_float = malloc(right_gray_img_size * sizeof(float));
    if(right_gray_img == NULL) {
        printf("Unable to allocate memory for the right gray image.\n");
        exit(1);
    }
	if(right_gray_img_float == NULL) {
        printf("Unable to allocate memory for the right gray image float.\n");
        exit(1);
    }

    for(unsigned char *p = right_img, *pg = right_gray_img; p != right_img + right_img_size; p += right_channels, pg += right_gray_channels) {
        *pg = (uint8_t)((*p + *(p + 1) + *(p + 2))/3.0);
        if(right_channels == 4) {
            *(pg + 1) = *(p + 3);
        }
    }
	
	int right_max = 0;
	for(int i = 0; i < right_gray_img_size; ++i){
		right_gray_img_float[i] = ((float)right_gray_img[i]) / 255.0;
		if(right_gray_img[i] > right_max){
			right_max = right_gray_img[i];
		}
	}
	printf("right gray image max = %d\n", left_max);

    stbi_write_png("right_img.png", right_width, right_height, right_gray_channels, right_gray_img, right_width * right_gray_channels);

    stbi_image_free(right_img);
	// free(right_gray_img);
	// free(right_gray_img_float);
	
	// initialize the disparity map
	unsigned char *disparity_map = malloc(left_gray_img_size);
	float *disparity_map_float = malloc(left_gray_img_size * sizeof(float));
	if(disparity_map == NULL) {
        printf("Unable to allocate memory for the disparity map.\n");
        exit(1);
    }
	if(disparity_map_float == NULL) {
        printf("Unable to allocate memory for the disparity map float.\n");
        exit(1);
    }
	
	for(int i = 0; i < left_gray_img_size; ++i){
		disparity_map[i] = 0;
	}
	
	for(int i = 0; i < left_gray_img_size; ++i){
		disparity_map_float[i] = 0;
	}
	
	// compute disparity map
	compute_disparity(BLOCK_SIZE, left_height-BLOCK_SIZE, BLOCK_SIZE, left_width-BLOCK_SIZE, left_gray_img_float, right_gray_img_float, disparity_map_float, left_height, left_width);
	float disparity_map_float_max  = 0;
	for(int i = 0; i < left_gray_img_size; ++i){
		if(disparity_map_float[i] > disparity_map_float_max){
			disparity_map_float_max = disparity_map_float[i];
		}
	}
	for(int i = 0; i < left_gray_img_size; ++i){
		disparity_map[i] = disparity_map_float[i] / disparity_map_float_max * 255;
	}
	stbi_write_png("cones_disparity_map_c.png", left_width, left_height, left_gray_channels, disparity_map, left_width * left_gray_channels);
	
	// realease memory
	free(left_gray_img);
	free(left_gray_img_float);
	free(right_gray_img);
	free(right_gray_img_float);
	free(disparity_map);
	free(disparity_map_float);
	
	return 0;
}