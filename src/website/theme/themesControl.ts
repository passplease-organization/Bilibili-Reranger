import {readonly, ref } from 'vue';

const themeList = THEMES

// 用于追踪当前激活的主题
const activeTheme = ref('light');
const currentTheme = ref<string | null>(localStorage.getItem('theme'));

// 设置主题的函数
export function setTheme(theme: string): void{
  document.documentElement.setAttribute('data-theme', theme);
  localStorage.setItem('theme', theme);
  activeTheme.value = theme;
  currentTheme.value = activeTheme.value;
}

function initializeTheme(){
  const savedTheme = localStorage.getItem('theme');

  if (savedTheme && themeList.includes(savedTheme)) {
    setTheme(savedTheme);
  } else {
    const prefersDark = window.matchMedia('(prefers-color-scheme: dark)').matches;
    setTheme(prefersDark ? 'dark' : 'light');
  }
}
initializeTheme();

export function useTheme() {
  return {
    theme: readonly(currentTheme),
    setTheme,
    themes: THEMES,
  };
}
