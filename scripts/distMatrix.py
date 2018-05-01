import numpy as np
import sys
import os
import json
from pyflann import FLANN

mode = sys.argv[1]
src = sys.argv[2]

files = os.listdir(src)
filenames = []
vectors = []

for f in files:
    # need to convert into list of vectors
    if f.endswith('.npy'):
        filenames.append(f[:-8])
        data = np.load(src + '/' + f)
        # print data.shape
        #vectors.append(np.reshape(data, (14 * 14, 512)))
        vectors.append(np.reshape(data, (28 * 28, 512)))

flann = FLANN()
distMap = {}
if mode == 'chamfer':
    for i in range(0, len(filenames)):
        print('Computing chamfer for file ' + str(i))
        imgDists = {}
        dataset = vectors[i]
        for j in range(0, len(filenames)):
            testset = vectors[j]
            _, dists = flann.nn(dataset, testset, 1)
            _, rdists = flann.nn(testset, dataset, 1)
            imgDists[filenames[j]] = np.asscalar(np.sum(dists) + np.sum(rdists))

        distMap[filenames[i]] = imgDists

with open(src + '/dists.json', 'w') as outfile:
    json.dump(distMap, outfile, sort_keys=True, indent=2)