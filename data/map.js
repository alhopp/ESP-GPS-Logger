// ============================================================================
// MapView
// - ESP-hosted Leaflet map (STA mode)
// - Displays exactly ONE session track at a time
// ============================================================================

window.MapView = {
  map:null, track:null, dot:null, _r:null,

  // -------------------------------------------------------------------------
  // init() — create map once
  // -------------------------------------------------------------------------
  init(){
    if(this.map){ this.map.invalidateSize(true); return; }

    const el = $("mapView");

    this.map = L.map(el,{
      zoomControl:false,
      attributionControl:true,
      inertia:false,
      preferCanvas:true,
      minZoom:14,maxZoom:16
    });

    // Shared canvas renderer (single context)
    this._r = L.canvas({padding:0.5});

    // Base imagery (STA internet)
    L.tileLayer(
      "https://server.arcgisonline.com/ArcGIS/rest/services/" +
      "World_Imagery/MapServer/tile/{z}/{y}/{x}",
      {maxZoom:16,attribution:"© Esri"}
    ).addTo(this.map);

    // Placeholder view (overridden when session loads)
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
  // zoomToLayer() — fit bounds + clamp zoom
  // -------------------------------------------------------------------------
  zoomToLayer(layer){
    if(!layer) return;
    const b = layer.getBounds();
    if(!b.isValid()) return;

    this.map.fitBounds(b,{padding:[30,30],animate:false});

    const z = this.map.getZoom();
    if(z > this.map.options.maxZoom) this.map.setZoom(this.map.options.maxZoom);
    if(z < this.map.options.minZoom) this.map.setZoom(this.map.options.minZoom);
  },

  // -------------------------------------------------------------------------
  // loadGeoJSON() — load one ESP-served track
  // -------------------------------------------------------------------------
  loadGeoJSON(url){
    if(!this.map) return;

    if(this.track){
      this.map.removeLayer(this.track);
      this.track = null;
    }

    fetch(url,{cache:"no-store"})
      .then(r=>{ if(!r.ok) throw Error("GeoJSON fetch failed"); return r.json(); })
      .then(gj=>{
        const feature = gj.features?.[0];
        const stats   = feature?.properties?.stats;

        // Update stats panel
        updateStatsUI(stats);

        // Draw geometry
        this.track = L.geoJSON(gj,{
          renderer:this._r,
          coordsToLatLng:c=>L.latLng(c[1],c[0]),
          style:{color:"#ff3b30",weight:5,opacity:1}
        }).addTo(this.map);

        this.track.bringToFront();
        this.zoomToLayer(this.track);
        setTimeout(()=>this.map.invalidateSize(true),50);
      })

      .catch(e=>console.warn("[Map] GeoJSON failed",e));
  },

  // -------------------------------------------------------------------------
  // clear() — remove dynamic layers
  // -------------------------------------------------------------------------
  clear(){
    if(this.track){ this.map.removeLayer(this.track); this.track=null; }
    if(this.dot){ this.map.removeLayer(this.dot); this.dot=null; }
  }
};


// ============================================================================
// MapSessions
// - Fetch available GeoJSON sessions from ESP
// - Maintain index
// - Swipe navigation
// ============================================================================

window.MapSessions = {
  files:[], index:0, _started:false,

  // -------------------------------------------------------------------------
  async init(){
    const r = await fetch("/api/files",{cache:"no-store"});
    const j = await r.json();
    if(!j.ok) return;

    // GeoJSON only, newest first
    this.files = j.files
      .filter(f=>f.name.endsWith(".geojson"))
      .sort((a,b)=>b.name.localeCompare(a.name));

    if(!this.files.length) return;

    this.index = 0;
    this.loadCurrent();
    this.bindGestures();
  },

  // -------------------------------------------------------------------------
  loadCurrent(){
    const f = this.files[this.index];
    if(!f) return;

    const total = this.files.length;
    const logical = total - this.index;

    $("sessionTitle").textContent = `Session ${logical} of ${total}`;
    $("sessionMeta").textContent = f.name.replace(".geojson","");

    MapView.clear();
    MapView.loadGeoJSON(`/api/download?file=${encodeURIComponent(f.name)}`);
  },

  prev(){ if(this.index < this.files.length-1){ this.index++; this.loadCurrent(); } },
  next(){ if(this.index > 0){ this.index--; this.loadCurrent(); } },

  // -------------------------------------------------------------------------
  // bindGestures()
  // - Horizontal swipe → session change
  // - Vertical swipe → stats panel
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

      if(locked==="y") e.preventDefault();
    },{passive:false}); // must be false

    card.addEventListener("touchend",()=>{
      if(!active) return;
      active=false;

      if(locked==="y"){
        if(dy < -THRESH) showStats();
        else if(dy > THRESH) hideStats();
        return;
      }

      if(locked==="x"){
        if(dx < -THRESH) this.prev();
        else if(dx > THRESH) this.next();
      }
    });
  }
};


// ============================================================================
// Stats helpers
// ============================================================================

function showStats(){
  sessionCard.classList.add("stats");
}

function hideStats(){
  sessionCard.classList.remove("stats");
  MapView.map?.invalidateSize(true);
}

function updateStatsUI(stats){
  const fmt = (v, d=1) =>
    (typeof v === "number" && isFinite(v)) ? v.toFixed(d) : "–";

  $("stat_nm").textContent       = fmt(stats?.nm, 2);
  $("stat_alpha").textContent    = fmt(stats?.alpha, 1);
  $("stat_1h").textContent       = fmt(stats?.h1, 1);
  $("stat_max").textContent      = fmt(stats?.max, 1);
  $("stat_10s").textContent      = fmt(stats?.avg10, 1);
  $("stat_distance").textContent = fmt(stats?.distance, 2);
}

