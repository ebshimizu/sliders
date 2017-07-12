import sys
import csv
import colorsys
from collections import OrderedDict
from subprocess import call
import numpy as Math
import plotly.offline as py
import plotly.graph_objs as go
import json

inputFile = sys.argv[1]
perp = sys.argv[2]
outputPrefix = sys.argv[3]

tsneOut = "temp-tsne-out.dat"
tsneIn = "temp-tsne-in.txt"

f = open(tsneIn, 'w')

labels = []
frequency = []
desc = []
huetemp = []
sattemp = []

# gather data from json into proper format for t-sne
dataFile = open(inputFile, 'r')
data = json.load(dataFile)

for x in range(0, len(data["minima"])):
	row = data["minima"][x]

	labels.append(data["minimaScores"][x])

	size = data["minimaCount"][x]
	if (size <= 8):
		size = 8

	frequency.append(size)

	desc.append('Score: ' + str(data["minimaScores"][x]) + '<br>Frequency: ' + str(data["minimaCount"][x]))

	strRow = ['{:.5f}'.format(x) for x in row]
	f.write(",".join(strRow) + "\n")

f.close()

# run t-sne
call("python C:/Users/eshimizu/Documents/sliders_project/sliders/scripts/bhtsne.py -i " + tsneIn + " -o " + tsneOut + " -d 2 -p " + perp)

# read data from files
Y = Math.loadtxt(tsneOut)

# plot with plotly
allPoints = go.Scatter(
	x = Y[:,0],
	y = Y[:,1],
	mode = 'markers',
	name = 'Samples',
	marker = dict(size=frequency, color=labels, showscale=True, colorscale='RdBu', colorbar = dict(xanchor="right")),
	text = desc
)

graphData = [allPoints]

layout = go.Layout(
	title='t-SNE Clustering of Param Vectors'
)

py.plot(dict(data=graphData, layout=layout), filename=outputPrefix + ".html", auto_open=False)