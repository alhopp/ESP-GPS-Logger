// ============================================================================
// Homescreen / PWA bootstrap
// ============================================================================

(function(){
  if(!("serviceWorker" in navigator)) return;

  window.addEventListener("load", () => {
    navigator.serviceWorker.register("/sw.js")
      .catch(err => console.warn("[PWA] service worker unavailable", err));
  });
})();
