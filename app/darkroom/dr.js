const PCAlib = require('ml-pca');

class DR {
  constructor(compositor, canvas) {
    this.samples = {};
    this.canvas = canvas;
    this.compositor = compositor;

    this.width = 2000;
    this.height = 2000;
    initCanvas();
  }

  setSamples(samples) {
    this.samples = samples;
  }

  initCanvas() {
    canvas.attr({ width: this.width, height: this.height });
    this.graphics = canvas[0].getContext("2d");
    this.graphics.clearRect(0, 0, this.width, this.height);
  }

  // runs PCA on the current set of samples
  // doesn't return anything, results are self-contained until accessed
  // also automatically computes the projection itself
  PCA() {
    // collect data into proper format
    // 2D-array, uses layer data instead of pixel data here
    var data = [];

    for (var s in this.samples) {
      data.push(this.samples[s].context.layerVector(this.compositor));
    }

    this.pca = new PCAlib(data);
  }

  PCAProject(points, dims) {
    return this.pca.predict(points, { 'nComponents': dims });
  }

  // computes and draws the embedding of the current dataset
  // draws the embedding of the current dataset
  // Params:
  // - alg: String indicating the method to use for the embedding
  // - options: various settings for the embedding
  embed(alg, options = {}) {
    var method, projection;

    if (alg === "PCA") {
      method = this.PCA;
      projection = this.PCAProject;
    }

    // run the DR method
    // this should cache the proper data inside the object
    method();

    var data = [];
    var ids = [];

    for (var s in this.samples) {
      data.push(this.samples[s].context.layerVector());
      ids.append(s);
    }

    var points = projection(points, 2);

    // draw everything on the canvas
    this.draw(points);
  }

  draw(points) {
    // determine extents and get coordinate conversions configured
    this.determineExtents(points);

    this.graphics.clearRect(0, 0, this.width, this.height);

    // right now just draw circles around everything
  }

  determineExtents(points) {
    // min, max n stuff
    this.xMin = Number.MAX_VALUE;
    this.yMin = Number.MAX_VALUE;
    this.xMax = Number.MIN_VALUE;
    this.yMax = Number.MIN_VALUE;

    for (var i = 0; i < points.length(); i++) {
      var pt = points[i];

      if (pt[0] < this.xMin) {
        this.xMin = pt[0];
      }

      if (pt[0] > this.xMax) {
        this.xMax = pt[0];
      }

      if (pt[1] < this.yMin) {
        this.yMin = pt[1];
      }

      if (pt[1] > this.yMax) {
        this.yMax = pt[1];
      }
    }

    this.xRange = this.xMax - this.xMin;
    this.yRange = this.yMax - this.yMin;
  }

  // coordinate conversion functions
  // relative coords are [0, 1], local coords are within the bounds defined by min and max
  relativeToLocal(x, y) {
    var lx = x * this.xRange + this.xMin;
    var ly = y * this.yRange + this.yMin;

    return [lx, ly];
  }

  localToRelative(x, y) {
    var rx = (x - this.xMin) / this.xRange;
    var ry = (y - this.yMin) / this.yRange;

    return [rx, ry];
  }
}

exports.DR = DR;