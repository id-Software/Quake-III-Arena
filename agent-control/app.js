(() => {
  const byId = (id) => document.getElementById(id);
  const generate = byId("generate");
  const keyWrap = byId("key-wrap");
  const keyField = byId("agent-key");
  const checkButton = byId("check");
  const result = byId("result");

  generate.addEventListener("click", () => {
    if (!globalThis.crypto || !crypto.getRandomValues) {
      result.textContent = "Secure key generation is unavailable in this browser. Use a modern browser over HTTPS or localhost.";
      result.className = "result bad";
      return;
    }
    const bytes = new Uint8Array(32);
    crypto.getRandomValues(bytes);
    keyField.value = "q3a_" + Array.from(bytes, b => b.toString(16).padStart(2, "0")).join("");
    keyWrap.hidden = false;
    byId("check-key").value = keyField.value;
    result.textContent = "Key generated locally. Set it as AGENT_API_KEY on the gateway before testing.";
    result.className = "result";
  });

  byId("copy").addEventListener("click", async () => {
    try {
      await navigator.clipboard.writeText(keyField.value);
      byId("copy").textContent = "Copied";
    } catch {
      keyField.focus();
      keyField.select();
      result.textContent = "Clipboard access was blocked. The key is selected; copy it manually.";
      result.className = "result bad";
    }
  });

  checkButton.addEventListener("click", async () => {
    const base = byId("gateway").value.trim().replace(/\/$/, "");
    const key = byId("check-key").value;
    if (!base || !key) {
      result.textContent = "Enter both a gateway URL and an agent key.";
      result.className = "result bad";
      return;
    }
    checkButton.disabled = true;
    result.textContent = "Checking gateway…";
    result.className = "result";
    try {
      const response = await fetch(base + "/api/agent/status", {
        headers: { Authorization: "Bearer " + key },
        cache: "no-store"
      });
      const body = await response.json();
      if (!response.ok) throw new Error(body.error || "Gateway rejected the credential (HTTP " + response.status + ").");
      result.textContent = "Access verified. Gateway: " + (body.name || "Quake III Agent Gateway") + ". Game-engine integration: " + (body.engine_connected ? "connected" : "not connected yet") + ".";
      result.className = "result ok";
    } catch (error) {
      result.textContent = "Connection failed: " + error.message + ". Check that the gateway is running and the key matches.";
      result.className = "result bad";
    } finally {
      checkButton.disabled = false;
    }
  });
})();