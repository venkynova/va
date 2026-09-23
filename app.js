// Venky Agent v1 - provider layer
const VenkyAI = {
  async ask(prompt) {
    const cfg = window.VENKY_CONFIG || {};
    if (cfg.provider === "ollama") return this.askOllama(prompt, cfg);
    if (cfg.provider === "gemini") return this.askGemini(prompt, cfg);
    return "Venky Agent is running in local demo mode. Configure Ollama or Gemini in config.js.";
  },

  async askOllama(prompt, cfg) {
    const base = cfg.endpoint || "http://localhost:11434";
    const res = await fetch(base + "/api/chat", {
      method: "POST",
      headers: {"Content-Type":"application/json"},
      body: JSON.stringify({
        model: cfg.model || "gemma3:4b",
        stream: false,
        messages: [{role:"user", content:prompt}]
      })
    });
    if (!res.ok) throw new Error("Ollama request failed: " + res.status);
    const data = await res.json();
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
    if (!res.ok) throw new Error("Gemini request failed: " + res.status);
    const data = await res.json();
    return data?.candidates?.[0]?.content?.parts?.[0]?.text || "No response from Gemini.";
  }
};
