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

  if(!pinned && this.defaultOverlays[mode]) return;

  if(pinned){
    if(this.defaultOverlays[mode]){
      this.map.removeLayer(this.defaultOverlays[mode]);
      this.defaultOverlays[mode] = null;
    }
  }else if(this.overlay){
    this.map.removeLayer(this.overlay);
    this.overlay = null;
  }

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
      .sort((a,b)=>b.name.localeCompare(a.name));

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
      .sort((a,b)=>b.name.localeCompare(a.name));

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
    MapView.loadGeoJSON(`/api/download?file=${encodeURIComponent(f.name)}`);
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
  StatsGraph.draw();
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
  mode:"alpha",
  graphModes:new Set(["2s","10s","alpha","nm","1h","distance"]),

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
    this.draw();
  },

  canGraph(mode){
    return this.graphModes.has(mode);
  },

  draw(){
    const el = $("statsGraph");
    if(!el) return;

    const series = this.seriesForMode(this.mode);
    if(!series.points.length){
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

    this.destroyChart();
    el.classList.remove("empty");
    delete el.dataset.empty;
    el.textContent = "";

    const x = series.points.map(p=>p.x);
    const y = series.points.map(p=>p.y);
    const xTicks = series.xTicks || [];

    this.chart = new uPlot({
      width,
      height,
      padding:[8, 38, 22, 10],
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
        {
          label:series.unit,
          stroke:"#1e88e5",
          width:3,
          points:{show:false}
        }
      ]
    }, [x,y], el);
  },

  drawEmpty(text){
    const el = $("statsGraph");
    if(!el) return;
    this.destroyChart();
    el.classList.add("empty");
    el.dataset.empty = text;
    el.textContent = "";
  },

  seriesForMode(mode){
    const values = this.base?.properties?.graphs?.[mode] || [];
    const axis = this.xAxisForMode(mode, values.length);
    return {
      label:this.labelForMode(mode),
      unit:"kt",
      xTicks:axis.ticks,
      points:values
        .map((v,i)=>({ x:this.xForIndex(i, values.length, axis.max), y:Number(v) }))
        .filter(p=>Number.isFinite(p.y))
    };
  },

  xForIndex(index, count, max){
    if(count <= 1) return 0;
    return (index / (count - 1)) * max;
  },

  xAxisForMode(mode, count){
    if(mode === "2s"){
      return { max:2, ticks:[
        { value:0, label:"0s" },
        { value:0.2, label:".2" },
        { value:0.4, label:".4" },
        { value:0.6, label:".6" },
        { value:0.8, label:".8" },
        { value:1.0, label:"1s" },
        { value:1.2, label:"1.2" },
        { value:1.4, label:"1.4" },
        { value:1.6, label:"1.6" },
        { value:1.8, label:"1.8" },
        { value:2, label:"2s" }
      ]};
    }
    if(mode === "10s"){
      return { max:10, ticks:[
        { value:0, label:"0s" },
        { value:1, label:"1" },
        { value:2, label:"2" },
        { value:3, label:"3" },
        { value:4, label:"4" },
        { value:5, label:"5s" },
        { value:6, label:"6" },
        { value:7, label:"7" },
        { value:8, label:"8" },
        { value:9, label:"9" },
        { value:10, label:"10s" }
      ]};
    }
    if(mode === "1h"){
      return { max:60, ticks:[
        { value:0, label:"0" },
        { value:15, label:"15" },
        { value:30, label:"30 min" },
        { value:45, label:"45" },
        { value:60, label:"60 min" }
      ]};
    }
    if(mode === "nm"){
      return { max:1852, ticks:[
        { value:0, label:"0m" },
        { value:926, label:"926m" },
        { value:1852, label:"1852m" }
      ]};
    }
    if(mode === "alpha"){
      return { max:500, ticks:[
        { value:0, label:"0m" },
        { value:250, label:"250m" },
        { value:500, label:"500m" }
      ]};
    }
    if(mode === "distance"){
      return { max:100, ticks:[
        { value:0, label:"start" },
        { value:50, label:"50%" },
        { value:100, label:"finish" }
      ]};
    }
    return { max:Math.max(1, count - 1), ticks:[] };
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

window.addEventListener("resize",()=>StatsGraph.draw());


document.querySelectorAll(".stat").forEach(stat=>{
  stat.addEventListener("click", ()=>{
    const mode = stat.dataset.mode;
    if(!mode) return;
    MapView.showOverlay(mode);
    StatsGraph.select(mode);
  });
});

