const PCAlib = require('ml-pca');

class DR {
  constructor() {
    this.samples = {};
  }

  setSamples(samples) {
    this.samples = samples;
  }

  // runs PCA on the current set of samples
  // doesn't return anything, results are self-contained untill accessed
  PCA() {
    // collect data into proper format
    // 2D-array, uses layer data instead of pixel data here
    var data = [];

    for (var s in this.samples) {
      data.append(s.context.layerVector());
    }

    this.pca = new PCAlib(data);
  }

  PCAProject(points) {
    return this.pca.predict(points);
  }
}

exports.PCA = DR;