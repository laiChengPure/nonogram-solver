import os
import sys
# suspend tensorflow errors
os.environ['TF_CPP_MIN_LOG_LEVEL']='2'
stderr = sys.stderr
sys.stderr = open(os.devnull, 'w')
import numpy as np
import cv2

#https://blog.csdn.net/truelovesuju/article/details/106106193
from tensorflow.keras.models import load_model

class ClassiFier:
    def __init__(self):
        self.__model_file = '../image process part/ML python version/model-classifier.h5'
        self.__model = load_model(self.__model_file)
    
    def __init__(self, model_path):
        self.__model_file = model_path
        self.__model = load_model(self.__model_file)

    def __normalizeInput (self, x):
        x_norm = x / 255
        x_norm = x_norm.reshape(1, 28, 28, 1).astype('float32')
        
        return x_norm

    
    def __resizeImage (self, img):
        '''
        Resize img 28x28
        '''
        
        target_size = 22 #width and border = 28
        border_width = 3 
        
        old_size = img.shape[:2]
        ratio = float(target_size)/max(old_size)
        new_size = tuple([int(x*ratio) for x in old_size])
        
        delta_w = target_size - new_size[1]
        delta_h = target_size - new_size[0]
        top, bottom = delta_h//2, delta_h-(delta_h//2)
        left, right = delta_w//2, delta_w-(delta_w//2)
        
        img = cv2.resize(img,(new_size[1],new_size[0]), 0, 0, interpolation = cv2.INTER_AREA)
    
        new_img = cv2.copyMakeBorder(img, top, bottom, left, right, cv2.BORDER_CONSTANT, value=[0,0,0])
        new_img = cv2.copyMakeBorder(new_img, border_width, border_width, border_width, border_width, cv2.BORDER_CONSTANT, value=[0,0,0])
        
        return new_img

    
    def classify (self, bwimage):
        '''
        ConvNet prediction
        '''    
        contours, hierarchy = cv2.findContours(bwimage, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
            
        # select only contours with dimension similar to digit
        digit_rectangles = []
        for c in contours:
            (x, y, w, h) = cv2.boundingRect(c)
            digit_rectangles.append((x, y, w, h))

        # sort by x position
        digit_rectangles = sorted(digit_rectangles, key=lambda x: x[0])

        # prediction
        cmrDigits = []
        for digit in digit_rectangles:
            x, y, w, h = digit
            digit_image = bwimage[y:y + h, x:x + w]
            # 28x28        
            digit_image = self.__resizeImage(digit_image)
            digit_image = 255 - digit_image
            prep_image = self.__normalizeInput(digit_image)
            probab = self.__model.predict(prep_image)
            cmr_digit = probab.argmax()  
            cmrDigits.append(cmr_digit)
        
        number = 0
        for num in cmrDigits:
            number = number * 10 + num

        return number