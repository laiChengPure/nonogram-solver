"""
ref: https://www.werc.cz/blog/2019/07/11/web-application-digit-classification-with-convolutional-neural-network
ref: https://stackoverflow.com/questions/67694895/module-tensorflow-api-v1-compat-v2-has-no-attribute-internal-google-col

version:
tensorflow==1.15.0
keras==2.1.6

"""
import numpy as np

from keras import layers
from keras.layers import Input, Dense, Activation, ZeroPadding2D, BatchNormalization, Flatten, Conv2D, Dropout
from keras.layers import MaxPooling2D
from keras.models import Model
from keras.utils import np_utils
from keras.preprocessing.image import ImageDataGenerator

NUM_CLASSES = 10
NUM_TRAINING = 9960
NUM_DEV = 200

BATCH_TRAIN = 83
BATCH_DEV = 8

PATH_TRAIN = 'dataset/train'
PATH_DEV = 'dataset/dev'

# dataset
generator = ImageDataGenerator(rescale = 1./255)
train_generator = generator.flow_from_directory(PATH_TRAIN, target_size = (28,28), 
batch_size = BATCH_TRAIN, color_mode = 'grayscale', classes = ['0','1','2','3','4','5','6','7','8','9'])

validation_generator = generator.flow_from_directory(PATH_DEV, target_size = (28,28), 
batch_size = BATCH_DEV, color_mode = 'grayscale', classes = ['0','1','2','3','4','5','6','7','8','9'])


# model
def digitClassifier(input_shape, classes):
    x_input = Input(shape = input_shape)
    
    # CONV -> BN -> RELU Block
    x = Conv2D(32, (5, 5), strides = (1, 1), name = 'conv0')(x_input)
    x = BatchNormalization(axis = 3, name = 'bn0')(x)
    x = Activation('relu')(x)
    
    x = MaxPooling2D(pool_size = (2,2), name = 'max_pool0')(x)
    
    # CONV -> BN -> RELU Block
    x = Conv2D(64, (5, 5), strides = (1, 1), name = 'conv1')(x)
    x = BatchNormalization(axis = 3, name = 'bn1')(x)
    x = Activation('relu')(x)

    x = MaxPooling2D(pool_size = (2,2), name = 'max_pool1')(x)
    
    x = Dropout(0.2)(x)
    
    x = Flatten()(x)
    x = Dense(512, activation = 'relu', name = 'fc1')(x)  #Hidden layer
    x = Dense(classes, activation = 'softmax', name = 'fc2')(x)  #Output layer

    model = Model(inputs = x_input, outputs = x, name = 'digitClassifier')
    
    return model


model = digitClassifier((28,28,1), NUM_CLASSES)
model.compile(optimizer = 'adam', loss = 'categorical_crossentropy', metrics = ['accuracy'])

model.fit_generator(train_generator, steps_per_epoch = NUM_TRAINING // BATCH_TRAIN, 
epochs = 8, validation_data = validation_generator, validation_steps = NUM_DEV // BATCH_DEV)


preds = model.evaluate_generator(validation_generator, steps = NUM_DEV // BATCH_DEV)

print ("Loss: " + str(preds[0]))
print ("Test Accuracy: " + str(preds[1]))
print ("Error: %.2f%%" % (100-preds[1]*100))

# after training and parameters tuning, I saved the trained model to file
model.save('model-classifier.h5')
model.save_weights('model-classifier.weight')