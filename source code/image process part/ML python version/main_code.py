import cv2
import numpy as np
import math

from new_classify import ClassiFier


#-----------------------------------------------------------------------------#  
#debug purpose
DEBUG = False


###################################################
#helper function: find all lines in the image using projection method
################################################### 
#reference: ChatGPT
def detect_grid_lines(thresh):
    # Load the image in grayscale
    #img = cv2.imread(image_path, cv2.IMREAD_GRAYSCALE)

    # Apply Gaussian blur to reduce noise
    #img = cv2.GaussianBlur(img, (3, 3), 0)

    # Threshold the image to binary (invert colors: lines are white)
    #_, binary = cv2.threshold(img, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
    
    binary = 255-thresh
    # Sum of pixels along rows (horizontal projection)
    horizontal_projection = np.sum(binary, axis=1)

    # Sum of pixels along columns (vertical projection)
    vertical_projection = np.sum(binary, axis=0)

    # Detect line positions
    def find_lines(projection, threshold=0.8):
        lines = []
        lines_no_filter = []
        in_line = False
        for i, val in enumerate(projection):
            if val > threshold * np.max(projection):
                lines_no_filter.append(i)
                if in_line == False:
                    lines.append(i)
                    in_line = True
            else:
                in_line = False
        return lines, lines_no_filter  #lines: some that are close to each other will be considered as the same line; lines_no_filter: no filter, store all the lines

    row_lines, row_lines_no_filter = find_lines(horizontal_projection)
    col_lines, col_lines_no_filter = find_lines(vertical_projection)
    
    '''
    # Optional: Draw lines on the image
    for y in row_lines_no_filter:
        cv2.line(img, (col_lines_no_filter[0], y), (col_lines_no_filter[-1], y), (0, 0, 0), 1)
    
    # Draw vertical lines (within grid boundary)
    for x in col_lines_no_filter:
        cv2.line(img, (x, row_lines_no_filter[0]), (x, row_lines_no_filter[-1]), (0, 0, 0), 1)
    '''
    
    return row_lines_no_filter, col_lines_no_filter


###################################################
#helper function: preprocess image
################################################### 
def preprocess_image(image):
    """Purpose:
        Returns a grayed, blurred, and adaptively thresholded camera image."""
    BKG_THRESH = 60
    gray = cv2.cvtColor(image,cv2.COLOR_BGR2GRAY)
    blur = cv2.GaussianBlur(gray,(5,5),0)

    # The best threshold level depends on the ambient lighting conditions.
    # For bright lighting, a high threshold must be used to isolate the cards
    # from the background. For dim lighting, a low threshold must be used.
    # To make the card detector independent of lighting conditions, the
    # following adaptive threshold method is used.
    #
    # A background pixel in the center top of the image is sampled to determine
    # its intensity. The adaptive threshold is set at 50 (THRESH_ADDER) higher
    # than that. This allows the threshold to adapt to the lighting conditions.
    img_w, img_h = np.shape(image)[:2]
    bkg_level = gray[int(img_h/100)][int(img_w/2)]
    thresh_level = bkg_level + BKG_THRESH
    
    retval, thresh = cv2.threshold(gray,175,255,cv2.THRESH_BINARY)
    return thresh


