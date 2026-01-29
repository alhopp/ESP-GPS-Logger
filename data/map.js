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
      attributionControl:false,
      inertia:false,
      preferCanvas:true,
      minZoom:5,
      maxZoom:22
    });

    // Shared canvas renderer (single context)
    this._r = L.canvas({padding:0.5});

    L.tileLayer(
      "https://server.arcgisonline.com/ArcGIS/rest/services/" +
      "World_Imagery/MapServer/tile/{z}/{y}/{x}",
      {
        maxNativeZoom:18,
        maxZoom:22,
        attribution:"© Esri"
      }
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
      .then(r=>{
        if(!r.ok) throw Error("GeoJSON fetch failed");
        return r.json();
      })
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

    this.files = j.files
      .filter(f=>f.name.endsWith(".geojson"))
      .sort((a,b)=>b.name.localeCompare(a.name));

    if(!this.files.length) return;

    this.index = 0;
    this.loadCurrent();
    this.bindGestures();
  },

  // -------------------------------------------------------------------------
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

  // -------------------------------------------------------------------------
  loadCurrent(){
    const f = this.files[this.index];
    if(!f) return;

    const total   = this.files.length;
    const logical = total - this.index;

    $("sessionTitle").textContent = `Session ${logical} of ${total}`;
    $("sessionMeta").textContent  = f.name.replace(".geojson","");

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
}

function hideStats(){
  $("sessionCard").classList.remove("stats");
  MapView.map?.invalidateSize(true);
}


// ---------------------------------------------------------------------------
// updateStatsUI()
// - Updates single stats grid (no preview duplication)
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
