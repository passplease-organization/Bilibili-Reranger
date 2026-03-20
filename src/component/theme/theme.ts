import {ref, type Ref, watch} from "vue";

type THEME = "dark" | "light";
const theme: string = 'theme';

function getTheme(): THEME{
    const darkMode = localStorage.getItem(theme);
    if(darkMode)
        return <THEME>darkMode;
    else return window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';
}

const dark: Ref<THEME> = ref<THEME>(getTheme());

function syncTheme(){
    if(dark.value === 'dark')
        document.documentElement.classList.add('dark');
    else document.documentElement.classList.remove('dark');
}

syncTheme();

watch(
    dark,
    (newValue) => {
        syncTheme();
        localStorage.setItem(theme, newValue);
    }
);

export default dark;