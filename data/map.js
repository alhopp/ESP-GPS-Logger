// ============================================================================
// MapView
// - ESP-hosted Leaflet map (STA mode)
// - Displays ONE base track + optional overlay
// ============================================================================

window.MapView = {
  map:null,
  baseTrack:null,
  overlay:null,
  defaultOverlays:{},
  dot:null,
  _r:null,
  _overlays:{},

  // -------------------------------------------------------------------------
  // init() — create map once
  // -------------------------------------------------------------------------
  init(){
    if(this.map){ this.map.invalidateSize(true); return; }

    const el = $("mapView");

    this.map = L.map(el,{
      zoomControl:false,
      attributionControl:false,
      inertia:false,
      preferCanvas:true,
      minZoom:5,
      maxZoom:22
    });

    // Shared canvas renderer
    this._r = L.canvas({padding:0.5});

    // -----------------------------------------------------------------------
    // PANES — REQUIRED for correct z-order with canvas
    // -----------------------------------------------------------------------
    this.map.createPane("basePane");
    this.map.createPane("overlayPane");
    this.map.createPane("overlayNmPane");
    this.map.createPane("overlayAlphaPane");
    this.map.createPane("overlay10Pane");
    this.map.createPane("overlay2Pane");

    this.map.getPane("basePane").style.zIndex    = 400;
    this.map.getPane("overlayPane").style.zIndex = 450;
    this.map.getPane("overlayNmPane").style.zIndex = 455;
    this.map.getPane("overlayAlphaPane").style.zIndex = 460;
    this.map.getPane("overlay10Pane").style.zIndex = 465;
    this.map.getPane("overlay2Pane").style.zIndex = 470;

    L.tileLayer(
      "https://server.arcgisonline.com/ArcGIS/rest/services/" +
      "World_Imagery/MapServer/tile/{z}/{y}/{x}",
      {
        maxNativeZoom:18,
        maxZoom:22,
        attribution:"© Esri"
      }
    ).addTo(this.map);

    setTimeout(()=>{
      this.map.setView([0,0],14);
      this.map.invalidateSize(true);

      if(window.MapSessions && !MapSessions._started){
        MapSessions._started = true;
        MapSessions.init();
      }
    },150);
  },

  // -------------------------------------------------------------------------
  zoomToBounds(bounds, meta){
    const b = bounds;
    if(!b.isValid()) return;

    this.map.fitBounds(b,{
      padding:[30,30],
      animate:false,
      maxZoom:16
    });

    const z = this.map.getZoom();
    if(z > 16) this.map.setZoom(16);
    if(z < 13) this.map.setZoom(13);

    if(meta){
      console.log("[Map] bounds", meta);
    }
  },

  trackBounds(feature){
    const coords = feature?.geometry?.coordinates || [];
    const bounds = L.latLngBounds([]);
    let count = 0;

    coords.forEach(c=>{
      if(!Array.isArray(c) || c.length < 2) return;

      const lon = Number(c[0]);
      const lat = Number(c[1]);
      if(!Number.isFinite(lat) || !Number.isFinite(lon)) return;
      if(Math.abs(lat) < 0.001 && Math.abs(lon) < 0.001) return;
      if(Math.abs(lat) > 90 || Math.abs(lon) > 180) return;

      bounds.extend([lat, lon]);
      count++;
    });

    return { bounds, count };
  },

  // -------------------------------------------------------------------------
  // loadGeoJSON() — multi-feature aware
  // -------------------------------------------------------------------------
  loadGeoJSON(url){
  if(!this.map) return;

  this.clear();

  fetch(url,{cache:"no-store"})
    .then(r=>{
      if(!r.ok) throw Error("GeoJSON fetch failed");
      return r.json();
    })
    .then(gj=>{
      if(!gj.features) return;

      this._overlays = {};

      // ---- split features by mode ----
      const base = gj.features.find(f=>f.properties?.mode==="track");

      gj.features.forEach(f=>{
        const m = f.properties?.mode;
        if(!m || m==="track") return;

        // ignore empty LineStrings
        if(!f.geometry?.coordinates || f.geometry.coordinates.length < 2) return;

        if(!this._overlays[m]) this._overlays[m] = [];
        this._overlays[m].push(f);
      });

      // ---- stats only live on base track ----
      const stats = base?.properties?.stats;
      updateStatsUI(stats);
      StatsGraph.setSession(base, this._overlays, stats);

      // ---- draw base track (grey) ----
      if(base){
        const tb = this.trackBounds(base);

        this.baseTrack = L.geoJSON(base,{
          pane:"basePane",
          renderer:this._r,
          coordsToLatLng:c=>L.latLng(c[1],c[0]),
          style:{ color:"#9aa0a6", weight:4, opacity:0.75 }
        }).addTo(this.map);

        this.zoomToBounds(tb.bounds,{
          points: tb.count,
          south: tb.bounds.getSouth(),
          west: tb.bounds.getWest(),
          north: tb.bounds.getNorth(),
          east: tb.bounds.getEast()
        });
      }

      this.showOverlay("nm", { pinned:true });
      this.showOverlay("alpha", { pinned:true });
      this.showOverlay("10s", { pinned:true });
      this.showOverlay("2s", { pinned:true });

      setTimeout(()=>this.map.invalidateSize(true),50);
    })
    .catch(e=>console.warn("[Map] GeoJSON failed",e));
},


// -------------------------------------------------------------------------
// showOverlay() — draw one performance slice on top
// -------------------------------------------------------------------------
showOverlay(mode, options={}){
  const pinned = !!options.pinned;
  const style = this.overlayStyle(mode);
  const pane = options.pane || style.pane;
  const color = options.color || style.color;
  const weight = options.weight || style.weight;

  if(pinned){
    if(this.defaultOverlays[mode]){
      this.map.removeLayer(this.defaultOverlays[mode]);
      this.defaultOverlays[mode] = null;
    }
  }else if(this.overlay){
    this.map.removeLayer(this.overlay);
    this.overlay = null;
  }

  if(!pinned && this.defaultOverlays[mode]) return;

  const list = this._overlays?.[mode];
  if(!list || !list.length) return;

  const layer = L.geoJSON(
    {
      type:"FeatureCollection",
      features:list
    },
    {
      pane,
      renderer:this._r,
      coordsToLatLng:c=>L.latLng(c[1],c[0]),
      style:{ color, weight, opacity:1 }
    }
  ).addTo(this.map);

  if(pinned) this.defaultOverlays[mode] = layer;
  else this.overlay = layer;
},

overlayStyle(mode){
  return ({
    nm:{ pane:"overlayNmPane", color:"#007aff", weight:4 },
    alpha:{ pane:"overlayAlphaPane", color:"#34c759", weight:5 },
    "10s":{ pane:"overlay10Pane", color:"#ff9500", weight:6 },
    "2s":{ pane:"overlay2Pane", color:"#ff3b30", weight:7 }
  })[mode] || { pane:"overlayPane", color:"#af52de", weight:5 };
},


  // -------------------------------------------------------------------------
  clear(){
    if(this.baseTrack){ this.map.removeLayer(this.baseTrack); this.baseTrack=null; }
    if(this.overlay){ this.map.removeLayer(this.overlay); this.overlay=null; }
    Object.keys(this.defaultOverlays).forEach(k=>{
      if(this.defaultOverlays[k]) this.map.removeLayer(this.defaultOverlays[k]);
    });
    this.defaultOverlays = {};
    if(this.dot){ this.map.removeLayer(this.dot); this.dot=null; }
    this._overlays = {};
    StatsGraph.clear();
  }
};


