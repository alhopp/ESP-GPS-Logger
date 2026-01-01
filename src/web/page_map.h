#pragma once

static const char PAGE_MAP_APP[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Offline Map</title>

<link rel="stylesheet" href="/leaflet/leaflet.css">

<style>
html, body, #map {
  height: 100%;
  margin: 0;
}
#toolbar {
  position: absolute;
  top: 10px;
  left: 10px;
  z-index: 1000;
  background: rgba(255,255,255,0.95);
  padding: 8px;
  border-radius: 8px;
  font: 14px system-ui;
}
</style>
</head>

<body>

<div id="toolbar">
  <select id="site">
    <option value="site1">Site 1</option>
    <option value="site2">Site 2</option>
    <option value="site3">Site 3</option>
  </select>
</div>

<div id="map"></div>

<script src="/leaflet/leaflet.js"></script>
<script>
const SITES = {
  site1:{center:[-32.056,115.742],zoom:13},
  site2:{center:[37.774,-122.419],zoom:13},
  site3:{center:[51.507,-0.128],zoom:13}
};

let map = L.map("map",{minZoom:11,maxZoom:15});
let layer;

function loadSite(k){
  if(layer) map.removeLayer(layer);
  layer = L.tileLayer(`/tiles/${k}/{z}/{x}/{y}.png`,{
    minZoom:11,maxZoom:15,tileSize:256,noWrap:true
  }).addTo(map);
  map.setView(SITES[k].center, SITES[k].zoom);
}

document.getElementById("site").onchange = e => loadSite(e.target.value);
loadSite("site1");
</script>

</body>
</html>
)rawliteral";
