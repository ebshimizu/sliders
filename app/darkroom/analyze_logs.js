const fs = require('fs');
const path = require('path');
const root = './logs';
const logDir = path.join(root, process.argv[2]);
const order = JSON.parse(fs.readFileSync(path.join(root, 'ordering', `${process.argv[2]}.json`))).order;
const ifOrder = JSON.parse(fs.readFileSync(path.join(root, 'ordering', `${process.argv[2]}.json`))).interface;
const out = process.argv[3];

console.log('Processing logs.');
console.log(`Ordering: ${order.join(',')}`);

const targetLayers = {
  1: 'hair2',
  2: 'bow black',
  3: 'gold trim',
  4: 'a3',
  5: 'b2',
  6: 'b11',
  7: 'Layer 128',
  8: 'Layer 8',
  9: 'Layer 150',
  10: 'photo78',
  11: 'photo03',
  12: 'browser-right'
};

const files = fs.readdirSync(logDir);

if (files.length !== 36) {
  console.log(`Missing log file. Found ${files.length}. Expected 36.`);
}

const lines = [];

let outFile = `user,id,interface,order,target,total time,time to find, time with mod, time to confirm, control clicks, errors`

for (let i = 0; i < files.length; i++) {
  const file = files[i];
  const id = order[i % 12];
  const iface = ifOrder[Math.floor(i / 12)];
  const target = targetLayers[id];
  console.log(`Task target: ${target}`);

  const filePath = path.join(logDir, file);
  const log = JSON.parse(fs.readFileSync(filePath));

  // search for latest target layer select followed by a param change of type 10
  let latest = 0;
  let latestMod = 0;
  let firstMod = 0;
  let firstModOccur = false;
  let controlClick = 0;
  let timeToFirstSelect = 0;
  let otherLayerSelect = {};
  let firstSelectOccur = false;
  
  const targetMod = `layer: ${target}, adj: 10`;
  for (let l = 0; l < log._actions.length; l++) {
    const action = log._actions[l];
    
    if (action.type === "displayLayerControl") {
      controlClick += 1;
      if (action.message === target) {
        latest = action.time;
      }
    }

    if (action.type === "paramChange" && action.message.startsWith(targetMod)) {
      latestMod = action.time;

      if (!firstModOccur) {
        firstMod = action.time;
        firstModOccur = true;
      }
    }
    else if (action.type === "paramChange") {
      const matches = action.message.match(/layer: ([\w ]+), adj: 10/);

      if (matches !== null) {
        const lname = matches[1];
        otherLayerSelect[lname] = 1;
      }
    }

    if (action.type === "layersSelected" && firstSelectOccur === false) {
      timeToFirstSelect = action.time;
      firstSelectOccur = true;
    }
  }

  const last = log._actions[log._actions.length - 1].time;
  const selectFromFirst = latest - timeToFirstSelect;
  const errors = Object.keys(otherLayerSelect).length;
  const line = `\n${process.argv[2]},${id},${iface},${i},${target},${last},${latest},${latestMod},${firstMod},${controlClick},${errors}`;
  lines.push({
    line,
    id,
    order: i
  });
}

lines.sort(function(a, b) {
  if (a.id < b.id)
    return -1;
  if (a.id > b.id)
    return 1;
  return;
});

for (line of lines) {
  outFile += line.line;
}

fs.writeFileSync(path.join(root, out, `${process.argv[2]}.csv`), outFile);