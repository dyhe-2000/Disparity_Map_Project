import numpy as np
import matplotlib
from PIL import Image
from PIL import Image, ImageOps
import matplotlib.pyplot as plt
from tqdm import tqdm
import math 

def sum_of_abs_diff(pixel_vals_1, pixel_vals_2):
    """
    Args:
        pixel_vals_1 (numpy.ndarray): pixel block from left image
        pixel_vals_2 (numpy.ndarray): pixel block from right image

    Returns:
        float: Sum of absolute difference between individual pixels
    """
    if pixel_vals_1.shape != pixel_vals_2.shape:
        return (math.inf)

    return np.sum(abs(pixel_vals_1 - pixel_vals_2))
    
BLOCK_SIZE = 7
SEARCH_BLOCK_SIZE = 56

def compare_blocks(y, x, block_left, right_array, block_size=5):
    """
    Compare left block of pixels with multiple blocks from the right
    image using SEARCH_BLOCK_SIZE to constrain the search in the right
    image.

    Args:
        y (int): row index of the left block
        x (int): column index of the left block
        block_left (numpy.ndarray): containing pixel values within the 
                    block selected from the left image
        right_array (numpy.ndarray]): containing pixel values for the 
                     entrire right image
        block_size (int, optional): Block of pixels width and height. 
                                    Defaults to 5.

    Returns:
        tuple: (y, x) row and column index of the best matching block 
                in the right image
    """
    # Get search range for the right image
    x_min = max(0, x - SEARCH_BLOCK_SIZE)
    x_max = min(right_array.shape[1], x + SEARCH_BLOCK_SIZE)
    #print(f'search bounding box: ({y, x_min}, ({y, x_max}))')
    first = True
    min_sad = None
    min_index = None
    for x in range(x_min, x_max):
        block_right = right_array[y: y+block_size,
                                  x: x+block_size]
        sad = sum_of_abs_diff(block_left, block_right)
        # print(f'sad: {sad}, {y, x}')
        if first:
            min_sad = sad
            min_index = (y, x)
            first = False
        else:
            if sad < min_sad:
                min_sad = sad
                min_index = (y, x)
    # print(y, " ", x, " ", min_index)
    # plt.imshow(block_left)
    # plt.show()
    # block_right = right_array[min_index[0]: min_index[0]+block_size, min_index[1]: min_index[1]+block_size]
    # plt.imshow(block_right)
    # plt.show()
    return min_index
    
left_array = Image.open('teddy/teddy_im2.png')
right_array = img = Image.open('teddy/teddy_im6.png')
left_array = ImageOps.grayscale(left_array)
right_array = ImageOps.grayscale(right_array)
left_array = np.asarray(left_array)
right_array = np.asarray(right_array)
left_array = left_array / np.max(left_array)
right_array = right_array / np.max(right_array)
# print(left_array[100:110, 100:110])
# print(right_array[100:110, 100:110])
# print(left_array[0:10, 0:10])
# print(right_array[0:10, 0:10])
# plt.imshow(left_array)
# plt.show()
# plt.imshow(right_array)
# plt.show()

h, w = left_array.shape
disparity_map = np.zeros((h, w))

for y in tqdm(range(BLOCK_SIZE, h-BLOCK_SIZE)):
        for x in range(BLOCK_SIZE, w-BLOCK_SIZE):
            block_left = left_array[y:y + BLOCK_SIZE, x:x + BLOCK_SIZE]
            min_index = compare_blocks(y, x, block_left, right_array, block_size=BLOCK_SIZE)
            disparity_map[y, x] = abs(min_index[1] - x)
            
fig, axs = plt.subplots(nrows=1, ncols=1)
axs.imshow(disparity_map, cmap='hot')
fig.savefig('generated.PNG')