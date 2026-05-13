// ============================================================================
// ESP32 GPS homescreen cache
// ============================================================================

const CACHE_NAME = "esp32-gps-v65";

const APP_SHELL = [
  "/",
  "/index.html",
  "/app.css?v=57",
  "/map.css?v=60",
  "/leaflet.css",
  "/uPlot.min.css",
  "/leaflet.js",
  "/uPlot.iife.min.js",
  "/utils.js?v=64",
  "/files.js?v=64",
  "/config.js?v=64",
  "/system.js?v=64",
  "/stats_graph.js?v=64",
  "/map_view.js?v=64",
  "/map_sessions.js?v=64",
  "/map.js?v=64",
  "/app_init.js?v=64",
  "/pwa.js?v=65",
  "/ESP32_logo.jpg",
  "/manifest.webmanifest"
];

self.addEventListener("install", event => {
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then(cache => cache.addAll(APP_SHELL))
      .then(() => self.skipWaiting())
  );
});

self.addEventListener("activate", event => {
  event.waitUntil(
    caches.keys()
      .then(keys => Promise.all(
        keys.filter(key => key !== CACHE_NAME).map(key => caches.delete(key))
      ))
      .then(() => self.clients.claim())
  );
});

self.addEventListener("fetch", event => {
  const request = event.request;
  if(request.method !== "GET") return;

  const url = new URL(request.url);
  if(url.origin !== self.location.origin) return;

  if(url.pathname === "/api/files"){
    event.respondWith(networkFirst(request, normalizedRequest(url)));
    return;
  }

  if(url.pathname === "/api/download" && isGeojsonRequest(url)){
    event.respondWith(networkFirst(request, normalizedRequest(url)));
    return;
  }

  event.respondWith(cacheFirst(request));
});

function isGeojsonRequest(url){
  return (url.searchParams.get("file") || "").toLowerCase().endsWith(".geojson");
}

function normalizedRequest(url){
  const clean = new URL(url);
  clean.searchParams.delete("t");
  return new Request(clean.toString(), { credentials:"same-origin" });
}

async function cacheFirst(request){
  const cached = await caches.match(request);
  if(cached) return cached;
  return fetch(request);
}

async function networkFirst(request, cacheKey){
  const cache = await caches.open(CACHE_NAME);
  try{
    const response = await fetch(request);
    if(response.ok) cache.put(cacheKey, response.clone());
    return response;
  }catch(err){
    const cached = await cache.match(cacheKey);
    if(cached) return cached;
    throw err;
  }
}
