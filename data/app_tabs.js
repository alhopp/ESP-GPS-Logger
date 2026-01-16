// app_tabs.js
function tab(id, btn){
  document.querySelectorAll("nav button, section")
    .forEach(e => e.classList.remove("a"));

  btn.classList.add("a");
  document.getElementById(id).classList.add("a");

  if(id==="map" && window.MapView){
    MapView.init();
  }
}
