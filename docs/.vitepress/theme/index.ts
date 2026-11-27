import type { Theme } from "vitepress";
import DefaultTheme from "vitepress/theme";
import { h } from "vue";
import Clawd3D from "./components/Clawd3D.vue";
import ClawdApp from "./components/ClawdApp.vue";
import ModelDownloads from "./components/ModelDownloads.vue";
import "./custom.css";

export default {
  extends: DefaultTheme,
  // The home hero shows the 3D model instead of a still image.
  Layout: () => h(DefaultTheme.Layout, null, { "home-hero-image": () => h(Clawd3D, { compact: true }) }),
  enhanceApp({ app }) {
    app.component("ClawdApp", ClawdApp);
    app.component("ModelDownloads", ModelDownloads);
    app.component("Clawd3D", Clawd3D);
  },
} satisfies Theme;
