import plotly
import plotly.offline as offline
import plotly.graph_objs as go
import json
import sys

def plot(input, output):
	dataFile = open(input, 'r')
	data = json.load(dataFile)

	# set up relevant data arrays
	ks = []
	rnd = []
	ceres = []
	minima = []
	best = []
	avgImprove = []

	for x in range(0, len(data)):
		ks.append(x)
		rnd.append(data[x]["rndBetter"])
		ceres.append(data[x]["ceresBetter"])
		minima.append(data[x]["timesNewMinimaFound"])
		best.append(data[x]["bestScore"])
		avgImprove.append(data[x]["averageImprovement"])

	# construct plots
	rndGraph = go.Bar(x=ks, y=rnd, name='Random Sample < Original Minima')
	ceresGraph = go.Bar(x=ks, y=ceres, name='Ceres Optimized < Original Minima')
	minimaGraph = go.Bar(x=ks, y=minima, name='New Minima Found (tolerance=0.25)')

	plots = [rndGraph, ceresGraph, minimaGraph]

	offline.plot(dict(data=plots), filename=output + ".html", auto_open=False)

	bestGraph = go.Bar(x=ks, y=best, name='Best Found')
	impGraph = go.Bar(x=ks, y=avgImprove, name='Average Improvement')

	offline.plot(dict(data=[bestGraph]), filename=output + "_scores.html", auto_open=False)
	offline.plot(dict(data=[impGraph]), filename=output + "_deltas.html", auto_open=False)


if __name__ == "__main__":
	plot(sys.argv[1], sys.argv[2])