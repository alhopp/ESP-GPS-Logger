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
    this.map.createPane("overlay10Pane");
    this.map.createPane("overlayAlphaPane");
    this.map.createPane("overlay2Pane");

    this.map.getPane("basePane").style.zIndex    = 400;
    this.map.getPane("overlayPane").style.zIndex = 450;
    this.map.getPane("overlay10Pane").style.zIndex = 460;
    this.map.getPane("overlayAlphaPane").style.zIndex = 470;
    this.map.getPane("overlay2Pane").style.zIndex = 480;

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

      this.showOverlay("10s", { pinned:true, pane:"overlay10Pane", color:"#ff9500", weight:5 });
      this.showOverlay("alpha", { pinned:true, pane:"overlayAlphaPane", color:"#34c759", weight:6 });
      this.showOverlay("2s", { pinned:true, pane:"overlay2Pane", color:"#ff3b30", weight:7 });

      setTimeout(()=>this.map.invalidateSize(true),50);
    })
    .catch(e=>console.warn("[Map] GeoJSON failed",e));
},


// -------------------------------------------------------------------------
// showOverlay() — draw one performance slice on top
// -------------------------------------------------------------------------
showOverlay(mode, options={}){
  const pinned = !!options.pinned;
  const pane = options.pane || "overlayPane";
  const color = options.color || "#ff3b30";
  const weight = options.weight || 6;

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
    ].forEach(id=>set(id,"–"));
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
    el.textContent = "";

    const x = series.points.map(p=>p.x);
    const y = series.points.map(p=>p.y);

    this.chart = new uPlot({
      width,
      height,
      padding:[8, 26, 22, 18],
      legend:{show:false},
      cursor:{show:true, x:false, y:false},
      scales:{x:{time:false}},
      axes:[
        {
          stroke:"#5b6b82",
          grid:{show:false},
          ticks:{show:false},
          values:(u, vals)=>vals.map(v=>v === 0 ? "start" : v === x[x.length - 1] ? "finish" : "")
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
    el.textContent = text;
  },

  seriesForMode(mode){
    if(mode === "distance"){
      return {
        label:this.labelForMode(mode),
        unit:"km",
        points:this.cumulativeDistance(this.coordsOf(this.base))
      };
    }

    const features = this.overlays?.[mode] || [];
    const coords = features.length ? this.coordsOf(features[0]) : this.coordsOf(this.base);
    return {
      label:this.labelForMode(mode),
      unit:"kt",
      points:this.speedSeries(coords)
    };
  },

  coordsOf(feature){
    const coords = feature?.geometry?.coordinates || [];
    return coords
      .map(c=>({ lon:Number(c[0]), lat:Number(c[1]) }))
      .filter(p=>
        Number.isFinite(p.lat) &&
        Number.isFinite(p.lon) &&
        Math.abs(p.lat) <= 90 &&
        Math.abs(p.lon) <= 180 &&
        (Math.abs(p.lat) >= 0.001 || Math.abs(p.lon) >= 0.001)
      );
  },

  speedSeries(coords){
    const points = [];
    for(let i=1;i<coords.length;i++){
      const meters = this.distanceMeters(coords[i-1], coords[i]);
      points.push({ x:i, y:meters * 1.943844 });
    }
    return points;
  },

  cumulativeDistance(coords){
    const points = [];
    let meters = 0;
    for(let i=1;i<coords.length;i++){
      meters += this.distanceMeters(coords[i-1], coords[i]);
      points.push({ x:i, y:meters / 1000 });
    }
    return points;
  },

  distanceMeters(a,b){
    const rad = Math.PI / 180;
    const lat = ((a.lat + b.lat) * 0.5) * rad;
    const dlat = b.lat - a.lat;
    const dlon = (b.lon - a.lon) * Math.cos(lat);
    return Math.sqrt(dlat*dlat + dlon*dlon) * 111195;
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

