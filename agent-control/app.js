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
      if (assetCatalogLoaded) assetPlanButton.disabled = !providerSelect.value;
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


  const scanAssetsButton = byId("scan-assets");
  const assetStatus = byId("asset-status");
  const assetPlanButton = byId("asset-plan");
  const assetPlanResult = byId("asset-plan-result");
  const assetSelects = {
    player_model: byId("asset-player"),
    weapon_model: byId("asset-weapon"),
    map: byId("asset-map"),
    mapobject: byId("asset-mapobject")
  };
  let assetCatalogLoaded = false;

  function fillAssetSelect(select, values, placeholder) {
    select.replaceChildren();
    const empty = document.createElement("option");
    empty.value = "";
    empty.textContent = placeholder;
    select.appendChild(empty);
    values.forEach(item => {
      const option = document.createElement("option");
      option.value = typeof item === "string" ? item : item.name;
      option.textContent = option.value;
      select.appendChild(option);
    });
  }

  async function scanAssets() {
    const base = byId("gateway").value.trim().replace(/\/$/, "");
    const key = byId("check-key").value;
    scanAssetsButton.disabled = true;
    assetStatus.textContent = "Scanning configured PK3 archives…";
    assetStatus.className = "result";
    try {
      const response = await fetch(base + "/api/assets", {
        headers: { Authorization: "Bearer " + key }, cache: "no-store"
      });
      const body = await response.json();
      if (!response.ok) throw new Error(body.error || "Asset scan failed.");
      const counts = body.counts || {};
      fillAssetSelect(assetSelects.player_model, body.players || [], "Choose player model");
      fillAssetSelect(assetSelects.weapon_model, body.weapons || [], "Choose weapon model");
      fillAssetSelect(assetSelects.map, body.maps || [], "Choose map");
      fillAssetSelect(assetSelects.mapobject, body.mapobjects || [], "Choose map object set");
      assetCatalogLoaded = (counts.archives || 0) > 0;
      assetPlanButton.disabled = !assetCatalogLoaded || !providerSelect.value;
      assetStatus.textContent = assetCatalogLoaded
        ? "Indexed " + counts.archives + " PK3 archive(s): " + counts.players + " player models, " +
          counts.weapons + " weapon models, " + counts.mapobjects + " map-object groups, " +
          counts.maps + " maps, " + counts.shaders + " shaders and " + counts.textures + " textures. " +
          (body.errors && body.errors.length ? "Warnings: " + body.errors.join(" | ") : "")
        : ((body.errors || []).join(" ") || "No PK3 archives found in the configured asset folder.");
      assetStatus.className = assetCatalogLoaded ? "result ok" : "result bad";
    } catch (error) {
      assetCatalogLoaded = false;
      assetPlanButton.disabled = true;
      assetStatus.textContent = "Asset scan failed: " + error.message;
      assetStatus.className = "result bad";
    } finally {
      scanAssetsButton.disabled = false;
    }
  }

  scanAssetsButton.addEventListener("click", scanAssets);

  assetPlanButton.addEventListener("click", async () => {
    const base = byId("gateway").value.trim().replace(/\/$/, "");
    const key = byId("check-key").value;
    const prompt = byId("asset-prompt").value.trim();
    if (!providerSelect.value || !prompt || !assetCatalogLoaded) {
      assetPlanResult.textContent = "Scan the PK3 archives, choose a configured provider, and describe the desired setup.";
      assetPlanResult.className = "result bad";
      return;
    }
    assetPlanButton.disabled = true;
    assetPlanResult.textContent = "AI is choosing only from assets found in your archives…";
    assetPlanResult.className = "result";
    try {
      const response = await fetch(base + "/api/ai/asset-plan", {
        method: "POST",
        headers: { Authorization: "Bearer " + key, "Content-Type": "application/json" },
        body: JSON.stringify({ provider: providerSelect.value, model: modelField.value.trim(), prompt })
      });
      const body = await response.json();
      if (!response.ok) throw new Error(body.detail || body.error || "Asset selection failed.");
      const plan = body.plan;
      Object.keys(assetSelects).forEach(field => {
        if (plan[field]) assetSelects[field].value = plan[field];
      });
      assetPlanResult.textContent = "Validated AI selection — Player: " + (plan.player_model || "not selected") +
        "; Weapon: " + (plan.weapon_model || "not selected") + "; Map: " + (plan.map || "not selected") +
        "; Map objects: " + (plan.mapobject || "not selected") + ". " + (plan.reason || "");
      assetPlanResult.className = "result ok";
    } catch (error) {
      assetPlanResult.textContent = "AI asset selection failed: " + error.message;
      assetPlanResult.className = "result bad";
    } finally {
      assetPlanButton.disabled = !assetCatalogLoaded || !providerSelect.value;
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
      result.textContent = "Access verified. Gateway: " + (body.name || "Quake III Agent Gateway") + ". Input backend: " + (body.input_backend || "unavailable") + ".";
      result.className = "result ok";
      await loadProviders(base, key);
      if (assetCatalogLoaded) assetPlanButton.disabled = !providerSelect.value;
    } catch (error) {
      result.textContent = "Connection failed: " + error.message + ". Check that the gateway is running and the key matches.";
      result.className = "result bad";
    } finally {
      checkButton.disabled = false;
    }
  });
})();