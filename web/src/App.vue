<template>
  <div class="app">
    <header class="top-bar">
      <h1 class="app-title">充电桩运营管理后台</h1>
      <div class="top-bar-info">
        <span class="charger-summary">{{ chargerSummaryText }}</span>
        <span class="clock">{{ currentTime }}</span>
      </div>
    </header>
    <DashboardView
      :summary="summary"
      :chargers="chargers"
      :orders="orders"
      :connected="connected"
      :backendStatus="backendStatus"
      :onReconnect="reconnect"
    />
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { useWebSocket } from './composables/useWebSocket.js'
import DashboardView from './views/DashboardView.vue'

const summary = ref({})
const chargers = ref([])
const orders = ref([])
const currentTime = ref('')

const { connected, backendStatus, onData, reconnect } = useWebSocket()

onData((msg) => {
  const t = msg.type
  const d = msg.data || {}
  if (t === 'admin.summary.result') {
    summary.value = d
  } else if (t === 'admin.chargers.result') {
    chargers.value = d.items || []
  } else if (t === 'admin.orders.result') {
    orders.value = d.items || []
  }
})

const chargerSummaryText = computed(() => {
  const c = summary.value
  return `电桩 ${c.chargers || 0}  |  空闲 ${c.idle || 0}  |  充电中 ${c.charging || 0}  |  故障 ${c.fault || 0}`
})

let clockTimer = null
onMounted(() => {
  currentTime.value = new Date().toLocaleString('zh-CN')
  clockTimer = setInterval(() => {
    currentTime.value = new Date().toLocaleString('zh-CN')
  }, 1000)
})
onUnmounted(() => clearInterval(clockTimer))
</script>
