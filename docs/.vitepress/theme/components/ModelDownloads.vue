<script setup>
import { onMounted, ref } from "vue";
import { withBase } from "vitepress";

const TYPES = {
  "3mf": { tag: "3D print", note: "Printable mesh (3MF)" },
  fcstd: { tag: "FreeCAD", note: "Editable CAD source" },
  svg: { tag: "Drawing", note: "Vector drawing export" },
  jpeg: { tag: "Image", note: "Render / photo" },
  jpg: { tag: "Image", note: "Render / photo" },
  png: { tag: "Image", note: "Render / photo" },
};

const files = ref(null);
const failed = ref(false);

function ext(name) {
  const i = name.lastIndexOf(".");
  return i < 0 ? "" : name.slice(i + 1).toLowerCase();
}

function meta(name) {
  return TYPES[ext(name)] || { tag: ext(name).toUpperCase() || "File", note: "Download" };
}

function humanSize(bytes) {
  if (!Number.isFinite(bytes)) return "";
  const units = ["B", "KB", "MB", "GB"];
  let value = bytes;
  let unit = 0;
  while (value >= 1024 && unit < units.length - 1) {
    value /= 1024;
    unit += 1;
  }
  const rounded = value >= 10 || unit === 0 ? Math.round(value) : value.toFixed(1);
  return `${rounded} ${units[unit]}`;
}

onMounted(async () => {
  try {
    const res = await fetch(withBase("/models/manifest.json"), { cache: "no-cache" });
    if (!res.ok) throw new Error(String(res.status));
    files.value = await res.json();
  } catch {
    failed.value = true;
  }
});
</script>

<template>
  <div class="models">
    <p v-if="failed" class="models-empty">
      Model list is unavailable here. Browse them on
      <a href="https://github.com/LuSaxy/claude-keyboard/tree/main/models">GitHub</a>.
    </p>
    <p v-else-if="files === null" class="models-empty">Loading files…</p>
    <p v-else-if="files.length === 0" class="models-empty">No model files published yet.</p>
    <div v-else class="models-grid">
      <a
        v-for="file in files"
        :key="file.name"
        class="model-card"
        :href="withBase('/models/' + file.name)"
        download
      >
        <span class="model-type">{{ meta(file.name).tag }}</span>
        <span class="model-name">{{ file.name }}</span>
        <span class="model-note">{{ meta(file.name).note }}</span>
        <span class="model-foot">
          <span class="model-size">{{ humanSize(file.size) }}</span>
          <span class="model-dl">Download ↓</span>
        </span>
      </a>
    </div>
  </div>
</template>

<style scoped>
.models {
  margin: 24px 0;
}

.models-empty {
  color: var(--vp-c-text-2);
}

.models-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
  gap: 16px;
}

.model-card {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 18px;
  background: var(--vp-c-bg-elv);
  border: 1px solid var(--vp-c-divider);
  border-radius: 14px;
  color: var(--vp-c-text-1);
  text-decoration: none;
  transition: transform 0.1s ease, border-color 0.15s ease;
}

.model-card:hover {
  transform: translateY(-2px);
  border-color: var(--vp-c-brand-3);
}

.model-type {
  align-self: flex-start;
  font-size: 11px;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--vp-c-brand-1);
  background: var(--vp-c-brand-soft);
  padding: 4px 10px;
  border-radius: 999px;
}

.model-name {
  font-weight: 600;
  font-size: 15px;
  word-break: break-word;
}

.model-note {
  font-size: 13px;
  color: var(--vp-c-text-2);
}

.model-foot {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-top: 8px;
  padding-top: 10px;
  border-top: 1px solid var(--vp-c-divider);
}

.model-size {
  font-size: 12px;
  color: var(--vp-c-text-2);
}

.model-dl {
  font-size: 13px;
  font-weight: 600;
  color: var(--vp-c-brand-1);
}
</style>
