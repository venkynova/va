// Venky Agent v1 - local-first provider layer
const VenkyAI = {
  async ask(prompt) {
    const cfg = window.VENKY_CONFIG || {};
    if (cfg.provider === "proxy" || !cfg.provider) return this.askProxy(prompt);
    if (cfg.provider === "ollama") return this.askOllama(prompt, cfg);
    if (cfg.provider === "gemini") return this.askGemini(prompt, cfg);
    return "Venky Agent is running in demo mode.";
  },

  async askProxy(prompt) {
    const res = await fetch("/api/chat", {
      method: "POST",
      headers: {"Content-Type":"application/json"},
      body: JSON.stringify({model:"gemma3:4b",stream:false,messages:[{role:"user",content:prompt}]})
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data?.error || ("Local AI request failed: " + res.status));
    return data?.message?.content || "No response from local model.";
  },

  async askOllama(prompt, cfg) {
    const base = cfg.endpoint || "http://localhost:11434";
    const res = await fetch(base + "/api/chat", {
      method:"POST",
      headers:{"Content-Type":"application/json"},
      body:JSON.stringify({model:cfg.model || "gemma3:4b",stream:false,messages:[{role:"user",content:prompt}]})
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data?.error || ("Ollama request failed: " + res.status));
    return data?.message?.content || "No response from local model.";
  },

  async askGemini(prompt, cfg) {
    if (!cfg.apiKey) throw new Error("Gemini API key is not configured.");
    const model = cfg.model || "gemini-2.5-flash";
    const url = "https://generativelanguage.googleapis.com/v1beta/models/" +
      encodeURIComponent(model) + ":generateContent?key=" + encodeURIComponent(cfg.apiKey);
    const res = await fetch(url, {
      method:"POST",
      headers:{"Content-Type":"application/json"},
      body:JSON.stringify({contents:[{parts:[{text:prompt}]}]})
    });
    const data = await res.json().catch(() => ({}));
    if (!res.ok) throw new Error(data?.error?.message || ("Gemini request failed: " + res.status));
    return data?.candidates?.[0]?.content?.parts?.[0]?.text || "No response from Gemini.";
  }
};
