/* global L */

// ============================================================================
// MapView — Leaflet map wrapper
// ============================================================================

window.MapView = {
  map: null,

  // --------------------------------------------------------------------------
  // Init map (call once, when Map tab is opened)
  // --------------------------------------------------------------------------
  init(){
    if (this.map) {
      // Leaflet needs a resize nudge when shown after being hidden
      setTimeout(() => this.map.invalidateSize(), 0);
      return;
    }

    // Create map
    this.map = L.map("mapView", {
      zoomControl: true,
      attributionControl: false,
      inertia: false
    }).setView([-32.0, 115.8], 13);   // ← change to your spot later

    // Base tiles (offline or local)
    L.tileLayer("/tiles/{z}/{x}/{y}.jpg", {
      minZoom: 10,
      maxZoom: 18,
      noWrap: true
    }).addTo(this.map);
  },

  // --------------------------------------------------------------------------
  // Load a GeoJSON track (optional, future use)
  // --------------------------------------------------------------------------
  loadGeoJSON(url){
    if (!this.map) return;

    fetch(url)
      .then(r => r.json())
      .then(gj => {
        const layer = L.geoJSON(gj, {
          style: { color:"#ff3b30", weight:3 }
        }).addTo(this.map);

        const b = layer.getBounds();
        if (b.isValid()) this.map.fitBounds(b, { animate:false });
      })
      .catch(err => console.warn("GeoJSON load failed", err));
  }
};
