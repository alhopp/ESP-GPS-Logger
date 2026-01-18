// -----------------------------------------------------------------------------
// MapView
// - Lazy initialised when Map tab opens
// - Always shows a background (never blank)
// - Uses ESRI tiles on PC, offline tiles on ESP
// -----------------------------------------------------------------------------
window.MapView={
  map:null,

  // ---------------------------------------------------------------------------
  // Initialise map (idempotent)
  // ---------------------------------------------------------------------------
  init(){
    if(this.map){ setTimeout(()=>this.map.invalidateSize(),0); return; }

    // Create map
    this.map=L.map("mapView",{
      zoomControl:false,
      attributionControl:false,
      inertia:false
    }).setView([-32.0,115.8],13);

    // -------------------------------------------------------------------------
    // Solid background pane (prevents white screen if no tiles)
    // -------------------------------------------------------------------------
    this.map.createPane("bg");
    const bg=this.map.getPane("bg");
    bg.style.background="#dbdbee";   // app theme background
    bg.style.zIndex=200;

    // -------------------------------------------------------------------------
    // Online tiles (PC dev only)
    // -------------------------------------------------------------------------
    if(window.IS_LOCAL){
      L.tileLayer(
        "https://server.arcgisonline.com/ArcGIS/rest/services/" +
        "World_Imagery/MapServer/tile/{z}/{y}/{x}",
        { maxZoom:19, crossOrigin:true }
      ).addTo(this.map);
      return;
    }

    // -------------------------------------------------------------------------
    // Offline tiles (ESP32 / SD / LittleFS)
    // - Missing tiles silently fall back to background
    // -------------------------------------------------------------------------
    const offline = L.tileLayer("/tiles/{z}/{x}/{y}.jpg",{
      minZoom:10,
      maxZoom:18,
      noWrap:true,

      // Prevent retries + broken icons
      errorTileUrl:"data:image/gif;base64,R0lGODlhAQABAAD/ACwAAAAAAQABAAACADs=",

      // Tile engine tuning (ESP32-friendly)
      updateWhenIdle:true,
      keepBuffer:0,
      reuseTiles:true,

      // Tile layer exists but visually gone
      opacity:0.001
    });



    offline.on("tileerror",()=>{
      console.warn("Offline tiles missing – background only");
    });

    offline.addTo(this.map);
    this.map.getPane("overlayPane").style.zIndex=400;
    this.map.getPane("tilePane").style.zIndex=300;

  },

  // ---------------------------------------------------------------------------
  // Load and display a GeoJSON track
  // ---------------------------------------------------------------------------
  loadGeoJSON(url){
    if(!this.map) return;

    fetch(url)
      .then(r=>r.json())
      .then(gj=>{
        const l=L.geoJSON(gj,{style:{color:"#ff3b30",weight:3}}).addTo(this.map);
        const b=l.getBounds(); b.isValid()&&this.map.fitBounds(b,{animate:false});
      })
      .catch(e=>console.warn("GeoJSON load failed",e));
  }
};
