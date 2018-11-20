const fs = require('fs');

class Instrumentation {
  constructor() {
    this.reset();
  }

  logAction(actionName, detail) {
    if (!(actionName in this._counts)) {
      this._counts[actionName] = 0;
    }

    this._counts[actionName] += 1;
    this._actions.push({
      type: actionName,
      message: detail,
      time: (new Date().getTime() - this._start.getTime()) / 1000
    });
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
    this._actions = [];
  }

  export(file) {
    let outData = Object.assign({}, this._counts);
    outData._start = this._start;
    outData._end = this._end;
    outData._duration = this.duration;
    outData._actions = this._actions;

    fs.writeFileSync(file, JSON.stringify(outData, null, 2));
  }

  fastExport() {
    this.export(`./logs/log_${new Date().getTime()}.json`);
  }
}

exports.Instrumentation = Instrumentation;