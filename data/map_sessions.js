// ============================================================================
// Map session list and gestures
// ============================================================================

window.MapSessions = {
  files:[],
  index:0,
  _started:false,
  _gesturesBound:false,

  async init(){
    await this.reload({ keepCurrent:false });
    this.bindGestures();
  },

  async reload(options={}){
    try{
      const j = await Api.json("/api/files");
      if(!j.ok) throw new Error("files unavailable");

      this.files = j.files
        .filter(f => f.name.endsWith(".geojson"))
        .sort(compareSessionFiles);

      if(!this.files.length){
        this.showEmpty("No sessions");
        return;
      }

      if(!options.keepCurrent) this.index = 0;
      if(this.index >= this.files.length) this.index = this.files.length - 1;
      this.loadCurrent();
    }catch(err){
      console.warn("[Map] session list failed", err);
      this.showEmpty("Sessions unavailable");
    }
  },

  showEmpty(title){
    this.index = 0;
    MapView.clear();
    updateStatsUI(null);
    const titleEl = $("sessionTitle");
    const metaEl = $("sessionMeta");
    if(titleEl) titleEl.textContent = title;
    if(metaEl) metaEl.textContent = "";
  },

  removeDeleted(name){
    if(!name) return;

    const removedIndex = this.files.findIndex(f => f.name === name || f.sbp_name === name);
    if(removedIndex < 0) return;

    const wasCurrent = removedIndex === this.index;
    this.files.splice(removedIndex, 1);

    if(!this.files.length){
      this.showEmpty("No sessions");
      return;
    }

    if(removedIndex < this.index) this.index--;
    if(this.index >= this.files.length) this.index = this.files.length - 1;

    if(wasCurrent) this.loadCurrent();
    else this.updateHeader();
  },

  updateHeader(){
    const f = this.files[this.index];
    if(!f) return;

    const total = this.files.length;
    const logical = total - this.index;
    const displayName = f.sbp_name || f.name;

    AppUtil.setText($("sessionTitle"), `Session ${logical} of ${total}`);
    AppUtil.setText($("sessionMeta"), displayName.replace(/\.(geojson|sbp|ubx|txt)$/i, ""));
  },

  loadCurrent(){
    const f = this.files[this.index];
    if(!f) return;

    this.updateHeader();
    MapView.clear();
    MapView.loadGeoJSON(`/api/download?file=${encodeURIComponent(f.name)}&t=${Date.now()}`);
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
    if(this._gesturesBound) return;
    this._gesturesBound = true;

    const card = $("sessionCard");
    let x0 = 0;
    let y0 = 0;
    let dx = 0;
    let dy = 0;
    let active = false;
    let locked = null;
    const threshold = 40;

    card.addEventListener("touchstart", e => {
      const t = e.touches[0];
      x0 = t.clientX;
      y0 = t.clientY;
      dx = 0;
      dy = 0;
      locked = null;
      active = true;
    }, { passive:true });

    card.addEventListener("touchmove", e => {
      if(!active) return;

      const t = e.touches[0];
      dx = t.clientX - x0;
      dy = t.clientY - y0;

      if(!locked){
        if(Math.abs(dx) > 12) locked = "x";
        else if(Math.abs(dy) > 12) locked = "y";
        else return;
      }

      e.preventDefault();
    }, { passive:false });

    card.addEventListener("touchend", () => {
      if(!active) return;
      active = false;

      if(locked === "y"){
        if(dy < -threshold) showStats();
        else if(dy > threshold) hideStats();
        return;
      }
      if(locked === "x"){
        if(dx < -threshold) this.next();
        else if(dx > threshold) this.prev();
      }
    });
  }
};

function compareSessionFiles(a, b){
  const am = Number(a.mtime || 0);
  const bm = Number(b.mtime || 0);
  if(am || bm){
    if(bm !== am) return bm - am;
  }
  return b.name.localeCompare(a.name);
}
