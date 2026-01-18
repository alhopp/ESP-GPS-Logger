// ============================================================================
// MapView
// ============================================================================
console.log("[Map] leaflet L =", window.L);


window.MapView = {
  map: null,
  track: null,

 init(){
  if(this.map){
    this.map.invalidateSize(true);
    return;
  }

  const el = document.getElementById("mapView");

  this.map = L.map(el,{
    zoomControl:false,
    attributionControl:true,
    inertia:false
  });

  // Dummy transparent base layer (required for iOS rendering)
  L.tileLayer("",{attribution: "© ESP32 GPS",opacity: 0}).addTo(this.map);


  L.circleMarker([-32.0,115.8],{
    radius:8,
    color:"#ff3b30",
    weight:2,
    fill:true,
    fillColor:"#ff3b30",
    fillOpacity:1
  }).addTo(this.map);




  // 🔑 Delay view until container is visible
  setTimeout(()=>{
    this.map.setView([-32.0,115.8],13);
    this.map.invalidateSize(true);
  }, 150);

  console.log("[Map] init OK, size =", el.offsetWidth, el.offsetHeight);
},


loadGeoJSON(url){
  if(!this.map) return;

  if(this.track){
    this.map.removeLayer(this.track);
    this.track = null;
  }

  fetch(url,{ cache:"no-store" })
    .then(r=>{
      if(!r.ok) throw new Error("GeoJSON fetch failed");
      return r.json();
    })
    .then(gj=>{
      console.log("[Map] GeoJSON loaded", gj);

      this.track = L.geoJSON(gj,{
        coordsToLatLng: c => L.latLng(c[1], c[0]),
        style:{ color:"#ff3b30", weight:3 }
      }).addTo(this.map);

      // 🔑 iOS paint fix sequence
      requestAnimationFrame(()=>{
        this.map.invalidateSize(true);

        const b = this.track.getBounds();
        console.log("[Map] bounds", b);

        if(b.isValid()){
          this.map.fitBounds(b,{
            padding:[20,20],
            animate:false,
            maxZoom:17
          });
        }
      });
    })
    .catch(e=>console.warn("GeoJSON load failed", e));
},







  /* -------------------------------------------------------------------------
  // Initialise map (idempotent)
  // -------------------------------------------------------------------------
  init(){
    if(this.map){
      setTimeout(()=>this.map.invalidateSize(),0);
      return;
    }

    this.map = L.map("mapView",{
      zoomControl:false,
      attributionControl:false,
      inertia:false
    }).setView([-32.0,115.8],13);

    // Solid background (never white)
    this.map.createPane("bg");
    const bg = this.map.getPane("bg");
    bg.style.background = "#dbdbee";
    bg.style.zIndex = 200;

    // Online tiles (PC dev)
    if(window.IS_LOCAL){
      L.tileLayer(
        "https://server.arcgisonline.com/ArcGIS/rest/services/" +
        "World_Imagery/MapServer/tile/{z}/{y}/{x}",
        { maxZoom:19, crossOrigin:true }
      ).addTo(this.map);
      return;
    }

    // Offline tiles (ESP)
    const offline = L.tileLayer("/tiles/{z}/{x}/{y}.jpg",{
      minZoom:10,
      maxZoom:18,
      noWrap:true,
      errorTileUrl:
        "data:image/gif;base64,R0lGODlhAQABAAD/ACwAAAAAAQABAAACADs=",
      updateWhenIdle:true,
      keepBuffer:0,
      reuseTiles:true,
      opacity:0.001
    });

    offline.addTo(this.map);

    this.map.getPane("tilePane").style.zIndex = 300;
    this.map.getPane("overlayPane").style.zIndex = 400;
  },

  */

  /* -------------------------------------------------------------------------
  // Load a GeoJSON track (single active session)
  // -------------------------------------------------------------------------



  loadGeoJSON(url){
  if(!this.map) return;

  if(this.track){
    this.map.removeLayer(this.track);
    this.track = null;
  }

  fetch(url,{cache:"no-store"})
    .then(r=>r.json())
    .then(gj=>{
      this.track = L.geoJSON(gj,{
        // 🔑 GPS GeoJSON is [lon, lat]
        coordsToLatLng: c => L.latLng(c[1], c[0]),
        style:{ color:"#ff3b30", weight:3 }
      }).addTo(this.map);

      const b = this.track.getBounds();
      if(b.isValid()){
        this.map.fitBounds(b,{
          padding:[20,20],
          animate:false,
          maxZoom:17   // prevents over-zoom on short tracks
        });
      }
    })
    .catch(e=>console.warn("GeoJSON load failed", e));
},

*/

  clear(){
    if(this.track){
      this.map.removeLayer(this.track);
      this.track = null;
    }
  }
};



// ============================================================================
// MapSessions (session browser + swipe control)
// ============================================================================

window.MapSessions = {
  files: [],
  index: 0,

  async init(){
    const r = await fetch("/api/files",{cache:"no-store"});
    const j = await r.json();
    if(!j.ok) return;

    this.files = j.files
      .filter(f => f.name.endsWith(".geojson"))
      .sort((a,b)=>b.name.localeCompare(a.name));

    if(!this.files.length) return;

    this.index = 0;
    this.loadCurrent();
    this.bindGestures();
  },

  loadCurrent(){
    const f = this.files[this.index];
    if(!f) return;

    document.getElementById("sessionTitle").textContent =
      `Session ${this.index+1} of ${this.files.length}`;

    document.getElementById("sessionMeta").textContent =
      f.name.replace(".geojson","");

    MapView.clear();
    MapView.loadGeoJSON(
      `/api/download?file=${encodeURIComponent(f.name)}`
    );
  },

  prev(){
    if(this.index < this.files.length - 1){
      this.index++;
      this.loadCurrent();
    }
  },

  next(){
    if(this.index > 0){
      this.index--;
      this.loadCurrent();
    }
  },

  bindGestures(){
    const card = document.getElementById("sessionCard");
    let x0 = 0, dx = 0, active = false;

    card.addEventListener("touchstart", e=>{
      x0 = e.touches[0].clientX;
      dx = 0;
      active = true;
    },{passive:true});

    card.addEventListener("touchmove", e=>{
      if(!active) return;
      dx = e.touches[0].clientX - x0;
    },{passive:true});

    card.addEventListener("touchend", ()=>{
      active = false;
      if(dx < -40) this.prev();
      else if(dx > 40) this.next();
    },{passive:true});
  }
};