// ============================================================================
// MapSessions
// ============================================================================

window.MapSessions = {
  files:[], index:0, _started:false,

  async init(){
    const r = await fetch("/api/files",{cache:"no-store"});
    const j = await r.json();
    if(!j.ok) return;

    this.files = j.files
      .filter(f=>f.name.endsWith(".geojson"))
      .sort(compareSessionFiles);

    if(!this.files.length) return;

    this.index = 0;
    this.loadCurrent();
    this.bindGestures();
  },

  async reload(){
    const r = await fetch("/api/files",{cache:"no-store"});
    const j = await r.json();
    if(!j.ok) return;

    this.files = j.files
      .filter(f=>f.name.endsWith(".geojson"))
      .sort(compareSessionFiles);

    if(!this.files.length){
      MapView.clear();
      updateStatsUI(null);
      $("sessionTitle").textContent = "No sessions";
      $("sessionMeta").textContent  = "";
      return;
    }

    if(this.index >= this.files.length)
      this.index = this.files.length - 1;

    this.loadCurrent();
  },

  loadCurrent(){
    const f = this.files[this.index];
    if(!f) return;

    const total   = this.files.length;
    const logical = total - this.index;
    const displayName = f.sbp_name || f.name;

    $("sessionTitle").textContent = `Session ${logical} of ${total}`;
    $("sessionMeta").textContent  = displayName.replace(/\.(geojson|sbp|ubx|txt)$/i,"");

    MapView.clear();
    MapView.loadGeoJSON(`/api/download?file=${encodeURIComponent(f.name)}&t=${Date.now()}`);
  },

  prev(){ if(this.index < this.files.length-1){ this.index++; this.loadCurrent(); } },
  next(){ if(this.index > 0){ this.index--; this.loadCurrent(); } },

  // -------------------------------------------------------------------------
  // FIXED gesture handler
  // -------------------------------------------------------------------------
bindGestures(){
  const card = $("sessionCard");
  let x0=0,y0=0,dx=0,dy=0,active=false,locked=null;
  const THRESH = 40;

  card.addEventListener("touchstart",e=>{
    const t = e.touches[0];
    x0=t.clientX; y0=t.clientY;
    dx=dy=0; locked=null; active=true;
  },{passive:true});

  card.addEventListener("touchmove",e=>{
    if(!active) return;

    const t = e.touches[0];
    dx=t.clientX-x0;
    dy=t.clientY-y0;

    if(!locked){
      if(Math.abs(dx)>12) locked="x";
      else if(Math.abs(dy)>12) locked="y";
      else return;
    }

    e.preventDefault();   // 🔒 OWN the gesture
  },{passive:false});

  card.addEventListener("touchend",()=>{
    if(!active) return;
    active=false;

    if(locked==="y"){
      if(dy < -THRESH) showStats();
      else if(dy > THRESH) hideStats();
      return;
    }
    if(locked==="x"){
      if(dx < -THRESH) this.next();
      else if(dx > THRESH) this.prev();
    }
  });
}

};


