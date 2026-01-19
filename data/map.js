// ============================================================================
// MapView
//
// Responsibility:
// - Owns the Leaflet map instance
// - Manages exactly ONE visible track at a time

// Running on PC / localhost (dev mode)



window.MapView = {
  map:null,       // Leaflet map instance
  track:null,     // Currently loaded GeoJSON track layer
  dot:null,       // Small dot marking last point of the track
  _r:null,        // Shared Canvas renderer (faster than SVG / DOM)

  // -------------------------------------------------------------------------
  // init()
  // Creates the map ONCE.

  init(){
    // Map already exists → just refresh layout
    if(this.map){
      this.map.invalidateSize(true);
      return;
    }

    // DOM element that hosts the map
    const el = document.getElementById("mapView");

    // Create Leaflet map with ESP / mobile-friendly options
    this.map = L.map(el,{
      zoomControl:false,        // no +/- buttons
      attributionControl:true,  // keep attribution text
      inertia:false,            // predictable movement on touch
      preferCanvas:true         // force Canvas over SVG
    });

    // Create ONE canvas renderer reused by all vector layers
    // This avoids multiple canvas contexts and improves performance
    this._r = L.canvas({ padding:0.5 });

    // -----------------------------------------------------------------------
    // Base layer selection
    //
    // PC (IS_LOCAL):
    //   - Online satellite imagery (Esri / SRI-style)
    //   - Used for development, debugging, UX tuning
    //
    // ESP32:
    //   - Dummy transparent tiles
    //   - Prevents white flash + avoids network requests
    // -----------------------------------------------------------------------
    if(IS_LOCAL){

      // PC DEV: Esri World Imagery (satellite)
      L.tileLayer(
        "https://server.arcgisonline.com/ArcGIS/rest/services/" +
        "World_Imagery/MapServer/tile/{z}/{y}/{x}",
        {
          maxZoom:12,
          attribution:"© Esri"
        }
      ).addTo(this.map);

       }else{

      // ---------------------------------------------------------------
      // ESP OFFLINE MODE
      //
      // Uses locally stored tiles:
      //   /tiles/{z}/{x}/{y}.jpg
      //
      // Falls back to transparent tiles if missing
      // ---------------------------------------------------------------

      const blank =
        "data:image/gif;base64,R0lGODlhAQABAAD/ACwAAAAAAQABAAACADs=";

      const OfflineTiles = L.TileLayer.extend({
        getTileUrl(c){
          return `/tiles/${c.z}/${c.x}/${c.y}.jpg`;
        },
        createTile(c, done){
          const img = document.createElement("img");
          img.width = img.height = 256;

          img.onload  = ()=>done(null, img);
          img.onerror = ()=>{
            // Missing tile → transparent fallback
            img.src = blank;
            done(null, img);
          };

          img.src = this.getTileUrl(c);
          return img;
        }
      });

      new OfflineTiles({
        minZoom:14,
        maxZoom:15,
        attribution:"© ESP32 GPS (offline)"
      }).addTo(this.map);
    }


        // -----------------------------------------------------------------------
        // Initial view
        //
        // This is only a placeholder.
        // Once a track loads, fitBounds() will override this.
        // Timeout ensures DOM + CSS layout is stable.
        // -----------------------------------------------------------------------
        setTimeout(()=>{
          this.map.setView([-32.0,115.8],13);
          this.map.invalidateSize(true);

          // Start session browser only AFTER map exists
          if(window.MapSessions && !window.MapSessions._started){
            window.MapSessions._started = true;
            console.log("[Map] starting MapSessions");
            window.MapSessions.init();
          }
        },150);

        console.log("[Map] init OK, size =", el.offsetWidth, el.offsetHeight);
      },

  // -------------------------------------------------------------------------
  // loadGeoJSON(url)
  //
  // Loads a single GeoJSON file and renders:
  // - the track polyline
  //
  // Only ONE track is ever active at a time.
  // -------------------------------------------------------------------------
  loadGeoJSON(url){
    if(!this.map) return;

    // Remove previously displayed layers
    if(this.track){
      this.map.removeLayer(this.track);
      this.track = null;
    }

    // Fetch GeoJSON directly from ESP
    fetch(url,{cache:"no-store"})
      .then(r=>{
        if(!r.ok) throw Error("GeoJSON fetch failed");
        return r.json();
      })
      .then(gj=>{
        console.log("[Map] feature count", gj.features?.length);

        // ---------------------------------------------------------------
        // Track polyline
        //
        // Important detail:
        // - GPS GeoJSON coordinates are [lon, lat]
        // - Leaflet expects [lat, lon]
        // ---------------------------------------------------------------
        this.track = L.geoJSON(gj,{
          renderer:this._r,
          coordsToLatLng:c=>L.latLng(c[1],c[0]),
          style:{
            color:"#ff3b30",
            weight:5,
            opacity:1
          }
        }).addTo(this.map);

        // Ensure track renders above base layer
        this.track.bringToFront();

        // ---------------------------------------------------------------
        // Fit map to track bounds
        //
        // - Padding prevents UI overlap
        // - maxZoom avoids extreme zoom on short tracks
        // ---------------------------------------------------------------
        const b = this.track.getBounds();
        if(b.isValid()){
          this.map.fitBounds(b,{
            padding:[30,30],
            animate:false,
            maxZoom:16
          });

          // iOS sometimes needs a second layout pass
          setTimeout(()=>this.map.invalidateSize(true),50);
        }
     })
      .catch(e=>console.warn("[Map] GeoJSON failed", e));
  },

  // -------------------------------------------------------------------------
  // clear()
  //
  // Removes all dynamic map content.
  // Used when switching sessions.
  // -------------------------------------------------------------------------
  clear(){
    if(this.track){
      this.map.removeLayer(this.track);
      this.track = null;
    }
    if(this.dot){
      this.map.removeLayer(this.dot);
      this.dot = null;
    }
  }
};


