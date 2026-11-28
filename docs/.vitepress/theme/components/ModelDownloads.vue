<script setup>
import { computed, onMounted, ref } from "vue";
import { withBase } from "vitepress";

const TYPES = {
  "3mf": { tag: "3D print", note: "Printable mesh (3MF)" },
  stl: { tag: "3D print", note: "Printable V2 part (millimetres)" },
  ket: { tag: "Ket source", note: "Editable CAD + PCB project; remote assets need network" },
  step: { tag: "CAD", note: "Assembly geometry — not a single printable object" },
  zip: { tag: "Package", note: "PCB manufacturing files (Gerbers, drill; BOM/CPL in the JLCPCB package)" },
  csv: { tag: "Assembly", note: "BOM or component placement list" },
  json: { tag: "Report", note: "Machine-readable verification / manufacturing information" },
  md: { tag: "Guide", note: "Printing, assembly and ordering notes" },
};

const files = ref(null);
const failed = ref(false);

// Tabs group the files by what they are for.
const GROUPS = [
  { id: "print", label: "Print", test: (n) => /\.(stl|3mf)$/i.test(n) },
  { id: "pcb", label: "PCB", test: (n) => n.includes("/pcb/") },
  { id: "source", label: "Source & docs", test: () => true },
];
const tab = ref("print");

function groupOf(name) {
  return GROUPS.find((g) => g.test(name)).id;
}

const shown = computed(() => (files.value || []).filter((f) => groupOf(f.name) === tab.value));
const counts = computed(() =>
  Object.fromEntries(GROUPS.map((g) => [g.id, (files.value || []).filter((f) => groupOf(f.name) === g.id).length])),
);

// Paths are shown without the common "v2/" folder.
function shortName(name) {
  return name.replace(/^v2\//, "");
}

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
    <template v-else>
    <div class="models-tabs" role="tablist">
      <button
        v-for="g in GROUPS"
        :key="g.id"
        type="button"
        role="tab"
        class="models-tab"
        :class="{ active: tab === g.id }"
        :aria-selected="tab === g.id"
        @click="tab = g.id"
      >
        {{ g.label }} <span class="models-count">{{ counts[g.id] }}</span>
      </button>
    </div>
    <div class="models-grid">
      <a
        v-for="file in shown"
        :key="file.name"
        class="model-card"
        :href="withBase('/models/' + file.name)"
        download
      >
        <span class="model-type">{{ meta(file.name).tag }}</span>
        <span class="model-name">{{ shortName(file.name) }}</span>
        <span class="model-note">{{ meta(file.name).note }}</span>
        <span class="model-foot">
          <span class="model-size">{{ humanSize(file.size) }}</span>
          <span class="model-dl">Download ↓</span>
        </span>
      </a>
    </div>
    </template>
  </div>
</template>

<style scoped>
.models {
  margin: 24px 0;
}

.models-empty {
  color: var(--vp-c-text-2);
}

.models-tabs {
  display: flex;
  flex-wrap: wrap;
  gap: 4px;
  margin-bottom: 16px;
  border-bottom: 1px solid var(--vp-c-divider);
}

.models-tab {
  padding: 8px 14px;
  margin-bottom: -1px;
  border-bottom: 2px solid transparent;
  font-size: 14px;
  font-weight: 600;
  color: var(--vp-c-text-2);
  transition: color 0.15s ease, border-color 0.15s ease;
}

.models-tab:hover {
  color: var(--vp-c-text-1);
}

.models-tab.active {
  color: var(--vp-c-brand-1);
  border-bottom-color: var(--vp-c-brand-1);
}

.models-count {
  margin-left: 4px;
  font-size: 12px;
  font-weight: 500;
  color: var(--vp-c-text-3);
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