// ============================================================================
// Stats helpers
// ============================================================================

function showStats(){
  $("sessionCard").classList.add("stats");
  StatsGraph.scheduleDraw();
}

function hideStats(){
  $("sessionCard").classList.remove("stats");
  MapView.map?.invalidateSize(true);
}


// ---------------------------------------------------------------------------
// updateStatsUI()
// ---------------------------------------------------------------------------

function updateStatsUI(stats){
  const set = (id,v)=>{
    const el = $(id);
    if(el) el.textContent = v;
  };

  if(!stats){
    [
      "map_stat_2s","map_stat_10s","map_stat_alpha",
      "map_stat_nm","map_stat_1h","map_stat_distance"
    ].forEach(id=>set(id,"00.000"));
    return;
  }

  set("map_stat_2s",       stats.max?.toFixed(3)      ?? "–");
  set("map_stat_10s",      stats.avg10?.toFixed(3)    ?? "–");
  set("map_stat_alpha",    stats.alpha?.toFixed(3)    ?? "–");
  set("map_stat_nm",       stats.nm?.toFixed(3)       ?? "–");
  set("map_stat_1h",       stats.h1?.toFixed(3)       ?? "–");
  set("map_stat_distance", stats.distance?.toFixed(3) ?? "–");
}

const StatsGraph = {
  base:null,
  overlays:{},
  stats:null,
  chart:null,
  drawTimer:null,
  mode:"alpha",
  graphModes:new Set(["2s","10s","alpha","nm","1h","distance"]),
  colors:["#1e88e5","#ff9500","#34c759","#af52de","#ff3b30"],

  setSession(base, overlays, stats){
    this.base = base || null;
    this.overlays = overlays || {};
    this.stats = stats || null;
    this.select(this.mode, { keepCollapsed:true });
  },

  clear(){
    this.base = null;
    this.overlays = {};
    this.stats = null;
    this.drawEmpty("No session data");
  },

  select(mode, options={}){
    if(!this.canGraph(mode)) return;

    this.mode = mode || "alpha";
    document.querySelectorAll(".stat").forEach(el=>{
      el.classList.toggle("selected", el.dataset.mode === this.mode);
    });
    if(!options.keepCollapsed) showStats();
    this.scheduleDraw();
  },

  canGraph(mode){
    return this.graphModes.has(mode);
  },

  scheduleDraw(){
    if(this.drawTimer){
      clearTimeout(this.drawTimer);
      this.drawTimer = null;
    }

    requestAnimationFrame(()=>{
      this.draw();
      this.drawTimer = setTimeout(()=>this.draw(), 340);
    });
  },

  draw(){
    const el = $("statsGraph");
    if(!el) return;

    const series = this.seriesForMode(this.mode);
    if(!series.sets.length || !series.sets[0].points.length){
      this.drawEmpty(`No ${this.labelForMode(this.mode)} graph data`);
      return;
    }

    if(typeof uPlot === "undefined"){
      this.drawEmpty("Graph library missing");
      return;
    }

    const parentRect = el.parentElement?.getBoundingClientRect();
    const width = Math.max(220, Math.round(el.clientWidth || parentRect?.width || 0));
    const height = Math.max(160, Math.round(el.clientHeight || parentRect?.height || 0));
    if(!width || !height) return;

    this.destroyChart();
    el.classList.remove("empty");
    delete el.dataset.empty;
    el.textContent = "";

    const x = series.sets[0].points.map(p=>p.x);
    const ys = series.sets.map(set=>set.points.map(p=>p.y));
    const xTicks = series.xTicks || [];

    this.chart = new uPlot({
      width,
      height,
      padding:[2, 38, 16, 8],
      legend:{show:false},
      cursor:{show:true, x:false, y:false},
      scales:{x:{time:false}},
      axes:[
        {
          stroke:"#5b6b82",
          grid:{show:false},
          ticks:{show:false},
          splits:()=>xTicks.map(t=>t.value),
          values:(u, vals)=>vals.map(v=>this.xLabelForValue(v, xTicks))
        },
        {
          stroke:"#5b6b82",
          grid:{stroke:"rgba(91,107,130,.22)", width:1},
          values:(u, vals)=>vals.map(v=>v.toFixed(1))
        }
      ],
      series:[
        {},
        ...series.sets.map((set,i)=>({
          label:set.label || series.unit,
          stroke:set.color || this.colors[i % this.colors.length],
          width:this.mode === "10s" ? 2 : 3,
          points:{show:false}
        }))
      ]
    }, [x,...ys], el);

    this.drawInfo(series);
  },

  drawEmpty(text){
    const el = $("statsGraph");
    const info = $("statsInfo");
    if(!el) return;
    this.destroyChart();
    el.classList.add("empty");
    el.dataset.empty = text;
    el.textContent = "";
    if(info) info.innerHTML = "";
  },

  seriesForMode(mode){
    const values = this.base?.properties?.graphs?.[mode] || [];
    const meta = this.base?.properties?.graph_meta?.[mode] || {};
    const rawSets = Array.isArray(values?.[0]) ? values : [values];
    const axis = this.xAxisForMode(mode, rawSets[0]?.length || 0, meta);
    const sets = rawSets
      .map((set,i)=>({
        label:mode === "10s" ? `${i + 1}` : this.labelForMode(mode),
        color:this.colors[i % this.colors.length],
        points:(set || [])
          .map((v,idx)=>({ x:this.xForIndex(idx, set.length, axis.max), y:Number(v) }))
          .filter(p=>Number.isFinite(p.y))
      }))
      .filter(set=>set.points.length);
    return {
      label:this.labelForMode(mode),
      unit:"kt",
      xTicks:axis.ticks,
      sets
    };
  },

  drawInfo(series){
    const el = $("statsInfo");
    if(!el) return;
    el.classList.toggle("ten-sec-info", this.mode === "10s");
    el.classList.toggle("alpha-info", this.mode === "alpha");

    const pills = [];
    if(this.mode === "10s"){
      for(let i = 0; i < 5; i++){
        const set = series.sets[i] || { color:this.colors[i % this.colors.length], points:[] };
        const avg = this.avg(set.points);
        const value = Number.isFinite(avg) ? avg : 0;
        pills.push(`<span class="stat-info-pill stat-info-box" style="--stat-color:${set.color}"><b>${i + 1}</b><span>${value.toFixed(2)}</span></span>`);
      }
    }else{
      const points = series.sets[0]?.points || [];
      const min = this.min(points);
      const max = this.max(points);
      if(Number.isFinite(min)) pills.push(`<span class="stat-info-pill">Min ${min.toFixed(2)}</span>`);
      if(Number.isFinite(max)) pills.push(`<span class="stat-info-pill">Max ${max.toFixed(2)}</span>`);
      if(this.mode === "alpha"){
        const distance = Number(this.stats?.alphaDistance);
        const closure = Number(this.stats?.alphaClosure);
        if(Number.isFinite(distance) && distance > 0 &&
           Number.isFinite(closure) && closure > 0){
          pills.push(`<span class="stat-info-pill">Dist ${distance.toFixed(0)}m @ ${closure.toFixed(1)}m</span>`);
        }
      }
    }

    el.innerHTML = pills.join("");
  },

  values(points){
    return points.map(p=>p.y).filter(Number.isFinite);
  },

  min(points){
    const vals = this.values(points);
    return vals.length ? Math.min(...vals) : NaN;
  },

  max(points){
    const vals = this.values(points);
    return vals.length ? Math.max(...vals) : NaN;
  },

  avg(points){
    const vals = this.values(points);
    return vals.length ? vals.reduce((a,b)=>a+b,0) / vals.length : NaN;
  },

  xForIndex(index, count, max){
    if(count <= 1) return 0;
    return (index / (count - 1)) * max;
  },

  xAxisForMode(mode, count, meta={}){
    const metaMax = Number(meta.xMax);
    const max = Number.isFinite(metaMax) && metaMax > 0 ? metaMax : null;

    if(mode === "2s"){
      return this.secondAxis(max || 2, 0.2);
    }
    if(mode === "10s"){
      return this.secondAxis(max || 10, 1);
    }
    if(mode === "1h"){
      return this.minuteAxis(max || 60, true);
    }
    if(mode === "nm"){
      return this.metreAxis(max || 1852);
    }
    if(mode === "alpha"){
      return this.metreAxis(max || 500);
    }
    if(mode === "distance"){
      return this.minuteAxis(max || Math.max(1, count - 1), false);
    }
    return { max:Math.max(1, count - 1), ticks:[] };
  },

  secondAxis(max, step){
    const ticks = [];
    const end = Number(max.toFixed(3));
    for(let v=0; v<end - 0.0001; v+=step){
      const value = Number(v.toFixed(3));
      ticks.push({ value, label:this.secondLabel(value, false) });
    }
    ticks.push({ value:end, label:this.secondLabel(end, true) });
    return { max:end, ticks };
  },

  secondLabel(value, forceUnit){
    if(value === 0) return "0s";
    if(forceUnit || Number.isInteger(value)) return `${this.trimNumber(value)}s`;
    return this.trimNumber(value).replace(/^0/, "");
  },

  minuteAxis(max, preferHourTicks){
    const end = Number(max.toFixed(3));
    if(preferHourTicks && end >= 60){
      return { max:end, ticks:[
        { value:0, label:"0" },
        { value:15, label:"15" },
        { value:30, label:"30 min" },
        { value:45, label:"45" },
        { value:60, label:"60 min" }
      ]};
    }
    const mid = Number((end / 2).toFixed(3));
    return { max:end, ticks:[
      { value:0, label:"0" },
      { value:mid, label:this.minuteLabel(mid) },
      { value:end, label:this.minuteLabel(end) }
    ]};
  },

  minuteLabel(value){
    if(value < 1) return `${Math.round(value * 60)}s`;
    return `${this.trimNumber(value)} min`;
  },

  metreAxis(max){
    const end = Math.round(max);
    const mid = Math.round(end / 2);
    return { max:end, ticks:[
      { value:0, label:"0m" },
      { value:mid, label:`${mid}m` },
      { value:end, label:`${end}m` }
    ]};
  },

  trimNumber(value){
    return Number(value.toFixed(1)).toString();
  },

  xLabelForValue(value, ticks){
    const tick = ticks.find(t=>Math.abs(value - t.value) < 0.001);
    return tick ? tick.label : "";
  },

  labelForMode(mode){
    return ({
      "2s":"2 seconds",
      "10s":"5 x 10 seconds",
      alpha:"Alpha",
      nm:"Nautical mile",
      "1h":"1 hour",
      distance:"Distance"
    })[mode] || mode;
  },

  destroyChart(){
    if(this.chart){
      this.chart.destroy();
      this.chart = null;
    }
  }
};

function compareSessionFiles(a,b){
  const am = Number(a.mtime || 0);
  const bm = Number(b.mtime || 0);
  if(am || bm){
    if(bm !== am) return bm - am;
  }
  return b.name.localeCompare(a.name);
}

window.addEventListener("resize",()=>StatsGraph.scheduleDraw());


document.querySelectorAll(".stat").forEach(stat=>{
  stat.addEventListener("click", ()=>{
    const mode = stat.dataset.mode;
    if(!mode) return;
    MapView.showOverlay(mode);
    StatsGraph.select(mode);
  });
});

