const PCAlib = require('ml-pca');
const matrixLib = require('ml-matrix');
const Matrix = matrixLib.Matrix;

class DR {
  constructor(compositor, canvas) {
    this.samples = {};
    this.canvas = canvas;
    this.compositor = compositor;
    this.ids = [];
    this.points = [];

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
      this.graphics.arc(screenPt[0], screenPt[1], 10, 0, Math.PI * 2);
      this.graphics.fill();
    }

    if (this.currentPoint) {
      var pt = this.currentPoint[0];
      var screenPt = this.localToScreen(pt[0], pt[1]);

      this.graphics.fillStyle = "#00FF00";
      this.graphics.strokeStyle = "#00FF00";

      this.graphics.beginPath();
      this.graphics.arc(screenPt[0], screenPt[1], 10, 0, Math.PI * 2);
      this.graphics.fill();
    }

    // save the data for hover test events.
    this.imgData = this.graphics.getImageData(0, 0, this.width, this.height).data;
  }

  // hover event handling
  mouseMove(event) {
    // if nothing's been rendered, skip
    if (this.imgData) {
      // scale factors
      var displayW = this.canvas.width();
      var displayH = this.canvas.height();

      var screenX = event.offsetX * (this.width / displayW);
      var screenY = event.offsetY * (this.height / displayH);

      // lock to integer vals
      screenX = Math.round(screenX);
      screenY = Math.round(screenY);

      // pixel lookup
      var idx = (screenX + screenY * this.width) * 4;

      // pixels with non-zero alpha values indicate that something's there
      if (this.imgData[idx + 3] > 0) {
        // search for the right element
        //console.log("(" + screenX + ", " + screenY + ") point hovered");

        var local = this.screenToLocal(screenX, screenY);
        var minDist = Number.MAX_VALUE;
        var id = -1;

        // brute force search for now, just use the closest point
        for (var i = 0; i < this.points.length; i++) {
          var pt = this.points[i];
          var dist = Math.sqrt(Math.pow(local[0] - pt[0], 2) + Math.pow(local[1] - pt[1], 2));

          if (dist < minDist) {
            minDist = dist;
            id = this.ids[i];
          }
        }

        if (id !== -1) {
          // show the popup
          this.showPopup(event.pageX, event.pageY, id);
        }
      }
      else {
        $('#mapVizPopup').hide();
      }
    }
  }

  showPopup(x, y, id) {
    // place elements
    // image
    var img = '<img class="ui image" src="data:image/png;base64,' + this.samples[id].img.base64() + '">';
    var info = 'ID: ' + id;

    $('#mapVizPopup .img').html(img);
    $('#mapVizPopup .content').text(info);

    // compute popup placement
    var top = y - $('#mapVizPopup').height() - 50;
    var left = x - $('#mapVizPopup').width() / 2;

    if (left < 0) {
      left = 10;
    }

    $('#mapVizPopup').css({ 'left': left, 'top': top });
    $('#mapVizPopup').show();
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

    // pad it to avoid clipping at edges
    this.xMin -= this.xMin * 0.05;
    this.xMax += this.xMax * 0.05;
    this.yMin -= this.yMin * 0.05;
    this.yMax += this.yMax * 0.05;

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

  screenToLocal(x, y) {
    var rel = this.screenToRelative(x, y);
    return this.relativeToLocal(rel[0], rel[1]);
  }
}

class PCA extends DR {
  constructor(compositor, canvas) {
    super(compositor, canvas);
  }

  // sets up data set, does initial projections, etc.
  analyze() {
    // collect data into proper format
    // 2D-array, uses layer data instead of pixel data here
    var data = [];

    for (var s in this.samples) {
      data.push(this.samples[s].context.layerVector(this.compositor));
    }

    this.pca = new PCAlib(data);
  }

  project(points, dims) {
    return this.pca.predict(points, { 'nComponents': dims });
  }

  reproject(points) {
    var pts = new Matrix(points);
    var V = this.pca.U.subMatrix(0, this.pca.U.rows - 1, 0, 1);
    var Vt = V.transpose();

    pts = pts.mmul(Vt);
    if (this.pca.center) {
      pts.addRowVector(this.pca.means);
    }

    return pts;
  }

  // computes and draws the embedding of the current dataset
  // draws the embedding of the current dataset
  // Params:
  // - alg: String indicating the method to use for the embedding
  // - options: various settings for the embedding
  embed(options = {}) {
    var method, projection;

    // run DR method
    this.analyze();

    var data = [];
    this.ids = [];

    for (var s in this.samples) {
      data.push(this.samples[s].context.layerVector(this.compositor));
      this.ids.push(s);
    }

    // project
    this.points = this.project(data, 2);
    this.currentPoint = this.project([this.compositor.getContext().layerVector(this.compositor)], 2);

    // draw everything on the canvas
    this.draw(this.points);
  }
}

exports.PCA = PCA;