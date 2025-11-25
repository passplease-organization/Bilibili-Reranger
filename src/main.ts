import { createApp } from 'vue'
import { createPinia } from 'pinia'

import MainPage from './website/App.vue'
import router from './router'
import '@/main.css'
import '@/website/theme/themesControl.ts'

const app = createApp(MainPage)

app.use(createPinia())
app.use(router)

app.mount('#app')
