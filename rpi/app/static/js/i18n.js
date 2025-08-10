let translations = {};
let currentLang = 'en';


async function loadTranslations(lang) {
  try {
    const res = await fetch(`/static/locales/${lang}.json`);
    if (!res.ok) throw new Error("Translation file not found");
    translations = await res.json();
    currentLang = lang;
    applyTranslations();
  } catch (e) {
    console.error(`Failed to load translations for '${lang}':`, e);
  }
}

function applyTranslations() {
  document.querySelectorAll("[data-i18n]").forEach(el => {
    const key = el.dataset.i18n;
    if (!key || !translations[key]) {
      console.warn(`No translation for key: "${key}"`);
      return;
    }
    // Определяем, как применить перевод
    if (el.dataset.i18nAttr) {
      // Если задан data-i18n-attr, обновляем атрибут
      const attr = el.dataset.i18nAttr;
      el.setAttribute(attr, translations[key]);
    } else {
      // По умолчанию — текстовое содержимое
      el.textContent = translations[key];
    }
  });
}

function setLanguage(lang) {
  localStorage.setItem('lang', lang);
  document.getElementById("current-lang").textContent = lang.toUpperCase();
  loadTranslations(lang);
}

document.addEventListener("DOMContentLoaded", () => {
  const lang = localStorage.getItem("lang") || "en";
  setLanguage(lang);
});