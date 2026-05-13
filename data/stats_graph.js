// ============================================================================
// Session stats graph and stat tile state
// ============================================================================

function showStats(){
  $("sessionCard")?.classList.add("stats");
  StatsGraph.scheduleDraw();
}

function hideStats(){
  $("sessionCard")?.classList.remove("stats");
  setTimeout(() => {
    MapView.map?.invalidateSize(true);
    MapView.refitTrack?.();
  }, 220);
}

function updateStatsUI(stats){
  const set = (id, value) => {
    const el = $(id);
    if(el) el.textContent = value;
  };

  if(!stats){
    [
      "map_stat_2s",
      "map_stat_10s",
      "map_stat_alpha",
      "map_stat_nm",
      "map_stat_1h",
      "map_stat_distance"
    ].forEach(id => set(id, "00.000"));
    return;
  }

  set("map_stat_2s",       stats.max?.toFixed(3)      ?? "-");
  set("map_stat_10s",      stats.avg10?.toFixed(3)    ?? "-");
  set("map_stat_alpha",    stats.alpha?.toFixed(3)    ?? "-");
  set("map_stat_nm",       stats.nm?.toFixed(3)       ?? "-");
  set("map_stat_1h",       stats.h1?.toFixed(3)       ?? "-");
  set("map_stat_distance", stats.distance?.toFixed(3) ?? "-");
}

