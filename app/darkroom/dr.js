const PCAlib = require('ml-pca');

class DR {
  constructor(compositor, canvas) {
    this.samples = {};
    this.canvas = canvas;
    this.compositor = compositor;

    this.width = 2000;
    this.height = 2000;
    this.initCanvas();
  }

  setSamples(samples) {
    this.samples = samples;
  }

  initCanvas() {
    this.canvas.attr({ width: this.width, height: this.height });
    this.graphics = this.canvas[0].getContext("2d");
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

    // run DR method
    if (alg === "PCA") {
      this.PCA();
    }

    var data = [];
    var ids = [];

    for (var s in this.samples) {
      data.push(this.samples[s].context.layerVector(this.compositor));
      ids.push(s);
    }

    // project
    var points;
    if (alg === "PCA") {
      points = this.PCAProject(data, 2);
    }

    // draw everything on the canvas
    this.draw(points);
  }

  draw(points) {
    // determine extents and get coordinate conversions configured
    this.determineExtents(points);

    this.graphics.clearRect(0, 0, this.width, this.height);

    // right now just draw circles around everything
    // points is expected to be an array consisting of (x,y) tuples
    for (var i in points) {
      var pt = points[i];

      // draw the thing in screen coords
      var screenPt = this.localToScreen(pt[0], pt[1]);

      this.graphics.fillStyle = "#FFFFFF";
      this.graphics.strokeStyle = "#FFFFFF";

      this.graphics.beginPath();
      this.graphics.arc(screenPt[0], screenPt[1], 5, 0, Math.PI * 2);
      this.graphics.fill();
    }
  }

  determineExtents(points) {
    // min, max n stuff
    this.xMin = Number.MAX_VALUE;
    this.yMin = Number.MAX_VALUE;
    this.xMax = Number.MIN_VALUE;
    this.yMax = Number.MIN_VALUE;

    for (var i = 0; i < points.length; i++) {
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
  // screen coords are between the canvas bounds
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

  localToScreen(x, y) {
    var rel = this.localToRelative(x, y);
    return this.relativeToScreen(rel[0], rel[1]);
  }

  relativeToScreen(x, y) {
    return [x * this.width, y * this.height];
  }

  screenToRelative(x, y) {
    return [x / this.width, y / this.height];
  }
}

exports.DR = DR;