###################################################
#helper function: find all rects in the image
###################################################     
def find_rect(thresh_image):
    # Find contours and sort their indices by contour size
    cnts,hier = cv2.findContours(thresh_image,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
    index_sort = sorted(range(len(cnts)), key=lambda i : cv2.contourArea(cnts[i]),reverse=True)

    # If there are no contours, do nothing
    if len(cnts) == 0:
        return [], []
    
    # Otherwise, initialize empty sorted contour and hierarchy lists
    cnts_sort = []
    hier_sort = []
    cnt_is_rect = np.zeros(len(cnts),dtype=int)

    # Fill empty lists with sorted contour and sorted hierarchy. Now,
    # the indices of the contour list still correspond with those of
    # the hierarchy list. The hierarchy array can be used to check if
    # the contours have parents or not.
    for i in index_sort:
        cnts_sort.append(cnts[i])
        hier_sort.append(hier[0][i])

    # Determine which of the contours are grid blocks by applying the
    # following criteria: 1) have no child, 2) have parents,
    # and 3) have four corners
    cnt = 0
    for i in range(len(cnts_sort)):
        peri = cv2.arcLength(cnts_sort[i],True)
        approx = cv2.approxPolyDP(cnts_sort[i],0.1*peri,True)
        
        if ((hier_sort[i][2] == -1) and (len(approx) == 4)):
            cnts_sort[i] = approx
            cnt_is_rect[i] = 1
            cnt += 1
    if DEBUG:
        print("there are blocks:", cnt)
    return cnts_sort, cnt_is_rect


###################################################
#helper function: find hint numbers are at which side of the grid
################################################### 
"""Purpose:
    check whether column numbers hint are at top or bottom
    check whether row numbers hint are at left or right"""
def getWidthAndHeightRect(cnts_i):
    center_pos = (cnts_i[0] + cnts_i[1] + cnts_i[2] + cnts_i[3])/4
    width = round(abs(cnts_i[0][0][0] - center_pos[0][0])*2)
    height = round(abs(cnts_i[0][0][1] - center_pos[0][1])*2)
    return width, height
    

#-----------------------------------------------------------------------------#  
def getTopLeftofRect(cnts_i):
    tl = cnts_i[0]
    if cnts_i[1][0][0] + cnts_i[1][0][1] < tl[0][0] + tl[0][1]:
        tl = cnts_i[1]
    if cnts_i[2][0][0] + cnts_i[2][0][1] < tl[0][0] + tl[0][1]:
        tl = cnts_i[2]
    if cnts_i[3][0][0] + cnts_i[3][0][1] < tl[0][0] + tl[0][1]:
        tl = cnts_i[3]
    return tl

def getBottomLeftofRect(cnts_i):
    bl = cnts_i[0]
    if cnts_i[1][0][0] - cnts_i[1][0][1] < bl[0][0] - bl[0][1]:
        bl = cnts_i[1]
    if cnts_i[2][0][0] - cnts_i[2][0][1] < bl[0][0] - bl[0][1]:
        bl = cnts_i[2]
    if cnts_i[3][0][0] - cnts_i[3][0][1] < bl[0][0] - bl[0][1]:
        bl = cnts_i[3]
    return bl

def getTopRightofRect(cnts_i):
    tr = cnts_i[0]
    if cnts_i[1][0][0] - cnts_i[1][0][1] > tr[0][0] - tr[0][1]:
        tr = cnts_i[1]
    if cnts_i[2][0][0] - cnts_i[2][0][1] > tr[0][0] - tr[0][1]:
        tr = cnts_i[2]
    if cnts_i[3][0][0] - cnts_i[3][0][1] > tr[0][0] - tr[0][1]:
        tr = cnts_i[3]
    return tr

def getBottomRightofRect(cnts_i):
    br = cnts_i[0]
    if cnts_i[1][0][0] + cnts_i[1][0][1] > br[0][0] + br[0][1]:
        br = cnts_i[1]
    if cnts_i[2][0][0] + cnts_i[2][0][1] > br[0][0] + br[0][1]:
        br = cnts_i[2]
    if cnts_i[3][0][0] + cnts_i[3][0][1] > br[0][0] + br[0][1]:
        br = cnts_i[3]
    return br


#-----------------------------------------------------------------------------#      
def findTheClosetRectAndNotLine(thresh_image_rev, direction):
    cnts,hier = cv2.findContours(thresh_image_rev,cv2.RETR_TREE,cv2.CHAIN_APPROX_SIMPLE)
    if len(cnts) == 0:
        return -1, -1, -1, -1
    cnts_sort = []
    for (i,c)  in enumerate(cnts):
        if cv2.contourArea(c) > SKIP_AREA_SIZE:
            cnts_sort.append(c)
    if direction == DOWN_TO_UP:
    # Sort by down to top using our y_cord_contour function
        contours_direction = sorted(cnts_sort, key = y_cord_contour, reverse = True)
    elif direction == UP_TO_DOWN:
        contours_direction = sorted(cnts_sort, key = y_cord_contour, reverse = False)
    elif direction == RIGHT_TO_LEFT:
    # Sort by down to top using our x_cord_contour function
        contours_direction = sorted(cnts_sort, key = x_cord_contour, reverse = True)
    else:
        contours_direction = sorted(cnts_sort, key = x_cord_contour, reverse = False)
    
    for (i,c)  in enumerate(contours_direction):
        (x, y, w, h) = cv2.boundingRect(c)
        if direction == DOWN_TO_UP or direction == UP_TO_DOWN:
            if(x == 0 and x + w == thresh_image_rev.shape[1] and h < 0.4 * w):  #exclude the lines
                continue
        else:
            if(y == 0 and y + h == thresh_image_rev.shape[0] and w < 0.4 * h):  #exclude the lines
                continue
        return x, y, w, h
    return -1, -1, -1, -1 


###################################################
#helper function: find hint numbers
###################################################      
#reference: https://github.com/chewbacca89/OpenCV-with-Python/blob/master/Lecture%204.2%20-%20Sorting%20Contours.ipynb
"""Purpose:
    get the column numbers hint and row numbers hint"""
NUMBER_DIFF_MAX = 90000
TRAIN_IMG_W = 300
TRAIN_IMG_H = 300
DOWN_TO_UP = 0
UP_TO_DOWN = 1
RIGHT_TO_LEFT = 2
LEFT_TO_RIGHT = 3

SKIP_AREA_SIZE = 3

REMOVE_EDGE_LINE_PORTION = 0.3
REMOVE_EDGE_LINE_START_RATIO = 0.25


#-----------------------------------------------------------------------------#  
def x_cord_contour(contours):
    #Returns the X cordinate for the contour centroid
    M = cv2.moments(contours)
    return (int(M['m10']/M['m00']))
    

def y_cord_contour(contours):
    #Returns the Y cordinate for the contour centroid
    M = cv2.moments(contours)
    return (int(M['m01']/M['m00']))


#-----------------------------------------------------------------------------#  
def processImgBecomeSquare(cropped_img):
    cropped_h, cropped_w = cropped_img.shape[:2]    
    squareLen = max(cropped_h, cropped_w)    
    square_img = np.zeros((squareLen, squareLen), np.uint8)       
    shift_cy, shift_cx = (squareLen - cropped_h) // 2, (squareLen - cropped_w) // 2  
    square_img[shift_cy : shift_cy + cropped_h, shift_cx : shift_cx + cropped_w] = cropped_img
    return square_img


#-----------------------------------------------------------------------------#  
def removePossibleEdgeLineTopDown(cropped_img):
    """Purpose:
        Given cropped_img, remove the possible 2 sides white edge due to the miscontain of the lines at the sides of hint numbers"""
    cropped_h, cropped_w = cropped_img.shape[:2]
    explore_height = int(cropped_h*REMOVE_EDGE_LINE_PORTION)
        
    #Top
    lineExist = True
    for c in range(cropped_w):
        if cropped_img[0][c] == 0:
            lineExist = False
            break
    if lineExist:
        start_col = int(cropped_w * REMOVE_EDGE_LINE_START_RATIO)
        row = 0
        while(row <= explore_height and cropped_img[row][start_col] == 255):
            row += 1
        cropped_img[: row, :] = 0
    #Down
    lineExist = True
    for c in range(cropped_w):
        if cropped_img[-1][c] == 0:
            lineExist = False
            break
    if lineExist:
        start_col = int(cropped_w * REMOVE_EDGE_LINE_START_RATIO)
        row = -1
        while(abs(row) <= explore_height and cropped_img[row][start_col] == 255):
            row -= 1
        cropped_img[row + 1 :, :] = 0


def removePossibleEdgeLineLeftRight(cropped_img):
    """Purpose:
        Given cropped_img, remove the possible 2 sides white edge due to the miscontain of the lines at the sides of hint numbers"""
    cropped_h, cropped_w = cropped_img.shape[:2]
    explore_width = int(cropped_w * REMOVE_EDGE_LINE_PORTION)
    
    #left
    lineExist = True
    for r in range(cropped_h):
        if cropped_img[r][0] == 0:
            lineExist = False
            break
    if lineExist:
        start_row = int(cropped_h * REMOVE_EDGE_LINE_START_RATIO)
        col = 0
        while(col < explore_width and cropped_img[start_row][col] == 255):
            col += 1
        cropped_img[:, : col] = 0
    #Right
    lineExist = True
    for r in range(cropped_h):
        if cropped_img[r][-1] == 0:
            lineExist = False
            break
    if lineExist:
        start_row = int(cropped_h * REMOVE_EDGE_LINE_START_RATIO)
        col = -1
        while(abs(col) < explore_width and cropped_img[start_row][col] == 255):
            col -= 1
        cropped_img[:, col + 1 :] = 0


#-----------------------------------------------------------------------------#
def preprocessImgCol(cropped_img, rect_w):
    """Purpose:
        Find the bounding rectangle that enclose all the number in this cropped img"""
    cropped_h, cropped_w = cropped_img.shape[:2]
    if cropped_w == rect_w:
        removePossibleEdgeLineTopDown(cropped_img)
        removePossibleEdgeLineLeftRight(cropped_img)
    cnts,hier = cv2.findContours(cropped_img, cv2.RETR_EXTERNAL, cv2.cv2.CHAIN_APPROX_NONE)
    if len(cnts) == 0:
        return cropped_img
    
    obj_rectangles = []
    for c in cnts:
        (x, y, w, h) = cv2.boundingRect(c)
        obj_rectangles.append((x, y, w, h))

    # sort by height
    obj_rectangles = sorted(obj_rectangles, key=lambda h: h[3],reverse=True)    
    max_h = obj_rectangles[0][3]

    upper_y = -1
    lower_y = -1
    left_x = -1
    right_x = -1
    
    for rect in obj_rectangles:
        (x, y, w, h) = rect
        if(h < max_h * 0.4):
            continue
        if(upper_y == -1):
            upper_y = y + h
            lower_y = y
            left_x = x
            right_x = x + w
            continue
        
        upper_y = max(upper_y, y + h)
        lower_y = min(lower_y, y)
        left_x = min(left_x, x)
        right_x = max(right_x, x + w)
            
    return cropped_img[lower_y : upper_y, left_x : right_x]

    
def preprocessImgRow(cropped_img, rect_h):
    cropped_h, cropped_w = cropped_img.shape[:2]
    if cropped_h == rect_h:
        removePossibleEdgeLineTopDown(cropped_img)
        removePossibleEdgeLineLeftRight(cropped_img)
    cnts,hier = cv2.findContours(cropped_img, cv2.RETR_EXTERNAL, cv2.cv2.CHAIN_APPROX_NONE)
    if len(cnts) == 0:
        return cropped_img
    
    obj_rectangles = []
    for c in cnts:
        (x, y, w, h) = cv2.boundingRect(c)
        obj_rectangles.append((x, y, w, h))

    # sort by height
    obj_rectangles = sorted(obj_rectangles, key=lambda h: h[3],reverse=True)    
    max_h = obj_rectangles[0][3]

    upper_y = -1
    lower_y = -1
    left_x = -1
    right_x = -1
    
    for rect in obj_rectangles:
        (x, y, w, h) = rect
        if(h < max_h * 0.4):
            continue
        if(upper_y == -1):
            upper_y = y + h
            lower_y = y
            left_x = x
            right_x = x + w
            continue
        
        upper_y = max(upper_y, y + h)
        lower_y = min(lower_y, y)
        left_x = min(left_x, x)
        right_x = max(right_x, x + w)
            
    return cropped_img[lower_y : upper_y, left_x : right_x]


#-----------------------------------------------------------------------------#
##function to find hint numbers
def findColHintNumber(thresh_image_rev, rect_h, rect_w, direction, classifier):
    """Purpose:
        Given the j-th column image thresh_image_rev, find the hint number in this img"""
    cnts,hier = cv2.findContours(thresh_image_rev,cv2.RETR_EXTERNAL,cv2.cv2.CHAIN_APPROX_NONE)   
        
    if len(cnts) == 0:
        return
    cnts_sort = []
    for (i,c)  in enumerate(cnts):
        if cv2.contourArea(c) > SKIP_AREA_SIZE:
            cnts_sort.append(c)
    if direction == DOWN_TO_UP:
    # Sort by down to top using our y_cord_contour function
        contours_down_top = sorted(cnts_sort, key = y_cord_contour, reverse = True)
    else:
        contours_down_top = sorted(cnts_sort, key = y_cord_contour, reverse = False)
    '''
    # Labeling Contours down to top
    for (i,c)  in enumerate(contours_down_to_top):
        cv2.drawContours(thresh_image_rev, [c], -1, (0,0,255), 3)  
        M = cv2.moments(c)
        cx = int(M['m10'] / M['m00'])
        cy = int(M['m01'] / M['m00'])
        cv2.putText(thresh_image_rev, str(i+1), (cx, cy), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        cv2.imshow('6 - Down to Top Contour', thresh_image_rev)
        cv2.waitKey(0)
        (x, y, w, h) = cv2.boundingRect(c)  
        
        # Let's now crop each contour and save these images
        cropped_contour = thresh_image_rev[y:y + h, x:x + w]
        image_name = "output_shape_number_" + str(i+1) + ".jpg"
        #print image_name
        cv2.imwrite(image_name, cropped_contour)
    
    cv2.destroyAllWindows()
    '''
    
    col_hint_number_list = []    
    
    upper_y = -1
    lower_y = -1
    left_x = -1
    right_x = -1
    for (i,c)  in enumerate(contours_down_top):
        (x, y, w, h) = cv2.boundingRect(c)
        if(x == 0 and x + w == thresh_image_rev.shape[1] and h < 0.4 * w) or (h < rect_h * 0.3):  #exclude the lines or short stuff
        #if(x == 0 or y == 0 or x + w == thresh_image_rev.shape[1] or y + h == thresh_image_rev.shape[0]):  #exclude the lines
            if upper_y != -1:
                cropped_img = thresh_image_rev[lower_y : upper_y, left_x : right_x]
                cropped_img = preprocessImgCol(cropped_img, rect_w)
                cropped_img = processImgBecomeSquare(cropped_img)
                                        
                best_number_match = classifier.classify(cropped_img)
                col_hint_number_list.append(best_number_match)
            upper_y = -1
            lower_y = -1
            left_x = -1
            right_x = -1
            continue
        if(upper_y == -1):
            upper_y = y + h
            lower_y = y
            left_x = x
            right_x = x + w
            continue
        if(abs(y - lower_y) <= h * 0.3):  #for the case of two-digit number, 2 boundingRects combine together to form a number
            upper_y = max(upper_y, y + h)
            lower_y = min(lower_y, y)
            left_x = min(left_x, x)
            right_x = max(right_x, x + w)
            continue
        
        cropped_img = thresh_image_rev[lower_y : upper_y, left_x : right_x]       
        cropped_img = preprocessImgCol(cropped_img, rect_w)
        cropped_img = processImgBecomeSquare(cropped_img)                
        
        best_number_match = classifier.classify(cropped_img)
        col_hint_number_list.append(best_number_match)
        
        upper_y = y + h
        lower_y = y
        left_x = x
        right_x = x + w
        
        '''
        https://stackoverflow.com/questions/58248121/opencv-python-how-to-overlay-an-image-into-the-centre-of-another-image
        import cv2
        import numpy as np
        back = cv2.imread('back.png')
        overlay = cv2.imread('overlay.png')
        h, w = back.shape[:2]
        print(h, w)
        h1, w1 = overlay.shape[:2]
        print(h1, w1)
        # let store center coordinate as cx,cy
        cx, cy = (h - h1) // 2, (w - w1) // 2
        # use numpy indexing to place the resized image in the center of 
        # background image
        
        back[cy:cy + h1, cx:cx + w1] = overlay
        
        # view result
        cv2.imshow('back with overlay', back)
        cv2.waitKey(0)
        cv2.destroyAllWindows()
        '''
    if upper_y != -1:
        cropped_img = thresh_image_rev[lower_y : upper_y, left_x : right_x]
        cropped_img = preprocessImgCol(cropped_img, rect_w)
        cropped_img = processImgBecomeSquare(cropped_img)                
        
        best_number_match = classifier.classify(cropped_img)
        col_hint_number_list.append(best_number_match)
        
    return col_hint_number_list


def findRowHintNumber(thresh_image_rev, rect_h, rect_w, direction, classifier):
    """Given the i-th row image thresh_image_rev, find the hint number in this img"""
    cnts,hier = cv2.findContours(thresh_image_rev,cv2.RETR_EXTERNAL,cv2.cv2.CHAIN_APPROX_NONE)
    if len(cnts) == 0:
        return
    cnts_sort = []
    for (i,c)  in enumerate(cnts):
        print(cv2.contourArea(c))
        if cv2.contourArea(c) > SKIP_AREA_SIZE:
            cnts_sort.append(c)
    if direction == RIGHT_TO_LEFT:
    # Sort by down to top using our x_cord_contour function
        contours_left_right = sorted(cnts_sort, key = x_cord_contour, reverse = True)
    else:
        contours_left_right = sorted(cnts_sort, key = x_cord_contour, reverse = False)

    
    row_hint_number_list = []    
    
    upper_y = -1
    lower_y = -1
    left_x = -1
    right_x = -1
    for (i,c)  in enumerate(contours_left_right):
        (x, y, w, h) = cv2.boundingRect(c)
        if(y == 0 and y + h == thresh_image_rev.shape[0] and w < 0.4 * h) or (h < rect_h * 0.3):  #exclude the lines or short stuff
        #if(x == 0 or y == 0 or x + w == thresh_image_rev.shape[1] or y + h == thresh_image_rev.shape[0]):  #exclude the lines
            if upper_y != -1:
                cropped_img = thresh_image_rev[lower_y : upper_y, left_x : right_x]
                cropped_img = preprocessImgRow(cropped_img, rect_h)
                cropped_img = processImgBecomeSquare(cropped_img)
                
                best_number_match = classifier.classify(cropped_img)
                row_hint_number_list.append(best_number_match)
            upper_y = -1
            lower_y = -1
            left_x = -1
            right_x = -1
            continue
        if(upper_y == -1):
            upper_y = y + h
            lower_y = y
            left_x = x
            right_x = x + w
            continue
        
        if(abs(x - left_x) <= rect_w * 0.5):
            upper_y = max(upper_y, y + h)
            lower_y = min(lower_y, y)
            left_x = min(left_x, x)
            right_x = max(right_x, x + w)
            continue
        
        cropped_img = thresh_image_rev[lower_y : upper_y, left_x : right_x]
        cropped_img = preprocessImgRow(cropped_img, rect_h)
        cropped_img = processImgBecomeSquare(cropped_img)                   
        
        best_number_match = classifier.classify(cropped_img)
        row_hint_number_list.append(best_number_match)
        
        upper_y = y + h
        lower_y = y
        left_x = x
        right_x = x + w
        
    if upper_y != -1:
        cropped_img = thresh_image_rev[lower_y : upper_y, left_x : right_x]
        cropped_img = preprocessImgRow(cropped_img, rect_h)
        cropped_img = processImgBecomeSquare(cropped_img)          
        
        best_number_match = classifier.classify(cropped_img)
        row_hint_number_list.append(best_number_match)
        
        '''
        cv2.imshow('back with overlay', cropped_img)
        cv2.waitKey(0)
        cv2.destroyAllWindows()
        '''        
    return row_hint_number_list


###################################################
#main function
###################################################
"""This is the main function"""
import os

def nonogramHintDetect(path_str):
    """Purpose:
        1.preprocess the image
        2.find the grid lines in the image
        3.redraw lines onto the image for the case that the lines are a little bit incomplete"""
    #start
    file_path = path_str
    img = cv2.imread(file_path)
    thresh = preprocess_image(img)
    
    ###
    #detect lines in image
    row_lines_no_filter, col_lines_no_filter = detect_grid_lines(thresh)
   
    #Draw detected lines on the image
    for y in row_lines_no_filter:
        cv2.line(thresh, (col_lines_no_filter[0], y), (col_lines_no_filter[-1], y), (0, 0, 0), 1)
    for x in col_lines_no_filter:
        cv2.line(thresh, (x, row_lines_no_filter[0]), (x, row_lines_no_filter[-1]), (0, 0, 0), 1)

    """Purpose:
        1.find all possible rect contours
        2.fill differnt number for each rect contours onto the image as preparations for the next step."""
    ###
    #find all possible rectangle contours
    cnts_sort, cnt_is_rect = find_rect(thresh)
    
    if DEBUG:
        cv2.imwrite('thresh.jpg', thresh)
        for i in range(len(cnts_sort)):
            if(cnt_is_rect[i]):
                cv2.circle(img, (cnts_sort[i][0][0][0], cnts_sort[i][0][0][1]), radius=1, color=(0, 255, 0), thickness=2)
                cv2.circle(img, (cnts_sort[i][1][0][0], cnts_sort[i][1][0][1]), radius=1, color=(0, 255, 0), thickness=2)
                cv2.circle(img, (cnts_sort[i][2][0][0], cnts_sort[i][2][0][1]), radius=1, color=(0, 255, 0), thickness=2)
                cv2.circle(img, (cnts_sort[i][3][0][0], cnts_sort[i][3][0][1]), radius=1, color=(0, 255, 0), thickness=2)
        cv2.imwrite('grid.jpg', img)  #show all the found rectangles

    #filter the contour that is rect and put them into cnts_filter
    cnts_filter = []
    for i in range(len(cnts_sort)):
        if cnt_is_rect[i]:
            cnts_filter.append(cnts_sort[i])
    
    #fill number in different rect contours onto contour_img
    contour_img = np.full(img.shape[:2], fill_value = -1, dtype = int)
    for i in range(len(cnts_filter)):
        cv2.drawContours(contour_img, cnts_filter, i, i, -1)
    

    if DEBUG:
        cv2.imwrite('contour_img.jpg',contour_img)


    """Purpose:
        find the connection between rect"""
    dist_threshold = 10  #error threshlod for the distance between two adjacent rects
    arc_threshold_percent = 0.05 #percentage error threshold for perimeter to distinguish whether two rects are in the same grid
    
    dict_horizontal_intersect_info = {}  #[prev idx, next idx, intersect cnt, distance to the next idx]
    dict_vertical_intersect_info = {}
    
    for i in range(len(cnts_filter)):
        if i not in dict_horizontal_intersect_info:  #horizontal case
            start_pos = (cnts_filter[i][0] + cnts_filter[i][1] + cnts_filter[i][2] + cnts_filter[i][3])/4  #the center of rect
            intersect_cnt = 1
            targetPerimeter = cv2.arcLength(cnts_filter[i],True)
            min_dist = -1
            prev_idx = i
            start_x = round(start_pos[0][0])
            start_y = round(start_pos[0][1])
            horizontal_intersect_list = []  #it is a list that stores all the index of rect that can go through if starting at i and travel to right
            horizontal_intersect_list.append(i)
            for x in range(start_x, img.shape[1]):
                if contour_img[start_y][x] != -1 and contour_img[start_y][x] != prev_idx:
                    cur_idx = contour_img[start_y][x]
                    pos = (cnts_filter[cur_idx][0] + cnts_filter[cur_idx][1] + cnts_filter[cur_idx][2] + cnts_filter[cur_idx][3])/4
                    distance = np.linalg.norm(start_pos - pos)
                    perimeter = cv2.arcLength(cnts_filter[cur_idx],True)
                    if(abs(targetPerimeter-perimeter) < arc_threshold_percent*targetPerimeter and (min_dist == -1 or abs(distance-min_dist) < dist_threshold)):  #whether the current rect is in the same grid with the previous rect
                        intersect_cnt += 1
                        min_dist = distance
                        start_pos = pos
                        prev_idx = cur_idx
                        horizontal_intersect_list.append(cur_idx)
                        if cur_idx in dict_horizontal_intersect_info: #if cur_idx has been visited before, then do not need to go further, just break out
                            if dict_horizontal_intersect_info[cur_idx][3] == -1 or abs(dict_horizontal_intersect_info[cur_idx][3] - min_dist) < dist_threshold:
                                intersect_cnt = intersect_cnt + dict_horizontal_intersect_info[cur_idx][2] - 1
                            break
                    else:
                        break
            
            if len(horizontal_intersect_list) == 1:
                dict_horizontal_intersect_info[horizontal_intersect_list[0]] = [-1, -1, intersect_cnt, -1]    
            else:
                dict_horizontal_intersect_info[horizontal_intersect_list[0]] = [-1, horizontal_intersect_list[1], intersect_cnt, min_dist]  #for the first rect idx in the list
                intersect_cnt -= 1        
                for j in range(1, len(horizontal_intersect_list)-1):
                    dict_horizontal_intersect_info[horizontal_intersect_list[j]] = [horizontal_intersect_list[j-1], horizontal_intersect_list[j+1], intersect_cnt, min_dist]
                    intersect_cnt -= 1
                if horizontal_intersect_list[-1] in dict_horizontal_intersect_info:  #if the last idx is already exists in the dictionary, just need to update the prev idx for it
                    dict_horizontal_intersect_info[horizontal_intersect_list[-1]][0] = horizontal_intersect_list[-2]
        
        if i not in dict_vertical_intersect_info:  #vertical case
            start_pos = (cnts_filter[i][0] + cnts_filter[i][1] + cnts_filter[i][2] + cnts_filter[i][3])/4
            intersect_cnt = 1
            targetPerimeter = cv2.arcLength(cnts_filter[i],True)
            min_dist = -1
            prev_idx = i
            start_x = round(start_pos[0][0])
            start_y = round(start_pos[0][1])
            vertical_intersect_list = []
            vertical_intersect_list.append(i)
            for y in range(start_y, img.shape[0]):
                if contour_img[y][start_x] != -1 and contour_img[y][start_x] != prev_idx:
                    cur_idx = contour_img[y][start_x]
                    pos = (cnts_filter[cur_idx][0] + cnts_filter[cur_idx][1] + cnts_filter[cur_idx][2] + cnts_filter[cur_idx][3])/4                
                    distance = np.linalg.norm(start_pos - pos)
                    perimeter = cv2.arcLength(cnts_filter[cur_idx],True)
                    if(abs(targetPerimeter-perimeter) < arc_threshold_percent*targetPerimeter and (min_dist == -1 or abs(distance-min_dist) < dist_threshold)):
                        intersect_cnt += 1
                        min_dist = distance
                        start_pos = pos
                        prev_idx = cur_idx
                        vertical_intersect_list.append(cur_idx)
                        if cur_idx in dict_vertical_intersect_info:
                            if dict_vertical_intersect_info[cur_idx][3] == -1 or abs(dict_vertical_intersect_info[cur_idx][3] - min_dist) < dist_threshold:
                                intersect_cnt = intersect_cnt + dict_vertical_intersect_info[cur_idx][2] - 1
                            break
                    else:
                        break
            
            if len(vertical_intersect_list) == 1:
                dict_vertical_intersect_info[vertical_intersect_list[0]] = [-1, -1, intersect_cnt, -1]    
            else:
                dict_vertical_intersect_info[vertical_intersect_list[0]] = [-1, vertical_intersect_list[1], intersect_cnt, min_dist]
                intersect_cnt -= 1        
                for j in range(1, len(vertical_intersect_list)-1):
                    dict_vertical_intersect_info[vertical_intersect_list[j]] = [vertical_intersect_list[j-1], vertical_intersect_list[j+1], intersect_cnt, min_dist]
                    intersect_cnt -= 1
                if vertical_intersect_list[-1] in dict_vertical_intersect_info:
                    dict_vertical_intersect_info[vertical_intersect_list[-1]][0] = vertical_intersect_list[-2]


    """Purpose:
        how many row and col of the grid"""
    #check how many row and col: method 1
    #Find out which col num and row num appear the most
    least_col_num_cnt = 0
    least_row_num_cnt = 0
    
    possible_col_num = {}
    possible_row_num = {}    
    for i in range(len(dict_horizontal_intersect_info)):
        if dict_horizontal_intersect_info[i][2] not in possible_col_num:
            possible_col_num[dict_horizontal_intersect_info[i][2]] = 1
        else:
            possible_col_num[dict_horizontal_intersect_info[i][2]] += 1
    for i in range(len(dict_vertical_intersect_info)):
        if dict_vertical_intersect_info[i][2] not in possible_row_num:
            possible_row_num[dict_vertical_intersect_info[i][2]] = 1
        else:
            possible_row_num[dict_vertical_intersect_info[i][2]] += 1
    
    col_num = 0
    row_num = 0
    for key in possible_col_num:
        if(key > col_num and possible_col_num[key] >= least_col_num_cnt):
            col_num = key
    for key in possible_row_num:
        if(key > row_num and possible_row_num[key] >= least_row_num_cnt):
            row_num = key
    
    if DEBUG:
        print('number of row:', row_num)
        print('number of col:', col_num)

    """
    #check how many row and col : method2
    row_num = 0
    for i in range(len(cnts_filter)):    
        intersect_cnt = dict_horizontal_intersect_info[i][2]
        cnt = 1
        cur_idx = i
        next_idx = dict_vertical_intersect_info[i][1]
        while next_idx != -1:
            if dict_horizontal_intersect_info[next_idx][2] == intersect_cnt:
                cnt += 1
                next_idx = dict_vertical_intersect_info[next_idx][1]
            else:
                break
        row_num = max(cnt, row_num)

    col_num = 0
    for i in range(len(cnts_filter)):    
        intersect_cnt = dict_vertical_intersect_info[i][2]
        cnt = 1
        cur_idx = i
        next_idx = dict_horizontal_intersect_info[i][1]
        while next_idx != -1:
            if dict_vertical_intersect_info[next_idx][2] == intersect_cnt:
                cnt += 1
                next_idx = dict_horizontal_intersect_info[next_idx][1]
            else:
                break
        col_num = max(cnt, col_num)       
    print('number of row:', row_num)
    print('number of col:', col_num)
    """


    """Purpose:
        find the top-left rect of the grid"""
    tl = -1
    #if the horizontal intersect cnt == col num && the vertical intersect cnt == row num, then the rect is the top-left rect of the grid
    for i in range(len(cnts_filter)):
        if dict_horizontal_intersect_info[i][2] == col_num and dict_vertical_intersect_info[i][2] == row_num:
            tl = i
    
    if tl == -1:  #if there is no rect satisfies the condition, there is no such grid exists
        if DEBUG:
            print("not found nonogram")
        return -1
    
    #get the top-right, bottom-left, bottom-left rect of the grid
    cur = tl
    step = 1
    while step < col_num:
        cur = dict_horizontal_intersect_info[cur][1]
        step += 1
    tr = cur
    cur = tl
    step = 1
    while step < row_num:
        cur = dict_vertical_intersect_info[cur][1]
        step += 1
    bl = cur
    step = 1
    while step < col_num:
        cur = dict_horizontal_intersect_info[cur][1]
        step += 1
    br = cur

       
    if DEBUG:
        #This block is just for showing the top-left rectangle of nonogram
        img = cv2.imread(file_path)
        cv2.circle(img, (cnts_filter[tl][0][0][0], cnts_filter[tl][0][0][1]), radius=1, color=(0, 255, 0), thickness=2)
        cv2.circle(img, (cnts_filter[tl][1][0][0], cnts_filter[tl][1][0][1]), radius=1, color=(0, 255, 0), thickness=2)
        cv2.circle(img, (cnts_filter[tl][2][0][0], cnts_filter[tl][2][0][1]), radius=1, color=(0, 255, 0), thickness=2)
        cv2.circle(img, (cnts_filter[tl][3][0][0], cnts_filter[tl][3][0][1]), radius=1, color=(0, 255, 0), thickness=2)
        cv2.imwrite('temp.jpg', img)
    
    
    
    """Purpose:
        find the hint numbers are at the top/bottom or left/right of the grid"""
    thresh_rev = 255 - thresh
    
    upper_cnt = 0
    lower_cnt = 0
    left_cnt = 0
    right_cnt = 0
    
    cur_idx = tl
    while(cur_idx != -1):
        tl_points_of_rect = getTopLeftofRect(cnts_filter[cur_idx])
        x = tl_points_of_rect[0][0]
        y = tl_points_of_rect[0][1]
        w, h = getWidthAndHeightRect(cnts_filter[cur_idx])
        x1, y1, w1, h1 = findTheClosetRectAndNotLine(thresh_rev[: y, x : x + w], DOWN_TO_UP)    
        if (y1 != -1) and (y1 >= y - 2 * h):
            upper_cnt += 1
        cur_idx = dict_horizontal_intersect_info[cur_idx][1]

    cur_idx = bl
    while(cur_idx != -1):
        bl_points_of_rect = getBottomLeftofRect(cnts_filter[cur_idx])
        x = bl_points_of_rect[0][0]
        y = bl_points_of_rect[0][1]
        w, h = getWidthAndHeightRect(cnts_filter[cur_idx])    
        x1, y1, w1, h1 = findTheClosetRectAndNotLine(thresh_rev[y : , x : x + w], UP_TO_DOWN)
        if (y1 != -1) and (y1 <= 2 * h):
            lower_cnt += 1
        cur_idx = dict_horizontal_intersect_info[cur_idx][1]

    cur_idx = tl
    while(cur_idx != -1):
        tl_points_of_rect = getTopLeftofRect(cnts_filter[cur_idx])
        x = tl_points_of_rect[0][0]
        y = tl_points_of_rect[0][1]
        w, h = getWidthAndHeightRect(cnts_filter[cur_idx])
        x1, y1, w1, h1 = findTheClosetRectAndNotLine(thresh_rev[y : y + h, : x], RIGHT_TO_LEFT)    
        if (x1 != -1) and (x1 >= x - 2 * w):
            left_cnt += 1
        cur_idx = dict_vertical_intersect_info[cur_idx][1]

    cur_idx = tr
    while(cur_idx != -1):
        tr_points_of_rect = getTopRightofRect(cnts_filter[cur_idx])
        x = tr_points_of_rect[0][0]
        y = tr_points_of_rect[0][1]
        w, h = getWidthAndHeightRect(cnts_filter[cur_idx])
        x1, y1, w1, h1 = findTheClosetRectAndNotLine(thresh_rev[y : y + h, x : ], LEFT_TO_RIGHT)
        if (x1 != -1) and (x1 <= 2 * w):
            right_cnt += 1
        cur_idx = dict_vertical_intersect_info[cur_idx][1]

    if DEBUG:
        print('upper_cnt:', upper_cnt)
        print('lower_cnt:', lower_cnt)
        print('left_cnt:', left_cnt)
        print('right_cnt:', right_cnt)



    """Purpose:
        get the hint numbers"""
    col_hint_list = []
    row_hint_list = []    
    
    # include the traing model
    MODEL_FILE = '../image process part/ML python version/model-classifier.h5'
    classifier = ClassiFier(MODEL_FILE)

    # If the column hint numbers are on the upper side of nonogram
    if upper_cnt > lower_cnt:
        cur_idx = tl
        while(cur_idx != -1):
            tl_points_of_rect = getTopLeftofRect(cnts_filter[cur_idx])
            x = tl_points_of_rect[0][0]
            y = tl_points_of_rect[0][1]
            w, h = getWidthAndHeightRect(cnts_filter[cur_idx])
            col_number_list = findColHintNumber(thresh_rev[0 : y, x : x + w], h, w, DOWN_TO_UP, classifier) #scan the hint numbers from down to top
            #reverse the order in row_number_list
            col_number_list.reverse()
            col_hint_list.append(col_number_list)
            cur_idx = dict_horizontal_intersect_info[cur_idx][1]
    # If the column hint numbers are on the lower side of nonogram
    else:
        cur_idx = bl
        while(cur_idx != -1):
            bl_points_of_rect = getBottomLeftofRect(cnts_filter[cur_idx])
            x = bl_points_of_rect[0][0]
            y = bl_points_of_rect[0][1]
            w, h = getWidthAndHeightRect(cnts_filter[cur_idx])
            col_number_list = findColHintNumber(thresh_rev[y : , x : x + w], h, w, UP_TO_DOWN, classifier) #scan the hint numbers from top to down
            col_hint_list.append(col_number_list)
            cur_idx = dict_horizontal_intersect_info[cur_idx][1]

    # If the row hint numbers are on the left side of nonogram
    if left_cnt > right_cnt:
        cur_idx = tl
        while(cur_idx != -1):
            tl_points_of_rect = getTopLeftofRect(cnts_filter[cur_idx])
            x = tl_points_of_rect[0][0]
            y = tl_points_of_rect[0][1]
            w, h = getWidthAndHeightRect(cnts_filter[cur_idx])
            row_number_list = findRowHintNumber(thresh_rev[y : y + h, 0 : x], h, w, RIGHT_TO_LEFT, classifier)  #scan the hint numbers from right to left
            #reverse the order in row_number_list
            row_number_list.reverse()
            row_hint_list.append(row_number_list)
            cur_idx = dict_vertical_intersect_info[cur_idx][1]
    # If the row hint numbers are on the right side of nonogram
    else:
        cur_idx = tr
        while(cur_idx != -1):
            tr_points_of_rect = getTopRightofRect(cnts_filter[cur_idx])
            x = tr_points_of_rect[0][0]
            y = tr_points_of_rect[0][1]
            w, h = getWidthAndHeightRect(cnts_filter[cur_idx])
            row_number_list = findRowHintNumber(thresh_rev[y : y + h, x :], h, w, LEFT_TO_RIGHT, classifier)  #scan the hint numbers from left to right
            row_hint_list.append(row_number_list)
            cur_idx = dict_vertical_intersect_info[cur_idx][1]

    if DEBUG:
        print("col hint:")
        for i in range(len(col_hint_list)):
            print(col_hint_list[i])
        print("row hint:")
        for i in range(len(row_hint_list)):
            print(row_hint_list[i])
    return row_hint_list, col_hint_list