import plotly
import plotly.offline as offline
import plotly.graph_objs as go
import json
import sys

def plot(input, output):
	dataFile = open(input, 'r')
	data = json.load(dataFile)

	# set up relevant data arrays
	best = []
	avg = []
	fitBest = []
	fitAvg = []
	gen = []

	for x in range(0, len(data) - 1):
		# analyze objectives [0] (should usually always be the ceres data)
		ceresObj = []

		for i in range(0, len(data[x]["objectives"])):
			ceresObj.append(data[x]["objectives"][i][0])

		best.append(min(ceresObj))
		avg.append(sum(ceresObj) / len(ceresObj))
		fitBest.append(min(data[x]["fitness"]))
		fitAvg.append(sum(data[x]["fitness"]) / len(data[x]["fitness"]))
		gen.append(x)

	# construct plots
	bestGraph = go.Bar(x=gen, y=best, name='Best Ceres Score in Population')
	avgGraph = go.Bar(x=gen, y=avg, name='Average Population Score')

	plots = [bestGraph, avgGraph]

	offline.plot(dict(data=plots, layout=dict(title="Ceres Objective Scores")), filename=output + "_ceres.html", auto_open=False)

	fitBestGraph = go.Bar(x=gen, y=fitBest, name='Best Fitness Score in Population')
	fitAvgGraph = go.Bar(x=gen, y=fitAvg, name='Average Population Fitness')

	plots2 = [fitBestGraph, fitAvgGraph]

	offline.plot(dict(data=plots2, layout=dict(title="Fitness Scores")), filename=output + "_fitness.html", auto_open=False)

if __name__ == "__main__":
	plot(sys.argv[1], sys.argv[2])