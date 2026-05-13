// ============================================================================
// Map page coordinator
// ============================================================================

window.addEventListener("resize", () => StatsGraph.scheduleDraw());

document.querySelectorAll(".stat").forEach(stat => {
  stat.addEventListener("click", () => {
    const mode = stat.dataset.mode;
    if(!mode) return;
    MapView.showOverlay(mode);
    StatsGraph.select(mode);
  });
});
