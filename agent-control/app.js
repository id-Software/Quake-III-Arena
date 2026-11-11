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

  const providerSelect = byId("provider");
  const modelField = byId("model");
  const askButton = byId("ask");
  const aiResult = byId("ai-result");
  const controlButton = byId("control");
  const controlResult = byId("control-result");

  async function loadProviders(base, key) {
    const response = await fetch(base + "/api/models", {
      headers: { Authorization: "Bearer " + key }, cache: "no-store"
    });
    const body = await response.json();
    if (!response.ok) throw new Error(body.error || "Could not load providers.");
    providerSelect.replaceChildren();
    body.providers.forEach(provider => {
      const option = document.createElement("option");
      option.value = provider.id;
      option.textContent = provider.name + (provider.configured ? " — configured" : " — not configured");
      option.disabled = !provider.configured;
      option.dataset.model = provider.model;
      providerSelect.appendChild(option);
    });
    const firstReady = body.providers.find(provider => provider.configured);
    if (firstReady) {
      providerSelect.value = firstReady.id;
      modelField.value = firstReady.model;
      askButton.disabled = false;
      controlButton.disabled = false;
      aiResult.textContent = "Providers loaded. Select a configured provider and send a prompt.";
      aiResult.className = "result ok";
    } else {
      askButton.disabled = true;
      controlButton.disabled = true;
      aiResult.textContent = "No providers configured. Add one or more provider API keys to the gateway environment.";
      aiResult.className = "result bad";
    }
  }

  providerSelect.addEventListener("change", () => {
    const option = providerSelect.selectedOptions[0];
    if (option) modelField.value = option.dataset.model || "";
  });

  askButton.addEventListener("click", async () => {
    const base = byId("gateway").value.trim().replace(/\/$/, "");
    const key = byId("check-key").value;
    const prompt = byId("prompt").value.trim();
    if (!providerSelect.value || !prompt) {
      aiResult.textContent = "Choose a configured provider and enter a prompt.";
      aiResult.className = "result bad";
      return;
    }
    askButton.disabled = true;
    aiResult.textContent = "Waiting for the model response…";
    aiResult.className = "result";
    try {
      const response = await fetch(base + "/api/ai/chat", {
        method: "POST",
        headers: { "Authorization": "Bearer " + key, "Content-Type": "application/json" },
        body: JSON.stringify({ provider: providerSelect.value, model: modelField.value.trim(), prompt })
      });
      const body = await response.json();
      if (!response.ok) throw new Error(body.detail || body.error || "AI request failed.");
      aiResult.textContent = body.response || "(The provider returned an empty response.)";
      aiResult.className = "result ok ai-output";
    } catch (error) {
      aiResult.textContent = "AI request failed: " + error.message;
      aiResult.className = "result bad";
    } finally {
      askButton.disabled = false;
    }
  });

  controlButton.addEventListener("click", async () => {
    const base = byId("gateway").value.trim().replace(/\/$/, "");
    const key = byId("check-key").value;
    const prompt = byId("control-prompt").value.trim();
    if (!providerSelect.value || !prompt) {
      controlResult.textContent = "Choose a configured model and describe the movement."; controlResult.className = "result bad"; return;
    }
    controlButton.disabled = true; controlResult.textContent = "Model is planning a short movement pulse…"; controlResult.className = "result";
    try {
      const response = await fetch(base + "/api/ai/control", { method: "POST", headers: { Authorization: "Bearer " + key, "Content-Type": "application/json" }, body: JSON.stringify({ provider: providerSelect.value, model: modelField.value.trim(), prompt }) });
      const body = await response.json();
      if (!response.ok) throw new Error(body.detail || body.error || "Control request failed.");
      controlResult.textContent = "Input sent to " + body.control.window + ". Action: " + JSON.stringify(body.control.action);
      controlResult.className = "result ok";
    } catch (error) { controlResult.textContent = "Control failed: " + error.message; controlResult.className = "result bad"; }
    finally { controlButton.disabled = false; }
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
      result.textContent = "Access verified. Gateway: " + (body.name || "Quake III Agent Gateway") + ". Input backend: " + (body.input_backend || "unavailable") + ".";
      result.className = "result ok";
      await loadProviders(base, key);
    } catch (error) {
      result.textContent = "Connection failed: " + error.message + ". Check that the gateway is running and the key matches.";
      result.className = "result bad";
    } finally {
      checkButton.disabled = false;
    }
  });
})();