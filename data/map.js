/* global L */

window.MapView = {
  map: null,

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

    // Tile selection
    if(window.IS_LOCAL){
      L.tileLayer(
        "https://server.arcgisonline.com/ArcGIS/rest/services/" +
        "World_Imagery/MapServer/tile/{z}/{y}/{x}",
        { maxZoom:19, crossOrigin:true }
      ).addTo(this.map);
    }else{
      L.tileLayer(
        "/tiles/{z}/{x}/{y}.jpg",
        { minZoom:10, maxZoom:18, noWrap:true }
      ).addTo(this.map);
    }
  },

  loadGeoJSON(url){
    if(!this.map) return;

    fetch(url)
      .then(r=>r.json())
      .then(gj=>{
        const layer=L.geoJSON(gj,{
          style:{color:"#ff3b30",weight:3}
        }).addTo(this.map);

        const b=layer.getBounds();
        b.isValid() && this.map.fitBounds(b,{animate:false});
      })
      .catch(e=>console.warn("GeoJSON load failed",e));
  }
};
