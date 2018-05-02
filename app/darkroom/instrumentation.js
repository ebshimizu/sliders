const fs = require('fs');

class Instrumentation {
  constructor() {
    this.reset();
  }

  logAction(actionName) {
    if (!(actionName in this._counts)) {
      this._counts[actionName] = 0;
    }

    this._counts[actionName] += 1;
  }

  start() {
    this._start = new Date();
  }

  end() {
    this._end = new Date();
  }

  get duration() {
    return (this._end.getTime() - this._start.getTime()) / 1000;
  }

  get actions() {
    return this._counts;
  }

  reset() {
    this._counts = {};
    this._start = new Date();
    this._end = new Date();
  }

  export(file) {
    let outData = Object.assign({}, this._counts);
    outData._start = this._start;
    outData._end = this._end;
    outData._duration = this.duration;

    fs.writeFileSync(file, JSON.stringify(outData, null, 2));
  }
}

exports.Instrumentation = Instrumentation;