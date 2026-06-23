import type { Theme } from "vitepress";
import DefaultTheme from "vitepress/theme";
import ClawdApp from "./components/ClawdApp.vue";
import ModelDownloads from "./components/ModelDownloads.vue";
import "./custom.css";

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component("ClawdApp", ClawdApp);
    app.component("ModelDownloads", ModelDownloads);
  },
} satisfies Theme;
