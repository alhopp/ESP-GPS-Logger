// ============================================================================
// Shared frontend helpers
// ============================================================================

window.$ = id => document.getElementById(id);

window.AppUtil = {
  escapeHtml(value){
    return String(value ?? "").replace(/[&<>"']/g, ch => ({
      "&":"&amp;",
      "<":"&lt;",
      ">":"&gt;",
      "\"":"&quot;",
      "'":"&#39;"
    })[ch]);
  },

  formatStorageSize(bytes, fallbackMb){
    const value = Number(bytes);
    if(Number.isFinite(value) && value > 0){
      if(value < 1024 * 1024){
        return `${(value / 1024).toFixed(1)} kB`;
      }
      return `${(value / (1024 * 1024)).toFixed(1)} MB`;
    }

    const mb = Number(fallbackMb);
    return Number.isFinite(mb) ? `${mb} MB` : null;
  },

  setText(el, value){
    if(!el) return;
    el.textContent = (value !== undefined && value !== null && value !== "") ? value : "-";
  }
};

window.Api = {
  async json(url, options={}){
    const response = await fetch(url, { cache:"no-store", ...options });
    if(!response.ok) throw new Error(`${url} failed: ${response.status}`);
    return response.json();
  },

  async postJson(url, payload){
    const response = await fetch(url, {
      method:"POST",
      headers:{ "Content-Type":"application/json" },
      body:JSON.stringify(payload),
      cache:"no-store"
    });
    if(!response.ok) throw new Error(`${url} failed: ${response.status}`);
    return response;
  },

  async post(url){
    const response = await fetch(url, { method:"POST", cache:"no-store" });
    if(!response.ok) throw new Error(`${url} failed: ${response.status}`);
    return response;
  },

  deleteJson(url, payload){
    return this.json(url, {
      method:"DELETE",
      headers:{ "Content-Type":"application/json" },
      body:JSON.stringify(payload),
      cache:"no-store"
    });
  }
};
