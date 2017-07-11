import plotly
import plotly.offline as offline
import plotly.graph_objs as go
import json
import sys

def plot(input, output):
	dataFile = open(input, 'r')
	data = json.load(dataFile)

	# set up relevant data arrays
	labels = data["distanceHistogram"]["labels"]
	vals = data["distanceHistogram"]["values"]

	# construct plots
	distanceHistogram = go.Bar(x=labels, y=vals, name='Number of elements')

	plots = [distanceHistogram]
	layout = dict(title="Histogram of Distances from Found Global Minima")

	offline.plot(dict(data=plots, layout=layout), filename=output + ".html", auto_open=False)


if __name__ == "__main__":
	plot(sys.argv[1], sys.argv[2])