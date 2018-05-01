import sys
import numpy as np
import scipy
import os
import tensorflow as tf
import os

try:
    import urllib2
except ImportError:
    import urllib.request as urllib

from datasets import imagenet
from nets import vgg
from preprocessing import vgg_preprocessing

from tensorflow.contrib import slim

image_size = vgg.vgg_16.default_image_size
inFolder = sys.argv[1]
outFolder = sys.argv[2]

# compile files, pick only png
files = os.listdir(inFolder)
processed_images = []

with tf.Graph().as_default():
    #image = scipy.ndimage.imread(filename, mode='RGB')
    #tfimg = tf.convert_to_tensor(np.asarray(image, np.float32), np.float32)
    #print(tfimg.shape)

    #processed_image = vgg_preprocessing.preprocess_image(tfimg, image_size, image_size, is_training=False)
    #processed_images = tf.expand_dims(processed_image, 0)
    #print(processed_images.shape)

    for f in files:
        if f.endswith('.png'):
            print('Processing: ' + f)
            image = scipy.ndimage.imread(inFolder + '/' + f, mode='RGB')
            tfimg = tf.convert_to_tensor(np.asarray(image, np.float32), np.float32)
            processed_image = vgg_preprocessing.preprocess_image(tfimg, image_size, image_size, is_training=False)
            processed_images = tf.expand_dims(processed_image, 0)

            # Create the model, use the default arg scope to configure the batch norm parameters.
            with slim.arg_scope(vgg.vgg_arg_scope()):
                # 1000 classes instead of 1001.
                logits, end_points = vgg.vgg_16(processed_images, num_classes=1000, is_training=False)

            probabilities = tf.nn.softmax(logits)

            init_fn = slim.assign_from_checkpoint_fn(
                'C:/Users/falindrith/Dropbox/Documents/research/sliders_project/vgg_16/vgg_16.ckpt',
                slim.get_model_variables('vgg_16'))
            
            #print (slim.get_model_variables('vgg_16'))
            feature_conv_5_3 = end_points['vgg_16/conv4/conv4_2']

            with tf.Session() as sess:
                tf.train.start_queue_runners(sess=sess)
                init_fn(sess)
                probabilities, feats = sess.run([probabilities, feature_conv_5_3])
                #probabilities = probabilities[0, 0:]
                #sorted_inds = [i[0] for i in sorted(enumerate(-probabilities), key=lambda x:x[1])]
            
            np.save(outFolder + '/' + f, feats)
            tf.get_variable_scope().reuse_variables()
            #names = imagenet.create_readable_names_for_imagenet_labels()
            #for i in range(5):
            #    index = sorted_inds[i]
                # Shift the index of a class name by one. 
            #    print('Probability %0.2f%% => [%s]' % (probabilities[index] * 100, names[index+1]))