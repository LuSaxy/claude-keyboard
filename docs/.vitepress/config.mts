import { defineConfig } from "vitepress";

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Clawd",
  description:
    "A tiny two-button BLE + USB keyboard built on the Seeed Studio XIAO nRF52840 — print it, flash it, and remap it from the browser.",
  lang: "en-US",

  // Project page served from https://lusaxy.github.io/claude-keyboard/
  base: "/claude-keyboard/",

  cleanUrls: true,
  lastUpdated: true,
  // The generated README pages carry repo-relative links; don't fail the build.
  ignoreDeadLinks: true,

  themeConfig: {
    logo: "/logo.svg",

    nav: [
      { text: "Setup", link: "/setup" },
      { text: "Models", link: "/models" },
      { text: "Control app", link: "/app" },
      { text: "Firmware", link: "/firmware" },
      { text: "README", link: "/readme" },
    ],

    sidebar: [
      {
        text: "Guide",
        items: [
          { text: "Overview", link: "/readme" },
          { text: "Setup guide", link: "/setup" },
          { text: "Models & downloads", link: "/models" },
          { text: "Control app", link: "/app" },
        ],
      },
      {
        text: "Reference",
        items: [{ text: "Firmware docs", link: "/firmware" }],
      },
    ],

    socialLinks: [
      { icon: "github", link: "https://github.com/LuSaxy/claude-keyboard" },
    ],

    search: { provider: "local" },

    footer: {
      message: "Built on the Seeed Studio XIAO nRF52840.",
      copyright: "Clawd Keyboard",
    },
  },
});
