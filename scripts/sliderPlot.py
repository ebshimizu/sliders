import plotly
import plotly.offline as offline
import plotly.graph_objs as go
import json
import sys

def plot(input, output):
	dataFile = open(input, 'r')
	data = json.load(dataFile)

	# set up relevant data arrays
	plots = []

	for id in data:
		plot = go.Scatter(x=data[id]["x"], y=data[id]["y"], name=id)
		plots.append(plot)

	# construct plots
	offline.plot(dict(data=plots, layout=dict(title="Slider Graph")), filename=output, auto_open=False)

if __name__ == "__main__":
	plot(sys.argv[1], sys.argv[2])