window.StatsGraph = {
  base:null,
  overlays:{},
  stats:null,
  chart:null,
  drawTimer:null,
  mode:"alpha",
  graphModes:new Set(["2s","10s","alpha","nm","1h","distance"]),
  colors:["#1e88e5","#ff9500","#34c759","#af52de","#ff3b30"],

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
    document.querySelectorAll(".stat").forEach(el => {
      el.classList.toggle("selected", el.dataset.mode === this.mode);
    });
    if(!options.keepCollapsed) showStats();
    this.scheduleDraw();
  },

  canGraph(mode){
    return this.graphModes.has(mode);
  },

  scheduleDraw(){
    if(this.drawTimer){
      clearTimeout(this.drawTimer);
      this.drawTimer = null;
    }

    requestAnimationFrame(() => {
      this.draw();
      this.drawTimer = setTimeout(() => this.draw(), 340);
    });
  },

  draw(){
    const el = $("statsGraph");
    if(!el) return;

    const series = this.seriesForMode(this.mode);
    if(!series.sets.length || !series.sets[0].points.length){
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
    if(!width || !height) return;

    this.destroyChart();
    el.classList.remove("empty");
    delete el.dataset.empty;
    el.textContent = "";

    const x = series.sets[0].points.map(p => p.x);
    const ys = series.sets.map(set => set.points.map(p => p.y));
    const xTicks = series.xTicks || [];

    this.chart = new uPlot({
      width,
      height,
      padding:[2, 38, 16, 8],
      legend:{show:false},
      cursor:{show:true, x:false, y:false},
      scales:{x:{time:false}},
      axes:[
        {
          stroke:"#5b6b82",
          grid:{show:false},
          ticks:{show:false},
          splits:() => xTicks.map(t => t.value),
          values:(u, vals) => vals.map(v => this.xLabelForValue(v, xTicks))
        },
        {
          stroke:"#5b6b82",
          grid:{stroke:"rgba(91,107,130,.22)", width:1},
          values:(u, vals) => vals.map(v => v.toFixed(1))
        }
      ],
      series:[
        {},
        ...series.sets.map((set, i) => ({
          label:set.label || series.unit,
          stroke:set.color || this.colors[i % this.colors.length],
          width:this.mode === "10s" ? 2 : 3,
          points:{show:false}
        }))
      ]
    }, [x, ...ys], el);

    this.drawInfo(series);
  },

  drawEmpty(text){
    const el = $("statsGraph");
    const info = $("statsInfo");
    if(!el) return;
    this.destroyChart();
    el.classList.add("empty");
    el.dataset.empty = text;
    el.textContent = "";
    if(info) info.innerHTML = "";
  },

  seriesForMode(mode){
    const values = this.base?.properties?.graphs?.[mode] || [];
    const meta = this.base?.properties?.graph_meta?.[mode] || {};
    const rawSets = Array.isArray(values?.[0]) ? values : [values];
    const axis = this.xAxisForMode(mode, rawSets[0]?.length || 0, meta);
    const sets = rawSets
      .map((set, i) => ({
        label:mode === "10s" ? `${i + 1}` : this.labelForMode(mode),
        color:this.colors[i % this.colors.length],
        points:(set || [])
          .map((v, idx) => ({ x:this.xForIndex(idx, set.length, axis.max), y:Number(v) }))
          .filter(p => Number.isFinite(p.y))
      }))
      .filter(set => set.points.length);

    return {
      label:this.labelForMode(mode),
      unit:"kt",
      xTicks:axis.ticks,
      sets
    };
  },

  drawInfo(series){
    const el = $("statsInfo");
    if(!el) return;
    el.classList.toggle("ten-sec-info", this.mode === "10s");
    el.classList.toggle("alpha-info", this.mode === "alpha");

    const pills = [];
    if(this.mode === "10s"){
      for(let i = 0; i < 5; i++){
        const set = series.sets[i] || { color:this.colors[i % this.colors.length], points:[] };
        const exact = Number(this.stats?.r10?.[i]);
        const avg = Number.isFinite(exact) && exact > 0 ? exact : this.avg(set.points);
        const value = Number.isFinite(avg) ? avg : 0;
        pills.push(`<span class="stat-info-pill stat-info-box" style="--stat-color:${set.color}"><b>${i + 1}</b><span>${value.toFixed(2)}</span></span>`);
      }
    }else{
      const points = series.sets[0]?.points || [];
      const min = this.min(points);
      const max = this.max(points);
      if(Number.isFinite(min)) pills.push(`<span class="stat-info-pill">Min ${min.toFixed(2)}</span>`);
      if(Number.isFinite(max)) pills.push(`<span class="stat-info-pill">Max ${max.toFixed(2)}</span>`);
      if(this.mode === "alpha"){
        const distance = Number(this.stats?.alphaDistance);
        const closure = Number(this.stats?.alphaClosure);
        if(Number.isFinite(distance) && distance > 0 &&
           Number.isFinite(closure) && closure > 0){
          pills.push(`<span class="stat-info-pill">Dist ${distance.toFixed(0)}m @ ${closure.toFixed(1)}m</span>`);
        }
      }
    }

    el.innerHTML = pills.join("");
  },

  values(points){
    return points.map(p => p.y).filter(Number.isFinite);
  },

  min(points){
    const vals = this.values(points);
    return vals.length ? Math.min(...vals) : NaN;
  },

  max(points){
    const vals = this.values(points);
    return vals.length ? Math.max(...vals) : NaN;
  },

  avg(points){
    const vals = this.values(points);
    return vals.length ? vals.reduce((a, b) => a + b, 0) / vals.length : NaN;
  },

  xForIndex(index, count, max){
    if(count <= 1) return 0;
    return (index / (count - 1)) * max;
  },

  xAxisForMode(mode, count, meta={}){
    const metaMax = Number(meta.xMax);
    const max = Number.isFinite(metaMax) && metaMax > 0 ? metaMax : null;

    if(mode === "2s") return this.secondAxis(max || 2, 0.2);
    if(mode === "10s") return this.secondAxis(max || 10, 1);
    if(mode === "1h") return this.minuteAxis(max || 60, true);
    if(mode === "nm") return this.metreAxis(max || 1852);
    if(mode === "alpha") return this.metreAxis(max || 500);
    if(mode === "distance") return this.minuteAxis(max || Math.max(1, count - 1), false);
    return { max:Math.max(1, count - 1), ticks:[] };
  },

  secondAxis(max, step){
    const ticks = [];
    const end = Number(max.toFixed(3));
    for(let v = 0; v < end - 0.0001; v += step){
      const value = Number(v.toFixed(3));
      ticks.push({ value, label:this.secondLabel(value, false) });
    }
    ticks.push({ value:end, label:this.secondLabel(end, true) });
    return { max:end, ticks };
  },

  secondLabel(value, forceUnit){
    if(value === 0) return "0s";
    if(forceUnit || Number.isInteger(value)) return `${this.trimNumber(value)}s`;
    return this.trimNumber(value).replace(/^0/, "");
  },

  minuteAxis(max, preferHourTicks){
    const end = Number(max.toFixed(3));
    if(preferHourTicks && end >= 60){
      return { max:end, ticks:[
        { value:0, label:"0" },
        { value:15, label:"15" },
        { value:30, label:"30 min" },
        { value:45, label:"45" },
        { value:60, label:"60 min" }
      ]};
    }
    const mid = Number((end / 2).toFixed(3));
    return { max:end, ticks:[
      { value:0, label:"0" },
      { value:mid, label:this.minuteLabel(mid) },
      { value:end, label:this.minuteLabel(end) }
    ]};
  },

  minuteLabel(value){
    if(value < 1) return `${Math.round(value * 60)}s`;
    return `${this.trimNumber(value)} min`;
  },

  metreAxis(max){
    const end = Math.round(max);
    const mid = Math.round(end / 2);
    return { max:end, ticks:[
      { value:0, label:"0m" },
      { value:mid, label:`${mid}m` },
      { value:end, label:`${end}m` }
    ]};
  },

  trimNumber(value){
    return Number(value.toFixed(1)).toString();
  },

  xLabelForValue(value, ticks){
    const tick = ticks.find(t => Math.abs(value - t.value) < 0.001);
    return tick ? tick.label : "";
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
