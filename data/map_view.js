// ============================================================================
// Leaflet map view
// ============================================================================

window.MapView = {
  map:null,
  baseTrack:null,
  overlay:null,
  defaultOverlays:{},
  trackFitBounds:null,
  _r:null,
  _overlays:{},
  _loadToken:0,

  init(){
    if(this.map){
      this.map.invalidateSize(true);
      return;
    }

    const el = $("mapView");
    this.map = L.map(el, {
      zoomControl:false,
      attributionControl:false,
      inertia:false,
      preferCanvas:true,
      minZoom:5,
      maxZoom:22
    });

    this._r = L.canvas({padding:0.5});
    this.createPanes();

    L.tileLayer(
      "https://server.arcgisonline.com/ArcGIS/rest/services/" +
      "World_Imagery/MapServer/tile/{z}/{y}/{x}",
      {
        maxNativeZoom:18,
        maxZoom:22,
        attribution:"Esri"
      }
    )
      .on("tileerror", () => {
        if($("sessionMeta") && !$("sessionMeta").textContent){
          $("sessionMeta").textContent = "Map imagery unavailable offline";
        }
      })
      .addTo(this.map);

    setTimeout(() => {
      this.map.setView([0,0], 14);
      this.map.invalidateSize(true);

      if(window.MapSessions && !MapSessions._started){
        MapSessions._started = true;
        MapSessions.init();
      }
    }, 150);
  },

  createPanes(){
    [
      ["basePane", 400],
      ["overlayPane", 450],
      ["overlayNmPane", 455],
      ["overlayAlphaPane", 460],
      ["overlay10Pane", 465],
      ["overlay2Pane", 470]
    ].forEach(([name, zIndex]) => {
      this.map.createPane(name);
      this.map.getPane(name).style.zIndex = zIndex;
    });
  },

  zoomToBounds(bounds){
    const b = bounds;
    if(!b.isValid()) return;
    this.trackFitBounds = b;

    const bottomPadding = this.bottomOverlayPadding();

    this.map.fitBounds(b, {
      paddingTopLeft:[30,30],
      paddingBottomRight:[30,bottomPadding],
      animate:false,
      maxZoom:16
    });

    const z = this.map.getZoom();
    if(z > 16) this.map.setZoom(16);
    if(z < 13) this.map.setZoom(13);
  },

  bottomOverlayPadding(){
    const card = $("sessionCard");
    if(!card || card.classList.contains("stats")) return 30;
    return Math.ceil(card.getBoundingClientRect().height + 34);
  },

  refitTrack(){
    if(!this.map || !this.trackFitBounds) return;
    this.zoomToBounds(this.trackFitBounds);
  },

  trackBounds(feature){
    const coords = feature?.geometry?.coordinates || [];
    const bounds = L.latLngBounds([]);
    let count = 0;

    coords.forEach(c => {
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

  async loadGeoJSON(url, options={}){
    if(!this.map) return;
    const token = ++this._loadToken;

    try{
      const gj = await Api.json(url);
      if(token !== this._loadToken) return;
      if(!gj.features) throw new Error("no features");

      const overlays = {};
      const base = gj.features.find(f => f.properties?.mode === "track");

      gj.features.forEach(f => {
        const mode = f.properties?.mode;
        if(!mode || mode === "track") return;
        if(!f.geometry?.coordinates || f.geometry.coordinates.length < 2) return;

        if(!overlays[mode]) overlays[mode] = [];
        overlays[mode].push(f);
      });

      this.clear({ preserveStats:!!options.preserveStats });
      this._overlays = overlays;

      const stats = base?.properties?.stats;
      updateStatsUI(stats);
      StatsGraph.setSession(base, this._overlays, stats);

      if(base){
        const tb = this.trackBounds(base);
        this.baseTrack = L.geoJSON(base, {
          pane:"basePane",
          renderer:this._r,
          coordsToLatLng:c => L.latLng(c[1], c[0]),
          style:{ color:"#9aa0a6", weight:4, opacity:0.75 }
        }).addTo(this.map);

        this.zoomToBounds(tb.bounds);
      }

      this.showOverlay("nm", { pinned:true });
      this.showOverlay("alpha", { pinned:true });
      this.showOverlay("10s", { pinned:true });
      this.showOverlay("2s", { pinned:true });

      setTimeout(() => this.map.invalidateSize(true), 50);
    }catch(err){
      if(token !== this._loadToken) return;
      console.warn("[Map] GeoJSON failed", err);
      updateStatsUI(null);
      StatsGraph.drawEmpty("Session map failed to load");
      AppUtil.setText($("sessionMeta"), "Session map failed to load");
    }
  },

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
      { type:"FeatureCollection", features:list },
      {
        pane,
        renderer:this._r,
        coordsToLatLng:c => L.latLng(c[1], c[0]),
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

  clear(options={}){
    if(this.baseTrack){
      this.map.removeLayer(this.baseTrack);
      this.baseTrack = null;
    }
    if(this.overlay){
      this.map.removeLayer(this.overlay);
      this.overlay = null;
    }
    Object.keys(this.defaultOverlays).forEach(k => {
      if(this.defaultOverlays[k]) this.map.removeLayer(this.defaultOverlays[k]);
    });
    this.defaultOverlays = {};
    this.trackFitBounds = null;
    this._overlays = {};
    if(!options.preserveStats) StatsGraph.clear();
  }
};