// ============================================================================
// MapSessions
//
// Responsibility:
// - Fetch available GeoJSON session files from ESP
// - Maintain a current index
// - Allow swipe-based navigation between sessions
// ============================================================================

window.MapSessions = {
  files:[],          // list of GeoJSON files
  index:0,           // currently selected file index
  _started:false,    // guard to prevent double init

  // -------------------------------------------------------------------------
  // init()
  //
  // Fetches file list and prepares first session.
  // -------------------------------------------------------------------------
  async init(){
    console.log("[MapSessions] init()");
    const r = await fetch("/api/files",{cache:"no-store"});
    const j = await r.json();
    if(!j.ok) return;

    // Only keep GeoJSON sessions, newest first
    this.files = j.files
      .filter(f=>f.name.endsWith(".geojson"))
      .sort((a,b)=>b.name.localeCompare(a.name));

    if(!this.files.length) return;

    this.index = 0;
    this.loadCurrent();
    this.bindGestures();
  },

  // -------------------------------------------------------------------------
  // loadCurrent()
  //
  // Loads the GeoJSON file at current index
  // -------------------------------------------------------------------------
  loadCurrent(){
    const f = this.files[this.index];
    if(!f) return;

   const total = this.files.length;
    const logical = total - this.index;

    $("sessionTitle").textContent = `Session ${logical} of ${total}`;
    $("sessionMeta").textContent = f.name.replace(".geojson","");

    MapView.clear();
    MapView.loadGeoJSON(`/api/download?file=${encodeURIComponent(f.name)}` );
  },

  // Move backward in time
  prev(){
    if(this.index < this.files.length-1){
      this.index++;
      this.loadCurrent();
    }
  },

  // Move forward in time
  next(){
    if(this.index > 0){
      this.index--;
      this.loadCurrent();
    }
  },

  
   bindGestures(){
  const card = $("sessionCard");
  let x0=0,y0=0,dx=0,dy=0,active=false,locked=null;

  const THRESH = 40;

  card.addEventListener("touchstart",e=>{
    const t = e.touches[0];
    x0 = t.clientX;
    y0 = t.clientY;
    dx = dy = 0;
    locked = null;
    active = true;
  },{passive:true});

  card.addEventListener("touchmove",e=>{
    if(!active) return;

    const t = e.touches[0];
    dx = t.clientX - x0;
    dy = t.clientY - y0;

    // Decide intent once
    if(!locked){
      if(Math.abs(dx) > 12) locked = "x";
      else if(Math.abs(dy) > 12) locked = "y";
      else return;
    }

    // Block page scroll only for vertical gestures
    if(locked === "y") e.preventDefault();
  },{passive:false});   // ⚠️ MUST be false

  card.addEventListener("touchend",()=>{
    if(!active) return;
    active = false;

    if(locked === "y"){
      if(dy < -THRESH) showStats();
      else if(dy > THRESH) hideStats();
      return;
    }

    if(locked === "x"){
      if(dx < -THRESH) MapSessions.prev();
      else if(dx > THRESH) MapSessions.next();
    }
  });
}

};


function showStats(){
  sessionCard.classList.add("stats");
}

function hideStats(){
  sessionCard.classList.remove("stats");
  MapView.map?.invalidateSize(true);
}


